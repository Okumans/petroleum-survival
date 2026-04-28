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
  
  // Spawn config: fraction of spawns that appear on-screen [0.0, 1.0]
  float m_onScreenSpawnFraction = 0.3f;
  float m_onScreenRadius = 12.0f;     // Spawns within this radius are considered on-screen
  float m_offScreenRadius = 25.0f;    // Spawns beyond on-screen radius but within this

public:
  EnemySpawner() = default;
  void init(Game *game);
  void addWave(const WaveConfig &wave);
  void clearWaves();
  void update(float currentTime, float delta_time);

  void spawnEnemy(glm::vec3 position, float healthMultiplier);
  void spawnSpecificEnemy(GameObjectType type, glm::vec3 pos,
                          float health_multiplier = 1.0f, int tier = 0);
  void spawnInCircle(int count, float radius, float healthMultiplier);
  
  // New: spawn with mixed on-screen and off-screen distribution
  void spawnMixed(int count, float healthMultiplier);
  
  // Setters for spawn config
  void setOnScreenSpawnFraction(float fraction) { m_onScreenSpawnFraction = fraction; }
  void setOnScreenRadius(float radius) { m_onScreenRadius = radius; }
  void setOffScreenRadius(float radius) { m_offScreenRadius = radius; }
};
