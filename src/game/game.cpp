#include "game.hpp"

#include "game/game_events.hpp"
#include "game/map_manager.hpp"
#include "graphics/debug_drawer.hpp"
#include "graphics/ibl_generator.hpp"
#include "graphics/render_context.hpp"
#include "graphics/shader.hpp"
#include "resource/animation_manager.hpp"
#include "resource/lighting_manager.hpp"
#include "resource/model_manager.hpp"
#include "resource/shader_manager.hpp"
#include "resource/texture_manager.hpp"
#include "scene/enemy/enemy.hpp"
#include "scene/game_factories.hpp"
#include "scene/game_object.hpp"
#include "scene/game_object_manager.hpp"
#include "scene/item.hpp"
#include "scene/map_population_system.hpp"
#include "scene/player.hpp"
#include "scene/projectile.hpp"
#include "scene/weapons/water_bottle.hpp"
#include "utility/random.hpp"
#include "utility/utility.hpp"

#include <cassert>
#include <cstdlib>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>

#include <algorithm>
#include <memory>
#include <ranges>

using namespace GameEvents;
using Random = Utility::Random;

namespace {
void snapObjectToGround(MapManager &map_manager, GameObject &object) {
  const float base_offset =
      object.getPosition().y - object.getWorldAABB().min.y;
  object.setPosition(
      map_manager.snapToGround(object.getPosition(), base_offset));
}

bool isDynamicBlockingObject(const GameObject &object) {
  return object.isEnemy();
}

bool resolveDynamicOverlap(MapManager &map_manager, GameObject &lhs,
                           GameObject &rhs) {
  const AABB lhs_box = lhs.getHitboxAABB();
  const AABB rhs_box = rhs.getHitboxAABB();

  if (!lhs_box.intersects(rhs_box)) {
    return false;
  }

  const float overlap_x = std::min(lhs_box.max.x, rhs_box.max.x) -
                          std::max(lhs_box.min.x, rhs_box.min.x);
  const float overlap_z = std::min(lhs_box.max.z, rhs_box.max.z) -
                          std::max(lhs_box.min.z, rhs_box.min.z);

  if (overlap_x <= 0.0f || overlap_z <= 0.0f) {
    return false;
  }

  const glm::vec3 lhs_center = lhs_box.getCenter();
  const glm::vec3 rhs_center = rhs_box.getCenter();
  constexpr float k_separation_epsilon = 0.001f;

  if (overlap_x < overlap_z) {
    const float shift = overlap_x * 0.5f + k_separation_epsilon;

    if (lhs_center.x <= rhs_center.x) {
      lhs.translate({-shift, 0.0f, 0.0f});
      rhs.translate({shift, 0.0f, 0.0f});
    } else {
      lhs.translate({shift, 0.0f, 0.0f});
      rhs.translate({-shift, 0.0f, 0.0f});
    }
  } else {
    const float shift = overlap_z * 0.5f + k_separation_epsilon;

    if (lhs_center.z <= rhs_center.z) {
      lhs.translate({0.0f, 0.0f, -shift});
      rhs.translate({0.0f, 0.0f, shift});
    } else {
      lhs.translate({0.0f, 0.0f, shift});
      rhs.translate({0.0f, 0.0f, -shift});
    }
  }

  snapObjectToGround(map_manager, lhs);
  snapObjectToGround(map_manager, rhs);

  return true;
}
} // namespace

Game::Game()
    : m_camera(glm::vec3(0.0f, 10.0f, 10.0f)),
      m_cameraController(m_camera, glm::vec3(0.0f, 12.0f, 10.0f)),
      m_skybox(std::make_unique<Skybox>()),
      m_shadowMap(std::make_unique<ShadowMap>(2048, 2048)),
      m_vfxHandler(m_particleSystem, m_objects, m_mapManager, m_eventBus),
      m_state(GameState::LOADING) {
  m_camera.setPitch(-45.0f);
  m_camera.setYaw(-90.0f);
  m_camera.zoom = 60.0f;
}

