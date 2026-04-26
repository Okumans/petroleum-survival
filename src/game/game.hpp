#pragma once

#include "graphics/camera.hpp"
#include "graphics/camera_controller.hpp"
#include "graphics/renderer.hpp"
#include "graphics/shadow_map.hpp"
#include "graphics/skybox.hpp"
#include "map_manager.hpp"
#include "scene/game_object_manager.hpp"
#include "scene/item.hpp"
#include "scene/exp.hpp"
#include "scene/player.hpp"
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

enum class GameState { LOADING, START_MENU, PLAYING, GAME_OVER };

class Game {
private:
  struct ItemCollectedEvent {
    Player *player;
    Item *item;
    int value;
  };

  struct ExpCollectedEvent {
    Player *player;
    Exp *exp;
    float amount;
  };

  struct DespawnRequestedEvent {
    GameObject *object;
  };

  struct ParticleSpawnRequestedEvent {
    glm::vec3 position;
    int effectId;
  };

  Camera m_camera;
  CameraController m_cameraController;
  MapManager m_mapManager;
  std::unique_ptr<Skybox> m_skybox;
  std::unique_ptr<ShadowMap> m_shadowMap;
  GameObjectManager m_objects;
  EventBus m_eventBus;

  int m_score = 0;

  NotInitialized<Player *> m_player;

  bool m_debugAABB = true;
  GameState m_state = GameState::START_MENU;

  Renderer m_renderer;

public:
  Game();
  ~Game();

  void update(double delta_time);
  void render(double delta_time);

  void setup();
  void reset();

  void startGame();

  void movePlayer(glm::vec3 vec);

  void setDebugAABB(bool state) { m_debugAABB = state; }

  [[nodiscard]] Camera &getCamera() { return m_camera; }
  [[nodiscard]] CameraController &getCameraController() {
    return m_cameraController;
  }

  [[nodiscard]] GameState getState() const { return m_state; }
  [[nodiscard]] uint32_t getScore() const {
    return static_cast<uint32_t>(m_score);
  }

private:
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
