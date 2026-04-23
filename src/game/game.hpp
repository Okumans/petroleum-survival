#pragma once

#include "graphics/camera.hpp"
#include "graphics/camera_controller.hpp"
#include "graphics/skybox.hpp"
#include "scene/game_object_manager.hpp"
#include "scene/enemy.hpp"
#include "scene/item.hpp"
#include "scene/player.hpp"
#include "utility/event_bus.hpp"

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

  struct DespawnRequestedEvent {
    GameObject *object;
  };

  struct ParticleSpawnRequestedEvent {
    glm::vec3 position;
    int effectId;
  };

  Camera m_camera;
  CameraController m_cameraController;
  std::unique_ptr<Skybox> m_skybox;
  GameObjectManager m_objects;
  EventBus m_eventBus;

  int m_score = 0;

  Player *m_player = nullptr;
  Enemy *m_testEnemy = nullptr;
  Item *m_testItem = nullptr;

  // Shadow mapping
  GLuint m_shadowMapFBO;
  GLuint m_shadowMapTex;
  const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;
  glm::mat4 m_lightSpaceMatrix;

  float m_currentTime = 0.0f;

  bool m_debugAABB = true;
  GameState m_state = GameState::START_MENU;

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

  Camera &getCamera() { return m_camera; }
  CameraController &getCameraController() { return m_cameraController; }
  GameState getState() const { return m_state; }
  uint32_t getScore() const { return 0; }

private:
  void _registerGameplayEventHandlers();
  void _runCollisionPass();
  void _updateCamera(double delta_time);
};
