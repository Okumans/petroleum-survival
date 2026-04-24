#include "game.hpp"

#include "graphics/debug_drawer.hpp"
#include "graphics/ibl_generator.hpp"
#include "graphics/idrawable.hpp"
#include "graphics/shader.hpp"
#include "resource/animation_manager.hpp"
#include "resource/lighting_manager.hpp"
#include "resource/model_manager.hpp"
#include "resource/shader_manager.hpp"
#include "resource/texture_manager.hpp"
#include "scene/enemy.hpp"
#include "scene/game_object.hpp"
#include "scene/item.hpp"
#include "scene/player.hpp"
#include "utility/random.hpp"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>

Game::Game()
    : m_camera(glm::vec3(0.0f, 10.0f, 10.0f)),
      m_cameraController(m_camera, glm::vec3(0.0f, 12.0f, 10.0f)),
      m_skybox(std::make_unique<Skybox>()),
      m_shadowMap(std::make_unique<ShadowMap>()), m_state(GameState::LOADING) {
  m_camera.setPitch(-45.0f);
  m_camera.setYaw(-90.0f);
  m_camera.zoom = 45.0f;
}

Game::~Game() {}

void Game::setup() {
  ShaderManager::ensureInit();
  ModelManager::ensureInit();
  AnimationManager::ensureInit();

  m_objects.clear();
  m_eventBus.clear();
  _registerGameplayEventHandlers();

  m_score = 0;

  m_player.init(
      &m_objects.emplace<Player>(ModelManager::copy(ModelName::KASANE_TETO)));
  m_player.ensureInitialized()->setScale(20.0f);
  m_player.ensureInitialized()->setup();

  for (size_t i = 0; i < 2; ++i) {
    Enemy &enemy =
        m_objects.emplace<Enemy>(ModelManager::copy(ModelName::HATSUNE_MIKU));
    enemy.setScale(60.0f);
    enemy.move(
        {Random::randFloat(2.1f, 5.0f), 0.0f, Random::randFloat(2.1f, 5.0f)});
    enemy.setup();
  }

  for (size_t i = 0; i < 4; ++i) {
    Item &coin = m_objects.emplace<Item>(ModelManager::copy(ModelName::COIN));
    coin.setScale(4.0f);
    coin.translate({Random::randFloat(-10.0f, 20.0f), 0.8f,
                    Random::randFloat(-10.0f, 20.0f)});
  }

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
  LightingManager::clear();

  // 1. "Sun" Light (Directional, casts shadows)
  LightingManager::add({.type = LightType::DIRECTIONAL,
                        .position = glm::vec3(0.3f, -1.0f, 0.1f),
                        .color = glm::vec3(11.0f, 9.0f, 8.0f) * .8f,
                        .castsShadows = true});

  // 2. Sky Blue Fill Light (Directional)
  LightingManager::add({.type = LightType::DIRECTIONAL,
                        .position = glm::vec3(0.5f, -1.0f, 0.2f),
                        .color = glm::vec3(0.1f, 0.15f, 0.25f)});

  // 3. Ground Bounce Fill (Point light)
  LightingManager::add({.type = LightType::POINT,
                        .position = glm::vec3(0.0f, -5.0f, 0.0f),
                        .color = glm::vec3(0.3f, 0.2f, 0.1f) * 5.0f});

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

  // --- PBR DEBUG: Rotate Sun ---
  static float current_time = 0.0f;

  current_time += static_cast<float>(delta_time);

  const float sun_speed = 0.03f;
  const float sun_radius = 1.0f;
  glm::vec3 sun_dir = glm::normalize(
      glm::vec3(std::cos(current_time * sun_speed) * sun_radius, -1.0f,
                std::sin(current_time * sun_speed) * sun_radius));

  Light sun = LightingManager::getShadowCaster();
  sun.position = sun_dir;
  LightingManager::set(0, sun);
  // -----------------------------

  _updateCamera(delta_time);
  m_cameraController.update(static_cast<float>(delta_time));

  // Update Player position in enemy
  for (GameObject *enemy :
       m_objects.getObjectsWithType(GameObjectType::ENEMY)) {
    static_cast<Enemy *>(enemy)->setPlayerPosition(
        m_player.ensureInitialized()->getPosition());
  }

  m_objects.update(delta_time);
  _runCollisionPass();
  m_eventBus.flush();
  m_objects.collectGarbage();
}