Game::~Game() {}

void setupWaves(EnemySpawner &spawner) {
  // Wave 1 (0-60s): Humanoids every 1.5s.
  spawner.addWave({
      .timeStart = 0.0f,
      .timeEnd = 60.0f,
      .spawnLogic =
          [timer = 0.0f](Game &game, float current_time,
                         float delta_time) mutable {
            (void)current_time;
            timer += delta_time;
            if (timer >= 1.5f) {
              timer = 0.0f;
              glm::vec3 pos = game.getPlayer()->getPosition() +
                              Random::randVec3Circle(25.0f);
              game.getSpawner().spawnSpecificEnemy(GameObjectType::ENEMY, pos);
            }
          },
  });

  // Wave 2 (60-120s): Humanoids every 0.6s with linear health scaling.
  // Introduce WeakCarEnemy every 15s.
  spawner.addWave({
      .timeStart = 60.0f,
      .timeEnd = 120.0f,
      .spawnLogic =
          [h_timer = 0.0f, c_timer = 0.0f](Game &game, float current_time,
                                           float delta_time) mutable {
            h_timer += delta_time;
            c_timer += delta_time;

            if (h_timer >= 0.6f) {
              h_timer = 0.0f;
              glm::vec3 pos = game.getPlayer()->getPosition() +
                              Random::randVec3Circle(25.0f);

              float health_scale = 1.0f + (current_time - 60.0f) / 60.0f;
              game.getSpawner().spawnSpecificEnemy(GameObjectType::ENEMY, pos,
                                                   health_scale);
            }

            if (c_timer >= 15.0f) {
              c_timer = 0.0f;
              glm::vec3 pos = game.getPlayer()->getPosition() +
                              Random::randVec3Circle(25.0f);

              game.getSpawner().spawnSpecificEnemy(
                  GameObjectType::WEAK_CAR_ENEMY, pos);
            }
          },
  });

  // Wave 3 (120-180s): Humanoids every 0.8s. StandardCarEnemy every 10s.
  spawner.addWave({
      .timeStart = 120.0f,
      .timeEnd = 180.0f,
      .spawnLogic =
          [h_timer = 0.0f, c_timer = 0.0f](Game &game, float current_time,
                                           float delta_time) mutable {
            (void)current_time;
            h_timer += delta_time;
            c_timer += delta_time;

            if (h_timer >= 0.8f) {
              h_timer = 0.0f;
              glm::vec3 pos = game.getPlayer()->getPosition() +
                              Random::randVec3Circle(25.0f);

              game.getSpawner().spawnSpecificEnemy(GameObjectType::ENEMY, pos);
            }

            if (c_timer >= 10.0f) {
              c_timer = 0.0f;
              glm::vec3 pos = game.getPlayer()->getPosition() +
                              Random::randVec3Circle(25.0f);

              game.getSpawner().spawnSpecificEnemy(
                  GameObjectType::STANDARD_CAR_ENEMY, pos);
            }
          },
  });

  // Wave 4 (180-240s): Humanoids spawned in circles of 15 every 15s.
  // ArmoredCarEnemy every 12s.
  spawner.addWave({
      .timeStart = 180.0f,
      .timeEnd = 240.0f,
      .spawnLogic =
          [h_timer = 0.0f, c_timer = 0.0f](Game &game, float current_time,
                                           float delta_time) mutable {
            (void)current_time;
            h_timer += delta_time;
            c_timer += delta_time;

            if (h_timer >= 15.0f) {
              h_timer = 0.0f;
              for (int i = 0; i < 15; ++i) {
                glm::vec3 pos = game.getPlayer()->getPosition() +
                                Random::randVec3Circle(25.0f);

                game.getSpawner().spawnSpecificEnemy(GameObjectType::ENEMY,
                                                     pos);
              }
            }

            if (c_timer >= 12.0f) {
              c_timer = 0.0f;
              glm::vec3 pos = game.getPlayer()->getPosition() +
                              Random::randVec3Circle(25.0f);

              game.getSpawner().spawnSpecificEnemy(
                  GameObjectType::ARMORED_CAR_ENEMY, pos);
            }
          },
  });

  // Wave 5 (240s+): Spawn one BossCarEnemy immediately, maintain heavy
  // humanoid spawns.
  spawner.addWave({
      .timeStart = 240.0f,
      .timeEnd = 999999.0f,
      .spawnLogic =
          [boss_spawned = false, h_timer = 0.0f](Game &game, float current_time,
                                                 float delta_time) mutable {
            (void)current_time;
            if (!boss_spawned) {
              boss_spawned = true;
              glm::vec3 pos = game.getPlayer()->getPosition() +
                              Random::randVec3Circle(30.0f);

              game.getSpawner().spawnSpecificEnemy(
                  GameObjectType::BOSS_CAR_ENEMY, pos);
            }

            h_timer += delta_time;
            if (h_timer >= 0.5f) {
              h_timer = 0.0f;
              glm::vec3 pos = game.getPlayer()->getPosition() +
                              Random::randVec3Circle(25.0f);

              game.getSpawner().spawnSpecificEnemy(GameObjectType::ENEMY, pos);
            }
          },
  });
}

