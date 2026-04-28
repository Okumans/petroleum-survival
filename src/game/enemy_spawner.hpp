#pragma once

#include "scene/game_object.hpp"
#include <functional>
#include <glm/glm.hpp>
#include <vector>

class Game;

struct WaveConfig {
  float timeStart;
  float timeEnd;
  std::function<void(Game &, float, float)>
      spawnLogic; // (game, currentTime, delta_time)
};

class EnemySpawner {
private:
  std::vector<WaveConfig> m_waves;
  Game *m_game = nullptr;
  float m_defaultTimer = 0.0f;

public:
  EnemySpawner() = default;
  void init(Game *game);
  void addWave(const WaveConfig &wave);
  void clearWaves();
  void update(float currentTime, float delta_time);

  void spawnEnemy(glm::vec3 position, float healthMultiplier);
  void spawnSpecificEnemy(GameObjectType type, glm::vec3 pos,
                          float health_multiplier = 1.0f);
  void spawnInCircle(int count, float radius, float healthMultiplier);
};