void Game::movePlayer(glm::vec3 vec) {
  m_player.ensureInitialized()->moveWithAnimation(vec);
}

void Game::render(double delta_time) {
  glEnable(GL_DEPTH_TEST);

  // 1. Shadow Pass
  m_shadowMap->updateLightSpaceMatrix(
      m_player.ensureInitialized()->getPosition());

  Shader &shadow_shader = ShaderManager::get(ShaderType::SHADOW);
  shadow_shader.use();
  shadow_shader.setMat4("u_LightSpaceMatrix",
                        m_shadowMap->getLightSpaceMatrix());

  m_shadowMap->bindForWriting();
  glDisable(GL_CULL_FACE);

  {
    RenderContext shadow_draw_ctx = {
        .shader = shadow_shader,
        .camera = m_camera,
        .deltaTime = delta_time,
    };

    m_objects.draw(shadow_draw_ctx);
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
  pbr_shader.setVec3("u_CameraPos", m_camera.position);
  pbr_shader.setMat4("u_LightSpaceMatrix", m_shadowMap->getLightSpaceMatrix());

  // Bind Skybox for reflections
  glBindTextureUnit(10, skyboxTex->getTexID());
  pbr_shader.setInt("u_SpecularEnvMap", 10);

  // Bind Irradiance Map for diffuse IBL
  auto irradiance_map = TextureManager::copy(TextureName("irradiance_map"));
  glBindTextureUnit(12, irradiance_map->getTexID());
  pbr_shader.setInt("u_IrradianceMap", 12);

  // Bind Shadow Map
  m_shadowMap->bindForReading(11);
  pbr_shader.setInt("u_ShadowMap", 11);

  // Lighting setup
  LightingManager::apply(pbr_shader);

  pbr_shader.setFloat("u_HeightScale", 0.03f);
  pbr_shader.setFloat("u_AOFactor", 1.0f);
  pbr_shader.setFloat("u_AmbientIntensity", 1.0f);
  pbr_shader.setVec3("u_BaseColor", glm::vec3(1.0f));
  pbr_shader.setVec2("u_UVOffset", glm::vec2(0.0f));

  RenderContext ctx = {
      .shader = pbr_shader,
      .camera = m_camera,
      .deltaTime = delta_time,
  };

  m_objects.draw(ctx);

  if (false && m_debugAABB) {
    RenderContext debug_ctx = {
        .shader = pbr_shader,
        .camera = m_camera,
        .deltaTime = delta_time,
    };

    for (GameObject *object : m_objects.getObjects()) {
      DebugDrawer::drawAABB(debug_ctx, object->getHitboxAABB(),
                            {1.0f, 0.0f, 0.0f});
    }
  }

  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
}

void Game::_updateCamera(double delta_time) {
  (void)delta_time;
  m_cameraController.follow(m_player.ensureInitialized()->getPosition());
}

void Game::_runCollisionPass() {
  for (GameObject *item_obj :
       m_objects.getObjectsWithType(GameObjectType::ITEM)) {
    if (!item_obj || item_obj->isRemovalRequested()) {
      continue;
    }

    Item *item = static_cast<Item *>(item_obj);
    if (m_player.ensureInitialized()->collidesWith(*item)) {
      m_eventBus.emit(ItemCollectedEvent{
          .player = m_player.ensureInitialized(),
          .item = item,
          .value = 1,
      });
    }
  }
}

void Game::_registerGameplayEventHandlers() {
  m_eventBus.subscribe<ItemCollectedEvent>(
      [this](const ItemCollectedEvent &evt) {
        m_score += evt.value;
        m_eventBus.emit(DespawnRequestedEvent{.object = evt.item});

        if (evt.item) {
          m_eventBus.emit(ParticleSpawnRequestedEvent{
              .position = evt.item->getPosition(),
              .effectId = 1,
          });
        }
      });

  m_eventBus.subscribe<DespawnRequestedEvent>(
      [](const DespawnRequestedEvent &evt) {
        if (evt.object) {
          evt.object->requestRemoval();
        }
      });

  m_eventBus.subscribe<ParticleSpawnRequestedEvent>(
      [](const ParticleSpawnRequestedEvent &evt) {
        (void)evt;
        // TODO: Hook this event into particle or VFX rendering once available.
      });
}
