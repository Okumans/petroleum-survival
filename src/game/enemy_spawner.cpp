#include "game/enemy_spawner.hpp"
#include "game/game.hpp"
#include "scene/enemy/enemy.hpp"
#include "scene/game_object_factory.hpp"
#include "utility/random.hpp"

// We need access to createEnemyFactory, which is in game.cpp. 
// We can move it to game.hpp or just re-implement/call a Game method.
// For now, let's declare it as extern if it's not static.
#include "scene/enemy/humanoid_enemy.hpp"

extern GameObjectFactory<HumanoidEnemy> createEnemyFactory();

void EnemySpawner::init(Game* game) {
  m_game = game;
}

void EnemySpawner::addWave(const WaveConfig& wave) {
  m_waves.push_back(wave);
}

void EnemySpawner::clearWaves() {
  m_waves.clear();
}

void EnemySpawner::update(float currentTime, float delta_time) {
  if (!m_game) return;

  bool waveActive = false;
  for (const auto& wave : m_waves) {
    if (currentTime >= wave.timeStart && currentTime <= wave.timeEnd) {
      if (wave.spawnLogic) {
        wave.spawnLogic(*m_game, currentTime, delta_time);
      }
      waveActive = true;
    }
  }

  // Default wave logic if no specific wave is active
  if (!waveActive) {
    m_defaultTimer += delta_time;
    if (m_defaultTimer >= 1.0f) { // spawn roughly every 1 second
      m_defaultTimer = 0.0f;
      // Health scales over time linearly: 1.0 + 0.1 per second
      float healthScaling = 1.0f + (currentTime * 0.05f);
      // Spawn 3 enemies off-screen
      spawnInCircle(3, 25.0f, healthScaling);
    }
  }
}

void EnemySpawner::spawnEnemy(glm::vec3 position, float healthMultiplier) {
  static GameObjectFactory<HumanoidEnemy> factory = createEnemyFactory();
  
  HumanoidEnemy enemy_clone = factory.create([&](HumanoidEnemy &enemy) {
    enemy.move(position);
    // TODO: apply health multiplier if enemy supports it
    // snapObjectToGround logic is handled in game.cpp or we can do it later
  });

  auto [enemy, enemy_handle] = m_game->getObjects().emplaceWithHandle<HumanoidEnemy>(std::move(enemy_clone));
  m_game->getMapManager().registerObject(enemy_handle, enemy.getPosition(), false);
}

void EnemySpawner::spawnInCircle(int count, float radius, float healthMultiplier) {
  if (!m_game || !m_game->getPlayer()) return;
  
  glm::vec3 center = m_game->getPlayer()->getPosition();
  
  for (int i = 0; i < count; ++i) {
    float angle = Random::randFloat(0.0f, 3.14159f * 2.0f);
    glm::vec3 offset(std::cos(angle) * radius, 0.0f, std::sin(angle) * radius);
    spawnEnemy(center + offset, healthMultiplier);
  }
}
