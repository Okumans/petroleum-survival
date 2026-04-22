#include "game.hpp"

#include "graphics/debug_drawer.hpp"
#include "graphics/ibl_generator.hpp"
#include "graphics/idrawable.hpp"
#include "graphics/shader.hpp"
#include "resource/lighting_manager.hpp"
#include "resource/model_manager.hpp"
#include "resource/shader_manager.hpp"
#include "resource/texture_manager.hpp"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>

Game::Game()
    : m_camera(glm::vec3(0.0f, 10.0f, 10.0f)),
      m_cameraController(m_camera, glm::vec3(0.0f, 12.0f, 10.0f)),
      m_skybox(std::make_unique<Skybox>()), m_shadowMapFBO(0),
      m_shadowMapTex(0), m_state(GameState::LOADING) {
  m_camera.setPitch(-45.0f);
  m_camera.setYaw(-90.0f);
  m_camera.Zoom = 45.0f;
}

Game::~Game() {
  glDeleteFramebuffers(1, &m_shadowMapFBO);
  glDeleteTextures(1, &m_shadowMapTex);
}

void Game::setup() {
  // Setup shadow map FBO
  glGenFramebuffers(1, &m_shadowMapFBO);
  glGenTextures(1, &m_shadowMapTex);
  glBindTexture(GL_TEXTURE_2D, m_shadowMapTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH,
               SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  float border_color[] = {1.0, 1.0, 1.0, 1.0};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);

  glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         m_shadowMapTex, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Initialize static textures
  if (!TextureManager::exists(STATIC_WHITE_TEXTURE))
    TextureManager::manage(STATIC_WHITE_TEXTURE,
                           TextureManager::generateStaticWhiteTexture());
  if (!TextureManager::exists(STATIC_BLACK_TEXTURE))
    TextureManager::manage(STATIC_BLACK_TEXTURE,
                           TextureManager::generateStaticBlackTexture());
  if (!TextureManager::exists(STATIC_NORMAL_TEXTURE))
    TextureManager::manage(STATIC_NORMAL_TEXTURE,
                           TextureManager::generateStaticNormalTexture());
  if (!TextureManager::exists(STATIC_PBR_DEFAULT_TEXTURE))
    TextureManager::manage(STATIC_PBR_DEFAULT_TEXTURE,
                           TextureManager::generateStaticPBRDefaultTexture());

  // Generate Irradiance Map
  std::shared_ptr<Texture> skybox_tex =
      TextureManager::copy(TextureName("skybox"));
  m_skybox->setTexture(skybox_tex);

  Shader &irradiance_shader = ShaderManager::get(ShaderType::IRRADIANCE);
  std::shared_ptr<Texture> irradiance_map = IBLGenerator::generateIrradianceMap(
      *skybox_tex, *m_skybox, irradiance_shader);
  TextureManager::manage(TextureName("irradiance_map"),
                         std::move(*irradiance_map));

  // Setup Lights
  LightingManager::clearLights();

  // 1. "Sun" Light (Directional, casts shadows)
  LightingManager::addLight({.type = LightType::DIRECTIONAL,
                             .position = glm::vec3(0.3f, -1.0f, 0.1f),
                             .color = glm::vec3(11.0f, 9.0f, 8.0f) * .8f,
                             .castsShadows = true});

  // 2. Sky Blue Fill Light (Directional)
  LightingManager::addLight({.type = LightType::DIRECTIONAL,
                             .position = glm::vec3(0.5f, -1.0f, 0.2f),
                             .color = glm::vec3(0.1f, 0.15f, 0.25f)});

  // 3. Ground Bounce Fill (Point light)
  LightingManager::addLight({.type = LightType::POINT,
                             .position = glm::vec3(0.0f, -5.0f, 0.0f),
                             .color = glm::vec3(0.3f, 0.2f, 0.1f) * 5.0f});

  m_testObject =
      std::make_unique<Player>(ModelManager::copy(ModelName::KASANE_TETO));
  m_testObject->setScale(20.0f);

  m_testAnimation = std::make_unique<Animation>(
      ASSETS_PATH "/objects/kasane_teto/teto_walking_normal.dae",
      m_testObject->getModel().get());

  m_testAnimator = std::make_unique<Animator>(m_testAnimation.get());

  reset();
}

void Game::reset() {
  m_state = GameState::START_MENU;
  m_cameraController.setTarget(glm::vec3(0.0f, 0.0f, 0.0f), true);
}

void Game::startGame() {
  if (m_state == GameState::START_MENU) {
    m_state = GameState::PLAYING;
  }
}

