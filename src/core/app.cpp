#include "core/app.hpp"

#include "game/game.hpp"
#include "glad/gl.h"

#include "GLFW/glfw3.h"
#include "glm/fwd.hpp"
#include "resource/animation_manager.hpp"
#include "resource/model_manager.hpp"
#include "resource/shader_manager.hpp"
#include "resource/texture_manager.hpp"
#include "ui/ui_manager.hpp"

#ifdef EMBED_SHADER
#include "debug.frag.glsl.h"
#include "debug.vert.glsl.h"
#include "irradiance.frag.glsl.h"
#include "irradiance.vert.glsl.h"
#include "pbr.frag.glsl.h"
#include "pbr.vert.glsl.h"
#include "shadow.frag.glsl.h"
#include "shadow.vert.glsl.h"
#include "skybox.frag.glsl.h"
#include "skybox.vert.glsl.h"
#include "ui.frag.glsl.h"
#include "ui.vert.glsl.h"
#include "water.frag.glsl.h"

static std::string arr_to_str(unsigned char *arr, unsigned int len) {
  return std::string(reinterpret_cast<char *>(arr), len);
}
#endif

void App::render(double delta_time) {
  _handleProcessInput(delta_time);

  if (m_game.getState() == GameState::LOADING) {
    if (m_currentLoadingTask < m_loadingTasks.size()) {
      const auto &task = m_loadingTasks[m_currentLoadingTask];
      if (auto *status = dynamic_cast<TextElement *>(
              m_uiManager.getElement("loading_status"))) {
        status->text = "Loading: " + task.name;
      }
      task.task();
      m_currentLoadingTask++;
    } else {
      m_game.reset(); // Set state to START_MENU
    }
  } else {
    m_game.update(delta_time);
  }

  _updateUIElements(delta_time);

  if (m_game.getState() != GameState::LOADING) {
    m_game.render(delta_time);
  } else {
    // Clear screen for loading
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  m_uiManager.render(m_appState.windowWidth, m_appState.windowHeight);
}

App::App(GLFWwindow *window) : m_window(window) {
  glfwSetWindowUserPointer(m_window, (void *)this);

  glfwSetKeyCallback(m_window, _glfwKeyCallback);
  glfwSetCursorPosCallback(m_window, _glfwMouseMoveCallback);
  glfwSetMouseButtonCallback(m_window, _glfwMouseButtonCallback);
  glfwSetScrollCallback(m_window, _glfwScrollCallback);
  glfwSetFramebufferSizeCallback(m_window, _glfwFramebufferSizeCallback);

  // 1. Initial Resources (UI & Font)
#ifdef EMBED_SHADER
  ShaderManager::loadFromSource(
      ShaderType::UI, arr_to_str(ui_vert_glsl, ui_vert_glsl_len).c_str(),
      arr_to_str(ui_frag_glsl, ui_frag_glsl_len).c_str());

  ShaderManager::loadFromSource(
      ShaderType::PBR, arr_to_str(pbr_vert_glsl, pbr_vert_glsl_len).c_str(),
      arr_to_str(pbr_frag_glsl, pbr_frag_glsl_len).c_str());

  ShaderManager::loadFromSource(
      ShaderType::SKYBOX,
      arr_to_str(skybox_vert_glsl, skybox_vert_glsl_len).c_str(),
      arr_to_str(skybox_frag_glsl, skybox_frag_glsl_len).c_str());

  ShaderManager::loadFromSource(
      ShaderType::SHADOW,
      arr_to_str(shadow_vert_glsl, shadow_vert_glsl_len).c_str(),
      arr_to_str(shadow_frag_glsl, shadow_frag_glsl_len).c_str());

  ShaderManager::loadFromSource(
      ShaderType::IRRADIANCE,
      arr_to_str(irradiance_vert_glsl, irradiance_vert_glsl_len).c_str(),
      arr_to_str(irradiance_frag_glsl, irradiance_frag_glsl_len).c_str());

  ShaderManager::loadFromSource(
      ShaderType::WATER, arr_to_str(pbr_vert_glsl, pbr_vert_glsl_len).c_str(),
      arr_to_str(water_frag_glsl, water_frag_glsl_len).c_str());

  ShaderManager::loadFromSource(
      ShaderType::DEBUG,
      arr_to_str(debug_vert_glsl, debug_vert_glsl_len).c_str(),
      arr_to_str(debug_frag_glsl, debug_frag_glsl_len).c_str());
#else
  ShaderManager::loadFromPath(ShaderType::UI, UI_VERTEX_SHADER_PATH,
                              UI_FRAGMENT_SHADER_PATH);
#endif
  m_font.loadDefaultFont();

  // 2. Setup UI
  _setupUIElements();

  // 3. Queue other resources
  _setupResources();

  int width, height;
  glfwGetWindowSize(m_window, &width, &height);

  m_appState.windowWidth = width;
  m_appState.windowHeight = height;

  m_game.getCamera().updateSceneSize(width, height);
}

App::~App() = default;

void App::_setupResources() {
  // Helpers
  auto loadModel = [](ModelName name, const std::string &path) {
    return [name, path]() { ModelManager::load(name, path.c_str(), false); };
  };

  auto loadAnimation = [](AnimationName name, const std::string &path,
                          ModelName model_name) {
    return [name, path, model_name]() {
      AnimationManager::load(name, path.c_str(),
                             &ModelManager::get(model_name));
    };
  };

  // Shaders
#ifndef EMBED_SHADER
  m_loadingTasks.push_back(
      {"Shaders", []() {
         ShaderManager::loadFromPath(ShaderType::PBR,
                                     SHADER_PATH "/pbr.vert.glsl",
                                     SHADER_PATH "/pbr.frag.glsl");
         ShaderManager::loadFromPath(ShaderType::SKYBOX,
                                     SHADER_PATH "/skybox.vert.glsl",
                                     SHADER_PATH "/skybox.frag.glsl");
         ShaderManager::loadFromPath(ShaderType::SHADOW,
                                     SHADER_PATH "/shadow.vert.glsl",
                                     SHADER_PATH "/shadow.frag.glsl");
         ShaderManager::loadFromPath(ShaderType::IRRADIANCE,
                                     SHADER_PATH "/irradiance.vert.glsl",
                                     SHADER_PATH "/irradiance.frag.glsl");
         ShaderManager::loadFromPath(ShaderType::WATER,
                                     SHADER_PATH "/pbr.vert.glsl",
                                     SHADER_PATH "/water.frag.glsl");
         ShaderManager::loadFromPath(ShaderType::DEBUG,
                                     SHADER_PATH "/debug.vert.glsl",
                                     SHADER_PATH "/debug.frag.glsl");
       }});
#endif

  // Static Textures
  m_loadingTasks.push_back(
      {"Static Textures", []() {
         if (!TextureManager::exists(STATIC_BLACK_TEXTURE))
           TextureManager::manage(STATIC_BLACK_TEXTURE,
                                  TextureManager::generateStaticBlackTexture());
         if (!TextureManager::exists(STATIC_WHITE_TEXTURE))
           TextureManager::manage(STATIC_WHITE_TEXTURE,
                                  TextureManager::generateStaticWhiteTexture());
         if (!TextureManager::exists(STATIC_NORMAL_TEXTURE))
           TextureManager::manage(
               STATIC_NORMAL_TEXTURE,
               TextureManager::generateStaticNormalTexture());
         if (!TextureManager::exists(STATIC_PBR_DEFAULT_TEXTURE))
           TextureManager::manage(
               STATIC_PBR_DEFAULT_TEXTURE,
               TextureManager::generateStaticPBRDefaultTexture());
       }});

  // Materials
  ;

  // Models
  m_loadingTasks.push_back(
      {"Model: Kasane Teto", loadModel(ModelName::KASANE_TETO, ASSETS_PATH
                                       "/objects/kasane_teto/teto.dae")});
  m_loadingTasks.push_back(
      {"Model: Hatsune Miku", loadModel(ModelName::HATSUNE_MIKU, ASSETS_PATH
                                        "/objects/hatsune_miku/miku.dae")});
  m_loadingTasks.push_back(
      {"Model: Coin",
       loadModel(ModelName::COIN, ASSETS_PATH "/objects/items/coin.glb")});


  // Animations
  m_loadingTasks.push_back({"Animation: Kasane Teto Idle",
                            loadAnimation(AnimationName::KASANE_TETO_IDLE,
                                          ASSETS_PATH "/objects/kasane_teto/"
                                                      "teto_idle.dae",
                                          ModelName::KASANE_TETO)});
  m_loadingTasks.push_back({"Animation: Kasane Teto Walking",
                            loadAnimation(AnimationName::KASANE_TETO_WALKING,
                                          ASSETS_PATH "/objects/kasane_teto/"
                                                      "teto_walking_normal.dae",
                                          ModelName::KASANE_TETO)});
  m_loadingTasks.push_back({"Animation: Kasane Teto Running",
                            loadAnimation(AnimationName::KASANE_TETO_RUNNING,
                                          ASSETS_PATH "/objects/kasane_teto/"
                                                      "teto_running.dae",
                                          ModelName::KASANE_TETO)});
  m_loadingTasks.push_back({"Animation: Kasane Teto Dancing",
                            loadAnimation(AnimationName::KASANE_TETO_DANCING,
                                          ASSETS_PATH "/objects/kasane_teto/"
                                                      "teto_dancing.dae",
                                          ModelName::KASANE_TETO)});

  m_loadingTasks.push_back({"Animation: Hatsune Miku Walking",
                            loadAnimation(AnimationName::HATSUNE_MIKU_IDLE,
                                          ASSETS_PATH "/objects/hatsune_miku/"
                                                      "miku_idle.dae",
                                          ModelName::HATSUNE_MIKU)});
  m_loadingTasks.push_back({"Animation: Hatsune Miku Walking",
                            loadAnimation(AnimationName::HATSUNE_MIKU_WALKING,
                                          ASSETS_PATH "/objects/hatsune_miku/"
                                                      "miku_walking.dae",
                                          ModelName::HATSUNE_MIKU)});

  // Static texture generation
  m_loadingTasks.push_back(
      {"Generating Static Texture", []() {
         if (!TextureManager::exists(STATIC_WHITE_TEXTURE))
           TextureManager::manage(STATIC_WHITE_TEXTURE,
                                  TextureManager::generateStaticWhiteTexture());
         if (!TextureManager::exists(STATIC_BLACK_TEXTURE))
           TextureManager::manage(STATIC_BLACK_TEXTURE,
                                  TextureManager::generateStaticBlackTexture());
         if (!TextureManager::exists(STATIC_NORMAL_TEXTURE))
           TextureManager::manage(
               STATIC_NORMAL_TEXTURE,
               TextureManager::generateStaticNormalTexture());
         if (!TextureManager::exists(STATIC_PBR_DEFAULT_TEXTURE))
           TextureManager::manage(
               STATIC_PBR_DEFAULT_TEXTURE,
               TextureManager::generateStaticPBRDefaultTexture());
       }});

  m_loadingTasks.push_back(
      {"Terrain Textures", []() {
         if (!TextureManager::exists(TextureName("terrain_grass_diffuse"))) {
           TextureManager::load(TextureName("terrain_grass_diffuse"),
                                TextureType::DIFFUSE,
                                ASSETS_PATH "/textures/grass/1/diffuse.jpg");
         }

         if (!TextureManager::exists(TextureName("terrain_grass_normal"))) {
           TextureManager::load(TextureName("terrain_grass_normal"),
                                TextureType::NORMAL,
                                ASSETS_PATH "/textures/grass/1/normal.jpg");
         }

         if (!TextureManager::exists(TextureName("terrain_grass_height"))) {
           TextureManager::load(TextureName("terrain_grass_height"),
                                TextureType::HEIGHT,
                                ASSETS_PATH "/textures/grass/1/height.jpg");
         }

         if (!TextureManager::exists(TextureName("terrain_grass_roughness"))) {
           TextureManager::load(TextureName("terrain_grass_roughness"),
                                TextureType::ROUGHNESS,
                                ASSETS_PATH "/textures/grass/1/roughness.jpg");
         }

         if (!TextureManager::exists(TextureName("terrain_grass_ao"))) {
           TextureManager::load(TextureName("terrain_grass_ao"),
                                TextureType::AO,
                                ASSETS_PATH "/textures/grass/1/ao.jpg");
         }
       }});

  // Skybox
  m_loadingTasks.push_back(
      {"Skybox", []() {
         TextureManager::loadCubemap(
             TextureName("skybox"),
             {
                 (ASSETS_PATH "/textures/skybox/sky_1/px.hdr"), // +X
                 (ASSETS_PATH "/textures/skybox/sky_1/nx.hdr"), // -X
                 (ASSETS_PATH "/textures/skybox/sky_1/py.hdr"), // +Y
                 (ASSETS_PATH "/textures/skybox/sky_1/ny.hdr"), // -Y
                 (ASSETS_PATH "/textures/skybox/sky_1/pz.hdr"), // +Z
                 (ASSETS_PATH "/textures/skybox/sky_1/nz.hdr"), // -Z
             });
       }});

  // Game Setup
  m_loadingTasks.push_back({"Game Engine Initializing", [this]() {
                              m_game.setup();
                              m_game.setDebugAABB(true);
                            }});
}

void App::_setupUIElements() {
  m_uiManager.addTextElement("fps_counter", {1.0f, 1.0f, 0.0f, 0.0f}, "FPS: 0",
                             m_font, {1.0f, 1.0f, 1.0f, 1.0f}, 0.15f);

  // Loading Screen
  m_uiManager.addTextElement("loading_title", {0.0f, 15.0f, 0.0f, 0.0f},
                             "LOADING...", m_font, {1.0f, 1.0f, 1.0f, 1.0f},
                             0.3f);
  m_uiManager.addTextElement("loading_status", {0.0f, 22.0f, 0.0f, 0.0f},
                             "Initializing...", m_font,
                             {0.7f, 0.7f, 0.7f, 1.0f}, 0.15f);

  // Darken Screen Overlay (added first so it's behind text)
  m_uiManager.addInteractiveElement(
      "darken_screen", {0.0f, 0.0f, 100.0f, 40.0f}, {0.0f, 0.0f, 0.0f, 0.5f},
      [this]() {
        if (m_game.getState() == GameState::START_MENU)
          m_game.startGame();
        else if (m_game.getState() == GameState::GAME_OVER)
          m_game.reset();
      });

  // Start Screen
  m_uiManager.addTextElement("start_title", {0.0f, 15.0f, 0.0f, 0.0f},
                             "VAMPIRE SURVIVOR", m_font,
                             {1.0f, 0.0f, 0.0f, 1.0f}, 0.3f);
  m_uiManager.addTextElement("start_hint", {0.0f, 20.0f, 0.0f, 0.0f},
                             "CLICK TO START", m_font, {1.0f, 1.0f, 1.0f, 1.0f},
                             0.15f);
}

void App::_updateUIElements(double delta_time) {
  static double timeAccumulator = 0.0;
  static int frameCount = 0;
  static double lastFps = 0.0;

  timeAccumulator += delta_time;
  frameCount++;

  if (timeAccumulator >= 0.5) {
    lastFps = frameCount / timeAccumulator;
    timeAccumulator = 0.0;
    frameCount = 0;

    auto *fpsElement =
        dynamic_cast<TextElement *>(m_uiManager.getElement("fps_counter"));
    if (fpsElement) {
      fpsElement->text = std::format("FPS: {}", static_cast<int>(lastFps));
    }
  }

  float vWidth = m_uiManager.getVirtualWidth();
  GameState state = m_game.getState();

  // Handle Loading Screen
  if (auto *title = dynamic_cast<TextElement *>(
          m_uiManager.getElement("loading_title"))) {
    title->visible = (state == GameState::LOADING);
    float w = m_font.getTextWidth(title->text, title->scale);
    title->bounds.x = (vWidth - w) / 2.0f;
  }
  if (auto *status = dynamic_cast<TextElement *>(
          m_uiManager.getElement("loading_status"))) {
    status->visible = (state == GameState::LOADING);
    float w = m_font.getTextWidth(status->text, status->scale);
    status->bounds.x = (vWidth - w) / 2.0f;
  }

  // Score removed

  // Handle Menu Screens
  bool isMenu = (state != GameState::PLAYING);
  m_uiManager.getElement("darken_screen")->visible = isMenu;
  m_uiManager.getElement("darken_screen")->bounds.w = vWidth;

  // Fading and Centering for Start Screen
  if (auto *title =
          dynamic_cast<TextElement *>(m_uiManager.getElement("start_title"))) {
    title->visible = (state == GameState::START_MENU);
    float w = m_font.getTextWidth(title->text, title->scale);
    title->bounds.x = (vWidth - w) / 2.0f;
  }

  if (auto *hint =
          dynamic_cast<TextElement *>(m_uiManager.getElement("start_hint"))) {
    hint->visible = (state == GameState::START_MENU);
    float w = m_font.getTextWidth(hint->text, hint->scale);
    hint->bounds.x = (vWidth - w) / 2.0f;
    hint->color.a =
        0.3f + 0.7f * (0.5f * (std::cos(glfwGetTime() * 4.0) + 1.0f));
  }

  // Removed Game Over Fade out
}

void App::_handleProcessInput(double delta_time) {
  (void)delta_time;

  const float step_amount = 0.8f;
  glm::vec3 direction(0.0f);

  if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS ||
      glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS) {
    direction += glm::vec3(0.0f, 0.0f, -step_amount);
  }

  if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS ||
      glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS) {
    direction += glm::vec3(-step_amount, 0.0f, 0.0f);
  }

  if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS ||
      glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS) {
    direction += glm::vec3(0.0f, 0.0f, step_amount);
  }

  if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS ||
      glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
    direction += glm::vec3(step_amount, 0.0f, 0.0f);
  }

  if (direction != glm::vec3(0.0f))
    m_game.movePlayer(direction);
}