void Game::setup() {
  _initializeManagers();

  m_renderer.setup();
  m_particleSystem.setup();
  m_spawner.init(this);

  setupWaves(m_spawner);

  _resetGameplayState();
  _setupPlayer();
  _setupEnvironment();
  _populateMap();

  reset();
}

void Game::movePlayer(glm::vec3 vec, bool isRunning) {
  m_player.ensureInitialized()->setRunning(isRunning);
  m_player.ensureInitialized()->moveWithAnimation(
      vec, m_statManager.getMultiplier(StatType::SPEED));

  GameObject *player_object = m_player.ensureInitialized();
  snapObjectToGround(m_mapManager, *player_object);

  if (auto handle = m_objects.getHandle(*player_object); handle.has_value()) {
    m_mapManager.updateObjectChunk(*handle, player_object->getPosition());
  }
}

void Game::render(double delta_time) {
  glEnable(GL_DEPTH_TEST);

  m_renderer.beginFrame();

  m_particleSystem.update(delta_time);

  // Push loaded object to be rendered
  Utility::concat(
      [this](const LoadedChunkObject &val) {
        GameObject *object = val.object;
        assert(object);

        if (object->isRemovalRequested())
          return;

        object->ensureTransformUpdated();
        m_renderer.submit(&object->getModel(), object->getModelMatrix(),
                          object->getAnimator(), object->getEmissionColor());
      },
      m_currentChunkObjects.dynamics, m_currentChunkObjects.statics);

  m_mapManager.submitToRenderer(m_renderer);

  m_shadowMap->updateLightSpaceMatrix(
      m_player.ensureInitialized()->getPosition());

  Shader &shadow_shader = ShaderManager::get(ShaderType::SHADOW);
  shadow_shader.use();
  shadow_shader.setMat4("u_LightSpaceMatrix",
                        m_shadowMap->getLightSpaceMatrix());

  m_shadowMap->bindForWriting();
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  {
    RenderContext shadow_draw_ctx = {
        .shader = shadow_shader,
        .camera = m_camera,
        .deltaTime = delta_time,
    };

    m_renderer.flush(shadow_draw_ctx);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glCullFace(GL_BACK);

  glViewport(0, 0, static_cast<int>(m_camera.getSceneWidth()),
             static_cast<int>(m_camera.getSceneHeight()));
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  Texture &skybox_tex = TextureManager::get(TextureName("skybox"));
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

  glBindTextureUnit(10, skybox_tex.getTexID());
  pbr_shader.setInt("u_SpecularEnvMap", 10);

  Texture &irradiance_map = TextureManager::get(TextureName("irradiance_map"));
  glBindTextureUnit(12, irradiance_map.getTexID());
  pbr_shader.setInt("u_IrradianceMap", 12);

  m_shadowMap->bindForReading(11);
  pbr_shader.setInt("u_ShadowMap", 11);

  LightingManager::apply(pbr_shader);

  pbr_shader.setFloat("u_HeightScale", 0.0f);
  pbr_shader.setFloat("u_AOFactor", 1.0f);
  pbr_shader.setFloat("u_AmbientIntensity", 1.0f);
  pbr_shader.setVec3("u_BaseColor", glm::vec3(1.0f));
  pbr_shader.setVec2("u_UVOffset", glm::vec2(0.0f));

  RenderContext forwardCtx = {
      .shader = pbr_shader,
      .camera = m_camera,
      .deltaTime = delta_time,
  };

  m_renderer.flush(forwardCtx);

  m_particleSystem.render(forwardCtx);

  if (m_debugAABB) {
    RenderContext debug_ctx = {
        .shader = pbr_shader,
        .camera = m_camera,
        .deltaTime = delta_time,
    };

    Utility::concat(
        [debug_ctx](const LoadedChunkObject &val) {
          GameObject *object = val.object;
          assert(object);

          if (object->isRemovalRequested())
            return;

          DebugDrawer::drawAABB(debug_ctx, object->getHitboxAABB(),
                                {1.0f, 0.0f, 0.0f});
        },
        m_currentChunkObjects.dynamics, m_currentChunkObjects.statics);
  }

  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
}

auto Game::getClosestEnemies(float radius, uint32_t top_k) const {
  return m_closestEnemies |
         std::views::take_while([radius](const EnemyDist &dist) {
           return dist.dist_sq < (radius * radius);
         }) |
         std::views::take(top_k);
}

void Game::_initializeManagers() {
  ShaderManager::ensureInit();
  ModelManager::ensureInit();
  AnimationManager::ensureInit();
  GameFactories::init();

  m_mapManager.setup();
}

void Game::_resetGameplayState() {
  m_mapManager.clearObjectTracking();

  m_objects.clear();
  m_eventBus.clear();
  _registerGameplayEventHandlers();

  m_score = 0;
}

glm::vec3 Game::getPlayerPosition() const {
  Player *player = m_player.ensureInitialized();
  AABB box = player->getHitboxAABB();
  return player->getPosition() +
         glm::vec3(0.0f, (box.max.y - box.min.y) * 0.5f, 0.0f);
}

float Game::getGroundLevel(glm::vec3 pos) const {
  return m_mapManager.sampleHeight(pos.x, pos.z);
}

glm::vec3 Game::getPlayerForward() const {
  Player *player = m_player.ensureInitialized();
  float rad = glm::radians(player->getRotation().y);
  return glm::vec3(std::sin(rad), 0.0f, std::cos(rad));
}

void Game::findTargets(float range, uint32_t k, EnemyCallback callback) const {
  for (auto &ed : getClosestEnemies(range, k)) {
    callback(ed.enemy);
  }
}

void Game::_setupPlayer() {
  auto [player_object, player_handle] =
      m_objects.createWithHandle<Player>(GameFactories::getPlayer());

  m_player.init(&player_object);

  snapObjectToGround(m_mapManager, *m_player.ensureInitialized());
  m_mapManager.registerObject(
      player_handle, m_player.ensureInitialized()->getPosition(), false);

  // Initial weapon
  auto water_bottle = std::make_shared<WaterBottle>();
  water_bottle->setContext(this);
  m_player.ensureInitialized()->addWeapon(water_bottle);
}

void Game::_setupEnvironment() {
  // Generate Irradiance Map
  std::shared_ptr<Texture> skybox_tex =
      TextureManager::copy(TextureName("skybox"));
  m_skybox->setTexture(skybox_tex);

  Shader &irradiance_shader = ShaderManager::get(ShaderType::IRRADIANCE);
  std::shared_ptr<Texture> irradiance_map = IBLGenerator::generateIrradianceMap(
      *skybox_tex, *m_skybox, irradiance_shader);
  TextureManager::manage(TextureName("irradiance_map"),
                         std::move(*irradiance_map));

  LightingManager::clear();

  // Sun Light (Directional, casts shadows)
  LightingManager::add({.type = LightType::DIRECTIONAL,
                        .position = glm::vec3(0.3f, -1.0f, 0.1f),
                        .color = glm::vec3(11.0f, 9.0f, 8.0f) * .8f,
                        .castsShadows = true});

  // Sky Blue Fill Light (Directional)
  LightingManager::add({.type = LightType::DIRECTIONAL,
                        .position = glm::vec3(0.5f, -1.0f, 0.2f),
                        .color = glm::vec3(0.1f, 0.15f, 0.25f)});

  // Ground Bounce Fill (Point light)
  LightingManager::add({.type = LightType::POINT,
                        .position = glm::vec3(0.0f, -5.0f, 0.0f),
                        .color = glm::vec3(0.3f, 0.2f, 0.1f) * 5.0f});
}

void Game::_populateMap() {
  MapPopulator populationSystem;
  populationSystem.populateMap(m_objects, m_mapManager);
}

void Game::reset() {
  _resetGameplayState();
  _setupPlayer();
  _populateMap();

  m_gameTime = 0.0f;
  m_currentExp = 0;
  m_expToNextLevel = 10;
  m_currentLevel = 1;
  m_pendingLevelUps = 0;
  m_score = 0;

  m_state = GameState::START_MENU;
  m_cameraController.setTarget(glm::vec3(0.0f, 0.0f, 0.0f), true);
}

void Game::startGame() {
  if (m_state == GameState::START_MENU) {
    m_state = GameState::PLAYING;
    m_gameTime = 0.0f;
  }
}

void Game::update(double delta_time) {
  // Pause gameplay simulation while the player is picking upgrades.
  if (m_state == GameState::LEVEL_UP) {
    return;
  }

  if (m_state == GameState::PLAYING) {
    m_gameTime += static_cast<float>(delta_time);
    m_spawner.update(m_gameTime, static_cast<float>(delta_time));
  }

  m_damageTextManager.update(static_cast<float>(delta_time));

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

  m_mapManager.update(m_player.ensureInitialized()->getPosition());

  _updateCurrentChunkObjects();
  _calculateClosestEnemies(m_player.ensureInitialized()->getPosition());

  // Update Objects
  Utility::concat(
      [delta_time](LoadedChunkObject &val) { val.object->update(delta_time); },
      m_currentChunkObjects.dynamics, m_currentChunkObjects.statics);

  _syncObjectsToTerrain();
  _runCollisionPass();
  m_eventBus.flush();

  if (m_state == GameState::PLAYING) {
    _updatePlayerRegen(delta_time);
  }

  m_objects.collectGarbage();
}

void Game::_updateCamera(double delta_time) {
  (void)delta_time;
  m_cameraController.follow(m_player.ensureInitialized()->getPosition());
}

void Game::_updatePlayerRegen(double delta_time) {
  Player &player = *m_player.ensureInitialized();

  if (player.isDead())
    return;

  const float regen_per_second =
      m_statManager.getMultiplier(StatType::HEALTH_REGEN);

  if (regen_per_second <= 0.0f)
    return;

  player.heal(regen_per_second * static_cast<float>(delta_time));
}

void Game::_runCollisionPass() {
  for (GameObject *item_obj :
       m_objects.getObjectsWithType(GameObjectType::ITEM)) {
    if (!item_obj || item_obj->isRemovalRequested())
      continue;

    Item *item = static_cast<Item *>(item_obj);
    if (m_player.ensureInitialized()->collidesWith(*item)) {
      m_eventBus.emit(ItemCollectedEvent{
          .player = m_player.ensureInitialized(),
          .item = item,
          .value = 1,
      });
    }
  }

  for (GameObject *exp_obj :
       m_objects.getObjectsWithType(GameObjectType::EXP)) {
    if (!exp_obj || exp_obj->isRemovalRequested())
      continue;

    Exp *exp = static_cast<Exp *>(exp_obj);
    if (m_player.ensureInitialized()->collidesWith(*exp)) {
      m_eventBus.emit(ExpCollectedEvent{
          .player = m_player.ensureInitialized(),
          .exp = exp,
          .amount = exp->getAmount(),
      });
    }
  }

  for (auto &[_, object] : m_currentChunkObjects.dynamics) {
    if (object->isRemovalRequested() || !object->isEnemy())
      continue;

    Enemy *enemy = static_cast<Enemy *>(object);
    if (enemy->collidesWith(*m_player.ensureInitialized())) {
      m_eventBus.emit(PlayerDamageRequestedEvent{
          .enemy = enemy,
          .amount = enemy->getBaseDamage(),
          .isCritical = false,
          .knockbackDirection =
              glm::normalize(m_player.ensureInitialized()->getPosition() -
                             enemy->getPosition()),
          .knockbackStrength = 1.0f,
          .hitPosition = m_player.ensureInitialized()->getPosition(),
          .hitEffect = ParticleEffectType::PLAYER_BLOOD});
    }
  }

  for (GameObject *proj_obj :
       m_objects.getObjectsWithType(GameObjectType::PLAYER_PROJECTILE)) {
    if (!proj_obj || proj_obj->isRemovalRequested())
      continue;

    Projectile *proj = static_cast<Projectile *>(proj_obj);

    for (auto &[_, object] : m_currentChunkObjects.dynamics) {
      if (object->isRemovalRequested() || !object->isEnemy())
        continue;

      Enemy *enemy = static_cast<Enemy *>(object);
      if (proj->collidesWith(*enemy)) {
        glm::vec3 velocity = proj->getVelocity();
        float velocity_len = glm::length(velocity);
        glm::vec3 knockback_dir = (velocity_len < 0.001f)
                                      ? glm::vec3(0.0f, 0.0f, 1.0f)
                                      : glm::normalize(velocity);

        m_eventBus.emit(EnemyDamageRequestedEvent{
            .enemy = enemy,
            .amount = proj->getDamage(),
            .isCritical = proj->isCritical(),
            .knockbackDirection = knockback_dir,
            .knockbackStrength = 2.0f,
            .hitPosition = enemy->getPosition() + glm::vec3(0.0f, 1.0f, 0.0f),
            .hitEffect = ParticleEffectType::MAGIC_HIT,
        });

        proj->requestRemoval();
        break;
      }
    }
  }
  auto &dynamic_object_entries = m_currentChunkObjects.dynamics;

  constexpr uint8_t k_resolve_iterations = 4;

  // All dynamic_objects below should be
  // * valid pointer
  // * not requested removal
  for (uint8_t iteration = 0; iteration < k_resolve_iterations; ++iteration) {
    bool resolved_any = false;

    for (size_t i = 0; i < dynamic_object_entries.size(); ++i) {
      auto &[lhs_handle, lhs] = dynamic_object_entries[i];

      if (!isDynamicBlockingObject(*lhs))
        continue;

      for (size_t j = i + 1; j < dynamic_object_entries.size(); ++j) {
        auto &[rhs_handle, rhs] = dynamic_object_entries[j];

        if (!isDynamicBlockingObject(*rhs))
          continue;

        if (!lhs->collidesWith(*rhs))
          continue;

        if (!resolveDynamicOverlap(m_mapManager, *lhs, *rhs))
          continue;

        resolved_any = true;

        m_mapManager.updateObjectChunk(lhs_handle, lhs->getPosition());
        m_mapManager.updateObjectChunk(rhs_handle, rhs->getPosition());
      }
    }

    if (!resolved_any)
      break;
  }
}

void Game::_calculateClosestEnemies(glm::vec3 position) {
  m_closestEnemies.clear();

  for (const auto &[handle, object] : m_currentChunkObjects.dynamics) {
    assert(object);

    if (object->isRemovalRequested() || !object->isEnemy())
      continue;

    Enemy *enemy = static_cast<Enemy *>(object);

    m_closestEnemies.emplace_back(
        enemy, glm::distance2(position, object->getHitboxAABB().getClosestPoint(
                                            position)));
  }

  std::ranges::sort(m_closestEnemies, {}, &EnemyDist::dist_sq);
}

void Game::_updateCurrentChunkObjects() {
  m_currentChunkObjects.dynamics.clear();
  m_currentChunkObjects.statics.clear();

  m_mapManager.foreachLoadedChunkHandles(
      [this](const ObjectHandle &handle) {
        if (!handle.isValid())
          return;

        GameObject *object = m_objects.get(handle);
        if (!object || object->isRemovalRequested())
          return;

        m_currentChunkObjects.dynamics.emplace_back(handle, object);
      },
      MapManager::ObjectFilter::Dynamic);

  m_mapManager.foreachLoadedChunkHandles(
      [this](const ObjectHandle &handle) {
        if (!handle.isValid())
          return;

        GameObject *object = m_objects.get(handle);
        if (!object || object->isRemovalRequested())
          return;

        m_currentChunkObjects.statics.emplace_back(handle, object);
      },
      MapManager::ObjectFilter::Static);
}

void Game::_syncObjectsToTerrain() {
  snapObjectToGround(m_mapManager, *m_player.ensureInitialized());

  auto snap_objects_to_terrain = [this](LoadedChunkObject &val) {
    GameObject *object = val.object;
    ObjectHandle handle = val.handle;

    if (object->getObjectType() == GameObjectType::STATIC_PROP)
      return;

    if (object->isRemovalRequested()) {
      m_mapManager.unregisterObject(handle);
      return;
    }

    if (object->getObjectType() == GameObjectType::EXP) {
      Exp *exp = static_cast<Exp *>(object);
      const float base_offset =
          exp->getPosition().y - exp->getWorldAABB().min.y;
      glm::vec3 snapped =
          m_mapManager.snapToGround(exp->getPosition(), base_offset);
      exp->setGroundY(snapped.y);
    }

    else if (object->getObjectType() != GameObjectType::PLAYER_PROJECTILE) {
      snapObjectToGround(m_mapManager, *object);
    }

    m_mapManager.updateObjectChunk(handle, object->getPosition());
  };

  Utility::concat(snap_objects_to_terrain, m_currentChunkObjects.dynamics,
                  m_currentChunkObjects.statics);
}

void Game::_registerGameplayEventHandlers() {
  m_eventBus.subscribe<ItemCollectedEvent>(
      [this](const ItemCollectedEvent &evt) {
        m_score += evt.value;
        m_eventBus.emit(DespawnRequestedEvent{.object = evt.item});

        if (evt.item) {
          m_eventBus.emit(ParticleSpawnRequestedEvent{
              .position = evt.item->getPosition(),
              .effectId = ParticleEffectType::ITEM_COLLECT,
          });
        }
      });

  m_eventBus.subscribe<ExpCollectedEvent>([this](const ExpCollectedEvent &evt) {
    m_score += static_cast<int>(evt.amount);
    m_currentExp += static_cast<int>(evt.amount);

    while (m_currentExp >= m_expToNextLevel) {
      m_currentExp -= m_expToNextLevel;
      m_expToNextLevel =
          static_cast<int>(m_expToNextLevel * 1.5f); // Scale requirement
      m_currentLevel++;
      m_pendingLevelUps++;
    }

    if (m_pendingLevelUps > 0 && m_state != GameState::LEVEL_UP) {
      m_state = GameState::LEVEL_UP;
      m_levelUpCandidates = UpgradeGenerator::generateUpgrades(*this, 3);
      m_levelUpSelection = 0;
    }

    m_eventBus.emit(DespawnRequestedEvent{.object = evt.exp});

    if (evt.exp) {
      m_eventBus.emit(ParticleSpawnRequestedEvent{
          .position = evt.exp->getPosition(),
          .effectId = ParticleEffectType::EXP_COLLECT,
      });
    }
  });

  m_eventBus.subscribe<DespawnRequestedEvent>(
      [](const DespawnRequestedEvent &evt) {
        if (evt.object) {
          evt.object->requestRemoval();
        }
      });

  m_eventBus.subscribe<EnemyDamageRequestedEvent>(
      [this](const EnemyDamageRequestedEvent &evt) {
        if (!evt.enemy || evt.enemy->isRemovalRequested())
          return;

        bool wasDead = evt.enemy->isDead();

        bool isCritical = evt.isCritical;
        float amount = evt.amount;
        Player *player = m_player.ensureInitialized();
        if (Random::randFloat(0.0f, 1.0f) <= player->getCritProbability()) {
          isCritical = true;
          amount *= player->getCritMultiplier();
        }

        evt.enemy->takeDamage(amount, isCritical, evt.knockbackDirection,
                              evt.knockbackStrength);

        glm::vec3 offset = {Random::randFloat(-0.5f, 0.5f),
                            Random::randFloat(-0.5f, 0.5f),
                            Random::randFloat(-0.5f, 0.5f)};

        m_damageTextManager.addText(evt.enemy->getPosition() + offset, amount,
                                    isCritical);

        m_eventBus.emit(ParticleSpawnRequestedEvent{.position = evt.hitPosition,
                                                    .effectId = evt.hitEffect});

        if (!wasDead && evt.enemy->isDead()) {
          m_eventBus.emit(EnemyKilledEvent{
              .enemy = evt.enemy,
              .killerPosition = m_player.ensureInitialized()->getPosition()});
        }
      });

  m_eventBus.subscribe<PlayerDamageRequestedEvent>(
      [this](const PlayerDamageRequestedEvent &evt) {
        Player &player = *m_player.ensureInitialized();

        bool wasDead = player.isDead();

        player.takeDamage(evt.amount, evt.isCritical);

        glm::vec3 offset = {Random::randFloat(-0.5f, 0.5f),
                            Random::randFloat(-0.5f, 0.5f),
                            Random::randFloat(-0.5f, 0.5f)};

        m_damageTextManager.addText(
            m_player.ensureInitialized()->getPosition() + offset, evt.amount,
            false, true);

        m_eventBus.emit(ParticleSpawnRequestedEvent{
            .position = m_player.ensureInitialized()->getPosition() +
                        glm::vec3(0.0f, 1.0f, 0.0f),
            .effectId = ParticleEffectType::PLAYER_BLOOD});

        if (!wasDead && player.isDead()) {
          m_eventBus.emit(PlayerKilledEvent{
              .enemy = evt.enemy, .killerPosition = evt.enemy->getPosition()});
        }
      });

  m_eventBus.subscribe<ProjectileSpawnRequestedEvent>(
      [this](const ProjectileSpawnRequestedEvent &evt) {
        auto [object, handle] =
            m_objects.emplaceWithHandle<Projectile>(*evt.projectile);
        m_mapManager.registerObject(handle, object.getPosition(), false);

        if (evt.onSpawned) {
          evt.onSpawned(handle);
        }
      });

  m_eventBus.subscribe<ParticleSpawnRequestedEvent>(
      [this](const ParticleSpawnRequestedEvent &evt) {
        m_vfxHandler.handleParticleSpawn(evt);
      });

  m_eventBus.subscribe<EnemyKilledEvent>([this](const EnemyKilledEvent &evt) {
    m_vfxHandler.handleEnemyKilled(evt);
  });

  m_eventBus.subscribe<PlayerKilledEvent>([this](const PlayerKilledEvent &evt) {
    (void)evt;
    m_state = GameState::GAME_OVER;
  });
}