void Game::update(double delta_time) {
  m_currentTime += static_cast<float>(delta_time);

  // --- PBR DEBUG: Rotate Sun ---
  float sun_speed = 0.3f;
  float sun_radius = 1.0f;
  glm::vec3 sun_dir = glm::normalize(
      glm::vec3(std::cos(m_currentTime * sun_speed) * sun_radius, -1.0f,
                std::sin(m_currentTime * sun_speed) * sun_radius));

  Light sun = LightingManager::getShadowCaster();
  sun.position = sun_dir;
  LightingManager::setLight(0, sun);
  // -----------------------------

  _updateCamera(delta_time);
  m_cameraController.update(static_cast<float>(delta_time));
  m_testObject->update(delta_time);

  if (m_testAnimator) {
    m_testAnimator->updateAnimation(static_cast<float>(delta_time));
  }
}

void Game::movePlayer(glm::vec3 vec) { m_testObject->moveWithAnimation(vec); }

void Game::render(double delta_time) {
  glEnable(GL_DEPTH_TEST);

  // 1. Shadow Pass
  m_lightSpaceMatrix =
      LightingManager::calculateLightSpaceMatrix(glm::vec3(0.0f));

  Shader &shadow_shader = ShaderManager::get(ShaderType::SHADOW);
  shadow_shader.use();
  shadow_shader.setMat4("u_LightSpaceMatrix", m_lightSpaceMatrix);

  glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
  glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFBO);
  glClear(GL_DEPTH_BUFFER_BIT);
  glDisable(GL_CULL_FACE);

  {
    RenderContext shadow_draw_ctx = {
        .shader = shadow_shader,
        .camera = m_camera,
        .deltaTime = delta_time,
    };

    if (m_testAnimator) {
      shadow_shader.setBool("u_HasAnimation", true);
      m_testAnimator->apply(shadow_shader);
    } else {
      shadow_shader.setBool("u_HasAnimation", false);
    }

    if (m_testObject)
      m_testObject->draw(shadow_draw_ctx);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glCullFace(GL_BACK); // Restore back-face culling

  // 2. Main Pass
  glViewport(0, 0, (int)m_camera.getSceneWidth(),
             (int)m_camera.getSceneHeight());
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Bind and draw Skybox first (as background)
  auto skyboxTex = TextureManager::copy(TextureName("skybox"));
  Shader &skybox_shader = ShaderManager::get(ShaderType::SKYBOX);
  glDepthMask(GL_FALSE);

  m_skybox->draw({
      .shader = skybox_shader,
      .camera = m_camera,
      .deltaTime = delta_time,
  });

  glDepthMask(GL_TRUE);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glm::mat4 projection = m_camera.getProjectionMatrix();
  glm::mat4 view = m_camera.getViewMatrix();

  Shader &pbr_shader = ShaderManager::get(ShaderType::PBR);
  pbr_shader.use();
  pbr_shader.setMat4("u_Projection", projection);
  pbr_shader.setMat4("u_View", view);
  pbr_shader.setVec3("u_CameraPos", m_camera.Position);
  pbr_shader.setMat4("u_LightSpaceMatrix", m_lightSpaceMatrix);

  // Bind Skybox for reflections
  glBindTextureUnit(10, skyboxTex->getTexID());
  pbr_shader.setInt("u_SpecularEnvMap", 10);

  // Bind Irradiance Map for diffuse IBL
  auto irradiance_map = TextureManager::copy(TextureName("irradiance_map"));
  glBindTextureUnit(12, irradiance_map->getTexID());
  pbr_shader.setInt("u_IrradianceMap", 12);

  // Bind Shadow Map
  glBindTextureUnit(11, m_shadowMapTex);
  pbr_shader.setInt("u_ShadowMap", 11);

  // Lighting setup
  LightingManager::apply(pbr_shader);

  pbr_shader.setFloat("u_HeightScale", 0.03f);
  pbr_shader.setFloat("u_AOFactor", 1.0f);
  pbr_shader.setFloat("u_AmbientIntensity", 1.0f);

  if (m_testObject) {
    pbr_shader.setVec3("u_BaseColor", glm::vec3(1.0f));
    pbr_shader.setVec2("u_UVOffset", glm::vec2(0.0f));

    if (true && m_testAnimator) {
      pbr_shader.setBool("u_HasAnimation", true);
      m_testAnimator->apply(pbr_shader);
    } else {
      pbr_shader.setBool("u_HasAnimation", false);
    }

    RenderContext ctx = {
        .shader = pbr_shader,
        .camera = m_camera,
        .deltaTime = delta_time,
    };
    m_testObject->draw(ctx);
  }

  if (m_debugAABB && m_testObject) {
    RenderContext debug_ctx = {
        .shader = pbr_shader,
        .camera = m_camera,
        .deltaTime = delta_time,
    };
    DebugDrawer::drawAABB(debug_ctx, m_testObject->getHitboxAABB(),
                          {1.0f, 0.0f, 0.0f});
    DebugDrawer::drawAABB(debug_ctx, m_testObject->getWorldAABB(),
                          {1.0f, 1.0f, 0.0f});
  }

  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
}

void Game::_updateCamera(double delta_time) {
  (void)delta_time;
  if (m_testObject) {
    // m_cameraController.follow(m_testObject->getPosition());
    m_cameraController.follow({0.0f, 0.0f, 0.0f});
  }
}