void App::_handleKeyCallback(int key, int scancode, int action, int mods) {
  (void)scancode;
  (void)mods;

  if (action != GLFW_PRESS)
    return;

  if (key == GLFW_KEY_SPACE) {
    switch (m_game.getState()) {
    case GameState::START_MENU:
      m_game.startGame();
      break;
    case GameState::PLAYING:
      break;
    case GameState::GAME_OVER:
      m_game.reset();
      break;
    case GameState::LOADING:
      break;
    }
  }
}

// internal event handler
void App::_handleMouseMoveCallback(double pos_x, double pos_y) {
  m_appState.inputState.mouseLastX = pos_x;
  m_appState.inputState.mouseLastY = pos_y;
}

void App::_handleMouseClickCallback(int button, int action, int mods) {
  (void)mods;

  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    m_uiManager.handleClick(m_appState.inputState.mouseLastX,
                            m_appState.inputState.mouseLastY);
  }
}

void App::_handleScrollCallback(double offset_x, double offset_y) {
  (void)offset_x;
  (void)offset_y;
}

void App::_handleFramebufferSizeCallback(int width, int height) {
  glViewport(0, 0, width, height);
  m_appState.windowWidth = width;
  m_appState.windowHeight = height;
  m_game.getCamera().updateSceneSize(width, height);
}

