#include "game.hpp"

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
#include "scene/player.hpp"
#include "scene/projectile.hpp"
#include "scene/weapons/gas_nozzle.hpp"
#include "scene/weapons/magic_wand.hpp"
#include "scene/weapons/orbiting_cones.hpp"
#include "scene/weapons/toxic_fumes.hpp"
#include "scene/weapons/water_bottle.hpp"
#include "scene/weapons/wood_block.hpp"
#include "utility/random.hpp"

#include <cstdlib>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>

#include <algorithm>
#include <memory>
#include <ranges>

using namespace GameEvents;

namespace {
void snapObjectToGround(MapManager &map_manager, GameObject &object) {
  const float base_offset =
      object.getPosition().y - object.getWorldAABB().min.y;
  object.setPosition(
      map_manager.snapToGround(object.getPosition(), base_offset));
}

bool isDynamicBlockingObject(const GameObject &object) {
  return object.getObjectType() == GameObjectType::ENEMY;
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
      m_shadowMap(std::make_unique<ShadowMap>()),
      m_vfxHandler(m_particleSystem, m_objects, m_mapManager, m_eventBus),
      m_state(GameState::LOADING) {
  m_camera.setPitch(-45.0f);
  m_camera.setYaw(-90.0f);
  m_camera.zoom = 60.0f;
}

Game::~Game() {}

void Game::setup() {
  // m_spawner.addWave({
  //     .timeStart = 0.0f,
  //     .timeEnd = 50.0f,
  //     .spawnLogic =
  //         [](Game &game, float current_time, float delta_time) {
  //           static AnimationState<void> cool_down(0.1f);
  //           static uint32_t enemies_spawn = 0;
  //
  //           cool_down.updateTimer(delta_time);
  //
  //           if (enemies_spawn >= 50 || !cool_down.isFinished())
  //             return;
  //
  //           glm::vec3 pos = game.m_player.ensureInitialized()->getPosition();
  //           glm::vec3 offset = {
  //               Random::randFloat(-5.0f, 5.0f),
  //               Random::randFloat(-5.0f, 5.0f),
  //               Random::randFloat(-5.0f, 5.0f),
  //           };
  //
  //           game.m_spawner.spawnEnemy(pos + offset, 3);
  //           cool_down.reset();
  //         },
  // });

  _initializeManagers();

  m_renderer.setup();
  m_particleSystem.setup();
  m_spawner.init(this);

  _resetGameplayState();
  _setupPlayer();
  _setupEnvironment();
  reset();
}

void Game::movePlayer(glm::vec3 vec, bool isRunning) {
  m_player.ensureInitialized()->setRunning(isRunning);
  m_player.ensureInitialized()->moveWithAnimation(vec);

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

  m_mapManager.foreachLoadedChunkHandles([this](const ObjectHandle &handle) {
    GameObject *object = m_objects.get(handle);
    if (!object || object->isRemovalRequested())
      return;
    object->ensureTransformUpdated();
    m_renderer.submit(&object->getModel(), object->getModelMatrix(),
                      object->getAnimator(), object->getEmissionColor());
  });

  m_mapManager.submitToRenderer(m_renderer);

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

    m_renderer.flush(shadow_draw_ctx);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glCullFace(GL_BACK); // Restore back-face culling

  // 2. Main Pass
  glViewport(0, 0, (int)m_camera.getSceneWidth(),
             (int)m_camera.getSceneHeight());
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Bind and draw Skybox first (as background)
  auto &skyboxTex = TextureManager::get(TextureName("skybox"));
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
  glBindTextureUnit(10, skyboxTex.getTexID());
  pbr_shader.setInt("u_SpecularEnvMap", 10);

  // Bind Irradiance Map for diffuse IBL
  auto &irradiance_map = TextureManager::get(TextureName("irradiance_map"));
  glBindTextureUnit(12, irradiance_map.getTexID());
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

  RenderContext forwardCtx = {
      .shader = pbr_shader,
      .camera = m_camera,
      .deltaTime = delta_time,
  };

  m_renderer.flush(forwardCtx);

  // Render particles on top of forward pass models
  m_particleSystem.render(forwardCtx);

  if (m_debugAABB) {
    RenderContext debug_ctx = {
        .shader = pbr_shader,
        .camera = m_camera,
        .deltaTime = delta_time,
    };

    m_mapManager.foreachLoadedChunkHandles(
        [this, &debug_ctx](const ObjectHandle &handle) {
          GameObject *object = m_objects.get(handle);
          if (!object || object->isRemovalRequested())
            return;
          DebugDrawer::drawAABB(debug_ctx, object->getHitboxAABB(),
                                {1.0f, 0.0f, 0.0f});
        });
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

void Game::_setupPlayer() {
  auto [player_object, player_handle] =
      m_objects.createWithHandle<Player>(GameFactories::getPlayer());

  m_player.init(&player_object);

  snapObjectToGround(m_mapManager, *m_player.ensureInitialized());
  m_mapManager.registerObject(
      player_handle, m_player.ensureInitialized()->getPosition(), false);

  auto magic_wand = std::make_shared<MagicWand>();
  magic_wand->setContext(
      [this](const GameEvents::ProjectileSpawnRequestedEvent &evt) {
        m_eventBus.emit(evt);
      });
  magic_wand->setTargetingContext(
      [this](float range, uint32_t k,
             ProjectileWeapon::EnemyCallback callback) {
        for (auto &ed : getClosestEnemies(range, k)) {
          callback(ed.enemy);
        }
      });

  magic_wand->setStats(&m_statManager);
  m_player.ensureInitialized()->addWeapon(magic_wand);

  auto wood_block = std::make_shared<SolidWoodBlock>();
  wood_block->setStats(&m_statManager);
  wood_block->setContext([this](const auto &evt) { m_eventBus.emit(evt); });
  m_player.ensureInitialized()->addWeapon(wood_block);

  // Water Bottle Weapon
  auto water_bottle = std::make_shared<WaterBottle>();
  water_bottle->setContext([this](const auto &evt) { m_eventBus.emit(evt); });
  water_bottle->setDamageContext(
      [this](const auto &evt) { m_eventBus.emit(evt); });
  water_bottle->setStats(&m_statManager);
  m_player.ensureInitialized()->addWeapon(water_bottle);

  // Orbiting Cones Weapon
  auto orbiting_cones = std::make_shared<OrbitingCones>();
  orbiting_cones->setContext([this](const auto &evt) { m_eventBus.emit(evt); });
  orbiting_cones->setDamageContext(
      [this](const auto &evt) { m_eventBus.emit(evt); });
  orbiting_cones->setTargetingContext(
      [this](float range, uint32_t k, std::function<void(Enemy *)> callback) {
        for (auto &ed : getClosestEnemies(range, k)) {
          callback(ed.enemy);
        }
      });
  orbiting_cones->setProjectileResolver(
      [this](const ObjectHandle &handle) -> Projectile * {
        GameObject *obj = m_objects.get(handle);
        if (!obj || obj->getObjectType() != GameObjectType::PLAYER_PROJECTILE) {
          return nullptr;
        }
        return static_cast<Projectile *>(obj);
      });
  orbiting_cones->setStats(&m_statManager);
  m_player.ensureInitialized()->addWeapon(orbiting_cones);

  // Toxic Fumes Weapon
  auto toxic_fumes = std::make_shared<ToxicFumes>();
  toxic_fumes->setDamageContext(
      [this](const auto &evt) { m_eventBus.emit(evt); });
  toxic_fumes->setStats(&m_statManager);
  toxic_fumes->setTargetingContext(
      [this](float range, uint32_t k, std::function<void(Enemy *)> callback) {
        for (auto &ed : getClosestEnemies(range, k)) {
          callback(ed.enemy);
        }
      });
  m_player.ensureInitialized()->addWeapon(toxic_fumes);

  // Gas Nozzle Weapon
  auto gas_nozzle = std::make_shared<GasNozzle>();
  gas_nozzle->setDamageContext(
      [this](const auto &evt) { m_eventBus.emit(evt); });
  gas_nozzle->setStats(&m_statManager);
  gas_nozzle->setTargetingContext(
      [this](float range, uint32_t k, std::function<void(Enemy *)> callback) {
        for (auto &ed : getClosestEnemies(range, k)) {
          callback(ed.enemy);
        }
      });
  m_player.ensureInitialized()->addWeapon(gas_nozzle);
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
}

void Game::reset() {
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

  _updateEnemies();

  _calculateClosestEnemies(m_player.ensureInitialized()->getPosition());

  // Update can be expensive
  m_objects.updateWhere(delta_time, [this](const GameObject &object) {
    return m_mapManager.isPositionInLoadedChunk(object.getPosition());
  });

  _syncObjectsToTerrain();

  _runCollisionPass();
  m_eventBus.flush();
  m_objects.collectGarbage();
}

void Game::_updateCamera(double delta_time) {
  (void)delta_time;
  m_cameraController.follow(m_player.ensureInitialized()->getPosition());
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

  for (GameObject *enemy_obj :
       m_objects.getObjectsWithType(GameObjectType::ENEMY)) {
    if (!enemy_obj || enemy_obj->isRemovalRequested())
      continue;

    Enemy *enemy = static_cast<Enemy *>(enemy_obj);
    if (enemy->collidesWith(*m_player.ensureInitialized())) {
      m_player.ensureInitialized()->takeDamage(enemy->getBaseDamage(), false);

      glm::vec3 offset = {Random::randFloat(-0.5f, 0.5f),
                          Random::randFloat(-0.5f, 0.5f),
                          Random::randFloat(-0.5f, 0.5f)};

      m_damageTextManager.addText(m_player.ensureInitialized()->getPosition() +
                                      offset,
                                  enemy->getBaseDamage(), false);

      m_eventBus.emit(ParticleSpawnRequestedEvent{
          .position = m_player.ensureInitialized()->getPosition() +
                      glm::vec3(0.0f, 1.0f, 0.0f),
          .effectId = ParticleEffectType::PLAYER_BLOOD});
    }
  }

  for (GameObject *proj_obj :
       m_objects.getObjectsWithType(GameObjectType::PLAYER_PROJECTILE)) {
    if (!proj_obj || proj_obj->isRemovalRequested())
      continue;

    Projectile *proj = static_cast<Projectile *>(proj_obj);

    for (GameObject *enemy_obj :
         m_objects.getObjectsWithType(GameObjectType::ENEMY)) {
      if (!enemy_obj || enemy_obj->isRemovalRequested())
        continue;

      Enemy *enemy = static_cast<Enemy *>(enemy_obj);
      if (proj->collidesWith(*enemy)) {
        glm::vec3 velocity = proj->getVelocity();
        float velocity_len = glm::length(velocity);
        glm::vec3 knockback_dir = (velocity_len < 0.001f)
                                      ? glm::vec3(0.0f, 0.0f, 1.0f)
                                      : glm::normalize(velocity);

        m_eventBus.emit(EnemyDamageRequestedEvent{
            .enemy = enemy,
            .amount = proj->getDamage(),
            .isCritical = false,
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

  std::vector<std::pair<ObjectHandle, GameObject *>> dynamic_object_entries;
  dynamic_object_entries.reserve(100);

  // Collect dynamic_objects, more effcient than collectLoadedChunkHandles
  m_mapManager.foreachLoadedChunkHandles(
      [this, &dynamic_object_entries](const ObjectHandle &handle) {
        if (!handle.isValid())
          return;

        GameObject *object = m_objects.get(handle);

        if (!object || object->isRemovalRequested() ||
            !isDynamicBlockingObject(*object))
          return;

        dynamic_object_entries.emplace_back(handle, object);
      },
      MapManager::ObjectFilter::Dynamic);

  constexpr uint8_t k_resolve_iterations = 4;

  // All dynamic_objects below should be
  // * valid pointer
  // * not requested removal
  for (uint8_t iteration = 0; iteration < k_resolve_iterations; ++iteration) {
    bool resolved_any = false;

    for (size_t i = 0; i < dynamic_object_entries.size(); ++i) {
      auto &[lhs_handle, lhs] = dynamic_object_entries[i];

      for (size_t j = i + 1; j < dynamic_object_entries.size(); ++j) {
        auto &[rhs_handle, rhs] = dynamic_object_entries[j];

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

  m_mapManager.foreachLoadedChunkHandles(
      [this, &position](const ObjectHandle &handle) {
        GameObject *object = m_objects.get(handle);

        if (!object || object->isRemovalRequested() ||
            object->getObjectType() != GameObjectType::ENEMY)
          return;

        Enemy *enemy = static_cast<Enemy *>(object);

        m_closestEnemies.emplace_back(
            enemy, glm::distance2(position, object->getPosition()));
      });

  std::ranges::sort(m_closestEnemies, {}, &EnemyDist::dist_sq);
}

void Game::_updateEnemies() {
  const glm::vec3 player_position = m_player.ensureInitialized()->getPosition();

  for (GameObject *enemy :
       m_objects.getObjectsWithType(GameObjectType::ENEMY)) {
    assert(enemy);

    static_cast<Enemy *>(enemy)->setPlayerPosition(player_position);
  }
}

void Game::_syncObjectsToTerrain() {
  for (GameObject *object : m_objects.getObjects()) {
    assert(object);

    if (!object->isRemovalRequested()) {
      if (object->getObjectType() == GameObjectType::EXP) {
        Exp *exp = static_cast<Exp *>(object);
        const float base_offset =
            exp->getPosition().y - exp->getWorldAABB().min.y;
        glm::vec3 snapped =
            m_mapManager.snapToGround(exp->getPosition(), base_offset);
        exp->setGroundY(snapped.y);
      } else if (object->getObjectType() != GameObjectType::PLAYER_PROJECTILE) {
        snapObjectToGround(m_mapManager, *object);
      }

      if (auto handle = m_objects.getHandle(*object); handle.has_value()) {
        m_mapManager.updateObjectChunk(*handle, object->getPosition());
      }

      continue;
    }

    if (auto handle = m_objects.getHandle(*object); handle.has_value()) {
      m_mapManager.unregisterObject(*handle);
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
              .effectId = ParticleEffectType::ITEM_COLLECT,
          });
        }
      });

  m_eventBus.subscribe<ExpCollectedEvent>([this](const ExpCollectedEvent &evt) {
    m_score += static_cast<int>(evt.amount);
    m_currentExp += static_cast<int>(evt.amount);

    if (m_currentExp >= m_expToNextLevel) {
      m_currentExp -= m_expToNextLevel;
      m_expToNextLevel =
          static_cast<int>(m_expToNextLevel * 1.5f); // Scale requirement
      m_currentLevel++;
      m_state = GameState::LEVEL_UP;

      // TODO: Generate 3 random upgrades
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
        evt.enemy->takeDamage(evt.amount, evt.isCritical,
                              evt.knockbackDirection, evt.knockbackStrength);

        glm::vec3 offset = {Random::randFloat(-0.5f, 0.5f),
                            Random::randFloat(-0.5f, 0.5f),
                            Random::randFloat(-0.5f, 0.5f)};

        m_damageTextManager.addText(evt.enemy->getPosition() + offset,
                                    evt.amount, evt.isCritical);

        m_eventBus.emit(ParticleSpawnRequestedEvent{.position = evt.hitPosition,
                                                    .effectId = evt.hitEffect});

        if (!wasDead && evt.enemy->isDead()) {
          m_eventBus.emit(EnemyKilledEvent{
              .enemy = evt.enemy,
              .killerPosition = m_player.ensureInitialized()->getPosition()});
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
}
