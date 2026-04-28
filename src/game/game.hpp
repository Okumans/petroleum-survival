#pragma once

#include "game/game_events.hpp"
#include "game/stat_manager.hpp"
#include "game/vfx_handler.hpp"
#include "graphics/camera.hpp"
#include "graphics/camera_controller.hpp"
#include "graphics/particle_system.hpp"
#include "graphics/renderer.hpp"
#include "graphics/shadow_map.hpp"
#include "graphics/skybox.hpp"
#include "map_manager.hpp"
#include "scene/enemy/enemy.hpp"
#include "scene/game_object_manager.hpp"
#include "scene/player.hpp"
#include "ui/damage_text_manager.hpp"
#include "utility/event_bus.hpp"
#include "utility/not_initialized.hpp"

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <memory>

#ifndef SHADER_PATH
#define SHADER_PATH ASSETS_PATH "/shaders"
#endif

#ifndef ICONS_PATH
#define ICONS_PATH ASSETS_PATH "/icons"
#endif

#include "game/enemy_spawner.hpp"
#include "scene/weapon/i_weapon_context.hpp"

enum class GameState { LOADING, START_MENU, PLAYING, LEVEL_UP, GAME_OVER };

class Game : public IWeaponContext {
public:
  struct EnemyDist {
    Enemy *enemy;
    float dist_sq;
  };

private:
  Camera m_camera;
  CameraController m_cameraController;
  MapManager m_mapManager;
  std::unique_ptr<Skybox> m_skybox;
  std::unique_ptr<ShadowMap> m_shadowMap;
  GameObjectManager m_objects;
  EventBus m_eventBus;

  ParticleSystem m_particleSystem;
  VFXHandler m_vfxHandler;
  EnemySpawner m_spawner;
  StatManager m_statManager;
  DamageTextManager m_damageTextManager;

  float m_gameTime = 0.0f;

  int m_score = 0;
  int m_currentExp = 0;
  int m_expToNextLevel = 10;
  int m_currentLevel = 1;

  NotInitialized<Player *, "Player"> m_player;

  bool m_debugAABB = true;
  GameState m_state = GameState::START_MENU;

  Renderer m_renderer;

  std::vector<EnemyDist> m_closestEnemies;

public:
  Game();
  ~Game();

  GameObjectManager &getObjects() { return m_objects; }
  MapManager &getMapManager() { return m_mapManager; }

  Player *getPlayer() override {
    return m_player.isInitialized() ? m_player.ensureInitialized() : nullptr;
  }

  const Player *getPlayer() const override {
    return m_player.isInitialized() ? m_player.ensureInitialized() : nullptr;
  }

  const StatManager *getStats() const override { return &m_statManager; }
  StatManager &getStats() { return m_statManager; }
  DamageTextManager &getDamageTextManager() { return m_damageTextManager; }
  const Camera &getCamera() const { return m_camera; }

  int getCurrentExp() const { return m_currentExp; }
  int getExpToNextLevel() const { return m_expToNextLevel; }
  int getCurrentLevel() const { return m_currentLevel; }

  EnemySpawner &getSpawner() { return m_spawner; }

  void resumePlaying() {
    if (m_state == GameState::LEVEL_UP)
      m_state = GameState::PLAYING;
  }

  void update(double delta_time);
  void render(double delta_time);

  void setup();
  void reset();

  void startGame();

  void movePlayer(glm::vec3 vec, bool isRunning);

  auto getClosestEnemies(float radius, uint32_t top_k) const;

  void setDebugAABB(bool state) { m_debugAABB = state; }

  [[nodiscard]] Camera &getCamera() { return m_camera; }
  [[nodiscard]] CameraController &getCameraController() {
    return m_cameraController;
  }

  [[nodiscard]] GameState getState() const { return m_state; }
  [[nodiscard]] uint32_t getScore() const {
    return static_cast<uint32_t>(m_score);
  }

  // IWeaponContext implementation
  [[nodiscard]] glm::vec3 getPlayerPosition() const override;
  [[nodiscard]] glm::vec3 getPlayerForward() const override;

  void emit(const GameEvents::ProjectileSpawnRequestedEvent &event) override {
    m_eventBus.emit(event);
  }
  void emit(const GameEvents::ParticleSpawnRequestedEvent &event) override {
    m_eventBus.emit(event);
  }
  void emit(const GameEvents::EnemyDamageRequestedEvent &event) override {
    m_eventBus.emit(event);
  }

  void findTargets(float range, uint32_t k,
                   EnemyCallback callback) const override;

  [[nodiscard]] const GameObject *
  resolveHandle(const ObjectHandle &handle) const override {
    return m_objects.get(handle);
  }

  [[nodiscard]] GameObject *resolveHandle(const ObjectHandle &handle) override {
    return m_objects.get(handle);
  }

private:
  void _calculateClosestEnemies(glm::vec3 position);

  void _initializeManagers();
  void _resetGameplayState();
  void _setupPlayer();
  void _spawnInitialEnemies();
  void _spawnInitialExp();
  void _setupEnvironment();

  void _registerGameplayEventHandlers();
  void _updateCamera(double delta_time);
  inline void _runCollisionPass();
  inline void _updateEnemies();
  inline void _syncObjectsToTerrain();
};