// GLFW static callbacks adapters
void App::_glfwKeyCallback(GLFWwindow *window, int key, int scancode,
                           int action, int mods) {
  App *app = static_cast<App *>(glfwGetWindowUserPointer(window));

  if (app) {
    app->_handleKeyCallback(key, scancode, action, mods);
  }
}

void App::_glfwMouseMoveCallback(GLFWwindow *window, double x_pos,
                                 double y_pos) {
  App *app = static_cast<App *>(glfwGetWindowUserPointer(window));

  if (app) {
    app->_handleMouseMoveCallback(x_pos, y_pos);
  }
}

void App::_glfwMouseButtonCallback(GLFWwindow *window, int button, int action,
                                   int mods) {
  App *app = static_cast<App *>(glfwGetWindowUserPointer(window));

  if (app) {
    app->_handleMouseClickCallback(button, action, mods);
  }
}

void App::_glfwScrollCallback(GLFWwindow *window, double offset_x,
                              double offset_y) {
  App *app = static_cast<App *>(glfwGetWindowUserPointer(window));

  if (app) {
    app->_handleScrollCallback(offset_x, offset_y);
  }
}

void App::_glfwFramebufferSizeCallback(GLFWwindow *window, int width,
                                       int height) {
  App *app = static_cast<App *>(glfwGetWindowUserPointer(window));

  if (app) {
    app->_handleFramebufferSizeCallback(width, height);
  }
}
