#pragma once

#include <functional>
#include <vector>
#include <glm/glm.hpp>

class Game;

struct WaveConfig {
  float timeStart;
  float timeEnd;
  std::function<void(Game&, float, float)> spawnLogic; // (game, currentTime, delta_time)
};

class EnemySpawner {
private:
  std::vector<WaveConfig> m_waves;
  Game* m_game = nullptr;
  float m_defaultTimer = 0.0f;

public:
  EnemySpawner() = default;
  void init(Game* game);
  void addWave(const WaveConfig& wave);
  void clearWaves();
  void update(float currentTime, float delta_time);
  
  void spawnEnemy(glm::vec3 position, float healthMultiplier);
  void spawnInCircle(int count, float radius, float healthMultiplier);
};
