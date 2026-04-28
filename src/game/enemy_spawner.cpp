#include "game/enemy_spawner.hpp"
#include "game/game.hpp"
#include "utility/random.hpp"

#include "resource/model_manager.hpp"
#include "scene/enemy/car_enemy.hpp"
#include "scene/enemy/humanoid_enemy.hpp"
#include "scene/game_factories.hpp"

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
  (void)healthMultiplier;
  spawnSpecificEnemy(GameObjectType::ENEMY, position);
}

void EnemySpawner::spawnSpecificEnemy(GameObjectType type, glm::vec3 pos,
                                      float health_multiplier) {
  if (!m_game)
    return;

  switch (type) {
  case GameObjectType::ENEMY: {
    const auto &factory = GameFactories::getHumanoidEnemy();
    auto [enemy, enemy_handle] =
        m_game->getObjects().createWithHandle<HumanoidEnemy>(
            factory, [&](HumanoidEnemy &enemy) {
              enemy.move(pos);
              enemy.setMaxHealth(enemy.getMaxHealth() * health_multiplier);
              enemy.setHealth(enemy.getMaxHealth());
            });
    m_game->getMapManager().registerObject(enemy_handle, enemy.getPosition(),
                                           false);
    break;
  }
  case GameObjectType::WEAK_CAR_ENEMY: {
    ModelName model = (Random::randFloat() < 0.5f) ? ModelName::CAR_SEDAN
                                                   : ModelName::CAR_TAXI;
    auto model_ptr = ModelManager::copy(model);
    auto [enemy, enemy_handle] =
        m_game->getObjects().emplaceWithHandle<WeakCarEnemy>(model_ptr, pos);
    enemy.setScale(0.8f);
    enemy.setMaxHealth(enemy.getMaxHealth() * health_multiplier);
    enemy.setHealth(enemy.getMaxHealth());
    m_game->getMapManager().registerObject(enemy_handle, enemy.getPosition(),
                                           false);
    break;
  }
  case GameObjectType::STANDARD_CAR_ENEMY: {
    ModelName model = (Random::randFloat() < 0.5f) ? ModelName::CAR_MUSCLE
                                                   : ModelName::CAR_PICKUP;
    auto model_ptr = ModelManager::copy(model);
    auto [enemy, enemy_handle] =
        m_game->getObjects().emplaceWithHandle<StandardCarEnemy>(model_ptr,
                                                                 pos);
    enemy.setScale(0.8f);
    enemy.setMaxHealth(enemy.getMaxHealth() * health_multiplier);
    enemy.setHealth(enemy.getMaxHealth());
    m_game->getMapManager().registerObject(enemy_handle, enemy.getPosition(),
                                           false);
    break;
  }
  case GameObjectType::ARMORED_CAR_ENEMY: {
    ModelName model = (Random::randFloat() < 0.5f) ? ModelName::CAR_POLICE
                                                   : ModelName::CAR_BUS;
    auto model_ptr = ModelManager::copy(model);
    auto [enemy, enemy_handle] =
        m_game->getObjects().emplaceWithHandle<ArmoredCarEnemy>(model_ptr, pos);
    enemy.setScale(0.8f);
    enemy.setMaxHealth(enemy.getMaxHealth() * health_multiplier);
    enemy.setHealth(enemy.getMaxHealth());
    m_game->getMapManager().registerObject(enemy_handle, enemy.getPosition(),
                                           false);
    break;
  }
  case GameObjectType::BOSS_CAR_ENEMY: {
    auto model_ptr = ModelManager::copy(ModelName::CAR_MONSTER_TRUCK);
    auto [enemy, enemy_handle] =
        m_game->getObjects().emplaceWithHandle<BossCarEnemy>(model_ptr, pos);
    enemy.setScale(1.5f);
    enemy.setMaxHealth(enemy.getMaxHealth() * health_multiplier);
    enemy.setHealth(enemy.getMaxHealth());
    m_game->getMapManager().registerObject(enemy_handle, enemy.getPosition(),
                                           false);
    break;
  }
  default:
    break;
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
