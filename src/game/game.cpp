#include "game.hpp"

#include "external/magic_enum.hpp"
#include "game/map_manager.hpp"
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
#include "scene/game_object_factory.hpp"
#include "scene/game_object_manager.hpp"
#include "scene/item.hpp"
#include "scene/player.hpp"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <utility>

namespace {
void snapObjectToGround(MapManager &map_manager, GameObject &object) {
  const float base_offset =
      object.getPosition().y - object.getWorldAABB().min.y;
  object.setPosition(
      map_manager.snapToGround(object.getPosition(), base_offset));
}

bool isDynamicBlockingObject(const GameObject &object) {
  return object.getObjectType() != GameObjectType::ITEM;
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

GameObjectFactory<Enemy> createEnemyFactory() {
  return GameObjectFactory<Enemy>::create_factory([]() {
    Enemy enemy(ModelManager::copy(ModelName::HATSUNE_MIKU));
    enemy.setScale(60.0f);
    enemy.setup();
    return enemy;
  });
}

GameObjectFactory<Item> createCoinFactory() {
  return GameObjectFactory<Item>::create_factory([]() {
    Item coin(ModelManager::copy(ModelName::COIN));
    coin.setScale(4.0f);
    return coin;
  });
}
} // namespace

Game::Game()
    : m_camera(glm::vec3(0.0f, 10.0f, 10.0f)),
      m_cameraController(m_camera, glm::vec3(0.0f, 12.0f, 10.0f)),
      m_skybox(std::make_unique<Skybox>()),
      m_shadowMap(std::make_unique<ShadowMap>()), m_state(GameState::LOADING) {
  m_camera.setPitch(-45.0f);
  m_camera.setYaw(-90.0f);
  m_camera.zoom = 45.0f;
}

Game::~Game() {
  if (m_instanceSSBO)
    glDeleteBuffers(1, &m_instanceSSBO);
  if (m_boneSSBO)
    glDeleteBuffers(1, &m_boneSSBO);
}

void Game::setup() {
  _initializeManagers();

  // GL_MAP_WRITE_BIT: Buffer memory will be mapped for writing by the CPU
  // GL_DYNAMIC_STORAGE_BIT: Buffer contents can be directly updated using
  //  glBufferSubData

  if (!m_instanceSSBO) {
    glCreateBuffers(1, &m_instanceSSBO);
    glNamedBufferStorage(m_instanceSSBO, MAX_INSTANCES * sizeof(glm::mat4),
                         nullptr, GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);
  }
  if (!m_boneSSBO) {
    glCreateBuffers(1, &m_boneSSBO);
    glNamedBufferStorage(m_boneSSBO,
                         MAX_INSTANCES * MAX_BONES * sizeof(glm::mat4), nullptr,
                         GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);
  }

  _resetGameplayState();
  _setupPlayer();
  _spawnInitialEnemies();
  _spawnInitialCoins();
  _setupEnvironment();
  reset();
}

void Game::_initializeManagers() {
  ShaderManager::ensureInit();
  ModelManager::ensureInit();
  AnimationManager::ensureInit();
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
  auto [player_object, player_handle] = m_objects.emplaceWithHandle<Player>(
      ModelManager::copy(ModelName::KASANE_TETO));

  m_player.init(&player_object);
  m_player.ensureInitialized()->setScale(20.0f);
  m_player.ensureInitialized()->setup();

  snapObjectToGround(m_mapManager, *m_player.ensureInitialized());
  m_mapManager.registerObject(
      player_handle, m_player.ensureInitialized()->getPosition(), false);
}

void Game::_spawnInitialEnemies() {
  GameObjectFactory<Enemy> enemy_factory = createEnemyFactory();

  for (size_t i = 0; i < 5; ++i) {
    Enemy enemy_clone = enemy_factory.create([this](Enemy &enemy) {
      enemy.move({Random::randFloat(-20.0f, 20.0f), 0.0f,
                  Random::randFloat(-20.0f, 20.0f)});
      snapObjectToGround(m_mapManager, enemy);
    });

    auto [enemy, enemy_handle] =
        m_objects.emplaceWithHandle<Enemy>(std::move(enemy_clone));

    m_mapManager.registerObject(enemy_handle, enemy.getPosition(), false);
  }
}

void Game::_spawnInitialCoins() {
  GameObjectFactory<Item> coin_factory = createCoinFactory();

  for (size_t i = 0; i < 4; ++i) {
    Item coin_clone = coin_factory.create([this](Item &coin) {
      coin.translate({Random::randFloat(-10.0f, 20.0f), 0.8f,
                      Random::randFloat(-10.0f, 20.0f)});
      snapObjectToGround(m_mapManager, coin);
    });

    auto [coin, coin_handle] = m_objects.emplaceWithHandle<Item>(coin_clone);

    m_mapManager.registerObject(coin_handle, coin.getPosition(), true);
  }
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

  m_mapManager.update(m_player.ensureInitialized()->getPosition());

  _updateEnemies();

  // Update can be expensive
  m_objects.updateWhere(delta_time, [this](const GameObject &object) {
    return m_mapManager.isPositionInLoadedChunk(object.getPosition());
  });

  _syncObjectsToTerrain();

  _runCollisionPass();
  m_eventBus.flush();
  m_objects.collectGarbage();
}

void Game::movePlayer(glm::vec3 vec) {
  m_player.ensureInitialized()->moveWithAnimation(vec);

  GameObject *player_object = m_player.ensureInitialized();
  snapObjectToGround(m_mapManager, *player_object);

  if (auto handle = m_objects.getHandle(*player_object); handle.has_value()) {
    m_mapManager.updateObjectChunk(*handle, player_object->getPosition());
  }
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

    _drawLoadedObjects(shadow_draw_ctx);

    glm::mat4 identity(1.0f);
    glNamedBufferSubData(m_instanceSSBO, 0, sizeof(glm::mat4), &identity);
    shadow_draw_ctx.shader.setBool("u_HasAnimation", false);

    m_mapManager.draw(shadow_draw_ctx);
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

  _drawLoadedObjects(ctx);

  glm::mat4 identity(1.0f);
  glNamedBufferSubData(m_instanceSSBO, 0, sizeof(glm::mat4), &identity);
  ctx.shader.setBool("u_HasAnimation", false);

  m_mapManager.draw(ctx);

  if (false && m_debugAABB) {
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
      snapObjectToGround(m_mapManager, *object);

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

void Game::_drawLoadedObjects(const RenderContext &ctx) {
  std::unordered_map<const Model *, std::vector<GameObject *>> modelBatches;

  m_mapManager.foreachLoadedChunkHandles(
      [this, &modelBatches](const ObjectHandle &handle) {
        GameObject *object = m_objects.get(handle);
        if (!object || object->isRemovalRequested())
          return;

        object->ensureTransformUpdated();
        modelBatches[&object->getModel()].push_back(object);
      });

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_instanceSSBO);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_boneSSBO);

  std::vector<glm::mat4> instanceData;
  std::vector<glm::mat4> boneData;
  instanceData.reserve(MAX_INSTANCES);
  boneData.reserve(MAX_INSTANCES * MAX_BONES);

  for (auto &[model, objects] : modelBatches) {
    instanceData.clear();
    boneData.clear();
    bool hasAnimation = false;

    for (GameObject *obj : objects) {
      if (instanceData.size() >= MAX_INSTANCES)
        break;

      instanceData.push_back(obj->getModelMatrix());

      const Animator *animator = obj->getAnimator();
      if (animator) {
        hasAnimation = true;
        const auto &bones = animator->getFinalBoneMatrices();
        size_t boneCount = std::min(bones.size(), MAX_BONES);
        for (size_t i = 0; i < boneCount; ++i) {
          boneData.push_back(bones[i]);
        }
        for (size_t i = boneCount; i < MAX_BONES; ++i) {
          boneData.push_back(glm::mat4(1.0f));
        }
      }
    }

    if (instanceData.empty())
      continue;

    glNamedBufferSubData(m_instanceSSBO, 0,
                         instanceData.size() * sizeof(glm::mat4),
                         instanceData.data());

    if (hasAnimation) {
      glNamedBufferSubData(m_boneSSBO, 0, boneData.size() * sizeof(glm::mat4),
                           boneData.data());
    }

    ctx.shader.setBool("u_HasAnimation", hasAnimation);
    const_cast<Model *>(model)->drawInstanced(ctx, instanceData.size());
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
        // TODO: Hook this event into particle or VFX rendering once
        // available.
      });
}
