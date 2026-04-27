#include "game/enemy_spawner.hpp"
#include "game/game.hpp"
#include "scene/enemy/enemy.hpp"
#include "scene/game_object_factory.hpp"
#include "utility/random.hpp"

#include "scene/game_factories.hpp"

// We need access to createEnemyFactory, which is in game.cpp.

void EnemySpawner::init(Game *game) { m_game = game; }

void EnemySpawner::addWave(const WaveConfig &wave) { m_waves.push_back(wave); }

void EnemySpawner::clearWaves() { m_waves.clear(); }

void EnemySpawner::update(float currentTime, float delta_time) {
  if (!m_game)
    return;

  bool waveActive = false;
  for (const auto &wave : m_waves) {
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
  // 20% chance to spawn a vehicle instead of a humanoid
  if (Random::randFloat() < 0.2f) {
    ModelName carModels[] = {ModelName::CAR_SEDAN,        ModelName::CAR_MUSCLE,
                             ModelName::CAR_PICKUP,       ModelName::CAR_TAXI,
                             ModelName::CAR_POLICE,       ModelName::CAR_BUS,
                             ModelName::CAR_MONSTER_TRUCK};

    int randIdx = Random::randInt(0, 6);
    const auto &factory = GameFactories::getCar(carModels[randIdx]);

    auto [enemy, enemy_handle] =
        m_game->getObjects().createWithHandle<CarEnemy>(
            factory, [&](CarEnemy &car) { car.moveWithAnimation(position); });

    m_game->getMapManager().registerObject(enemy_handle, enemy.getPosition(),
                                           false);
  } else {
    const auto &factory = GameFactories::getHumanoidEnemy();

    auto [enemy, enemy_handle] =
        m_game->getObjects().createWithHandle<HumanoidEnemy>(
            factory, [&](HumanoidEnemy &enemy) { enemy.move(position); });

    m_game->getMapManager().registerObject(enemy_handle, enemy.getPosition(),
                                           false);
  }
}

void EnemySpawner::spawnInCircle(int count, float radius,
                                 float healthMultiplier) {
  if (!m_game || !m_game->getPlayer())
    return;

  glm::vec3 center = m_game->getPlayer()->getPosition();

  for (int i = 0; i < count; ++i) {
    float angle = Random::randFloat(0.0f, 3.14159f * 2.0f);
    glm::vec3 offset(std::cos(angle) * radius, 0.0f, std::sin(angle) * radius);
    spawnEnemy(center + offset, healthMultiplier);
  }
}
