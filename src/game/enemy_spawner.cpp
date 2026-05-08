#include "game/enemy_spawner.hpp"
#include "game/game.hpp"
#include "glm/gtc/constants.hpp"
#include "utility/random.hpp"

#include "resource/model_manager.hpp"
#include "scene/enemy/car_enemy.hpp"
#include "scene/enemy/humanoid_enemy.hpp"
#include "scene/game_factories.hpp"

using Random = Utility::Random;

void EnemySpawner::init(Game *game) { m_game = game; }

void EnemySpawner::addWave(const WaveConfig &wave) { m_waves.push_back(wave); }

void EnemySpawner::clearWaves() { m_waves.clear(); }

void EnemySpawner::update(float current_time, float delta_time) {
  if (!m_game)
    return;

  bool wave_active = false;
  for (const auto &wave : m_waves) {
    if (current_time >= wave.timeStart && current_time <= wave.timeEnd) {
      if (wave.spawnLogic) {
        wave.spawnLogic(*m_game, current_time, delta_time);
      }
      wave_active = true;
    }
  }

  if (!wave_active) {
    m_defaultTimer += delta_time;
    if (m_defaultTimer >= 1.0f) { // spawn roughly every 1 second
      m_defaultTimer = 0.0f;
      // Health scales over time linearly: 1.0 + 0.1 per second
      float health_scaling = 1.0f + (current_time * 0.05f);
      // Spawn 3 enemies: mix of on-screen and off-screen
      spawnMixed(3, health_scaling);
    }
  }
}

void EnemySpawner::spawnEnemy(glm::vec3 position, float health_multiplier) {
  (void)health_multiplier;
  spawnSpecificEnemy(GameObjectType::ENEMY, position);
}

void EnemySpawner::spawnSpecificEnemy(GameObjectType type, glm::vec3 pos,
                                      float health_multiplier, int tier) {
  if (!m_game)
    return;

  // Determine exp drop amount based on tier
  // Tier 0 (common): 20, Tier 1 (elite): 100, Tier 2 (boss): 500, Tier 3
  // (legendary): 1000
  static constexpr int tier_exp_map[] = {20, 100, 500, 1000};
  int exp_amount = tier_exp_map[std::clamp(tier, 0, 3)];

  switch (type) {
  case GameObjectType::ENEMY: {
    const auto &factory = GameFactories::getHumanoidEnemy();
    auto [enemy, enemy_handle] =
        m_game->getObjects().createWithHandle<HumanoidEnemy>(
            factory, [&](HumanoidEnemy &enemy) {
              enemy.setContext(m_game);
              enemy.move(pos);
              enemy.setMaxHealth(enemy.getMaxHealth() * health_multiplier);
              enemy.setHealth(enemy.getMaxHealth());
              // Apply tier-based exp scaling
              float exp_scale = 1.0f + (tier * 0.5f);
              enemy.setExpDropAmount(exp_amount * exp_scale);
            });
    m_game->getMapManager().registerObject(enemy_handle, enemy.getPosition(),
                                           false);
    break;
  }
  case GameObjectType::WEAK_CAR_ENEMY: {
    ModelName model =
        Random::randChance(0.5f) ? ModelName::CAR_SEDAN : ModelName::CAR_TAXI;
    auto model_ptr = ModelManager::copy(model);
    auto [enemy, enemy_handle] =
        m_game->getObjects().emplaceWithHandle<WeakCarEnemy>(model_ptr, pos);
    enemy.setContext(m_game);
    enemy.setScale(0.8f);
    enemy.setMaxHealth(enemy.getMaxHealth() * health_multiplier);
    enemy.setHealth(enemy.getMaxHealth());
    // Apply tier-based exp scaling
    float exp_scale = 1.0f + (tier * 0.5f);
    enemy.setExpDropAmount(exp_amount * exp_scale);
    m_game->getMapManager().registerObject(enemy_handle, enemy.getPosition(),
                                           false);
    break;
  }
  case GameObjectType::STANDARD_CAR_ENEMY: {
    ModelName model = Random::randChance(0.5f) ? ModelName::CAR_MUSCLE
                                               : ModelName::CAR_PICKUP;
    auto model_ptr = ModelManager::copy(model);
    auto [enemy, enemy_handle] =
        m_game->getObjects().emplaceWithHandle<StandardCarEnemy>(model_ptr,
                                                                 pos);
    enemy.setContext(m_game);
    enemy.setScale(0.8f);
    enemy.setMaxHealth(enemy.getMaxHealth() * health_multiplier);
    enemy.setHealth(enemy.getMaxHealth());
    // Apply tier-based exp scaling
    float exp_scale = 1.0f + (tier * 0.5f);
    enemy.setExpDropAmount(exp_amount * exp_scale);
    m_game->getMapManager().registerObject(enemy_handle, enemy.getPosition(),
                                           false);
    break;
  }
  case GameObjectType::ARMORED_CAR_ENEMY: {
    ModelName model =
        Random::randChance(0.5f) ? ModelName::CAR_POLICE : ModelName::CAR_BUS;
    auto model_ptr = ModelManager::copy(model);
    auto [enemy, enemy_handle] =
        m_game->getObjects().emplaceWithHandle<ArmoredCarEnemy>(model_ptr, pos);
    enemy.setContext(m_game);
    enemy.setScale(0.8f);
    enemy.setMaxHealth(enemy.getMaxHealth() * health_multiplier);
    enemy.setHealth(enemy.getMaxHealth());
    // Apply tier-based exp scaling
    float exp_scale = 1.0f + (tier * 0.5f);
    enemy.setExpDropAmount(exp_amount * exp_scale);
    m_game->getMapManager().registerObject(enemy_handle, enemy.getPosition(),
                                           false);
    break;
  }
  case GameObjectType::BOSS_CAR_ENEMY: {
    auto model_ptr = ModelManager::copy(ModelName::CAR_MONSTER_TRUCK);
    auto [enemy, enemy_handle] =
        m_game->getObjects().emplaceWithHandle<BossCarEnemy>(model_ptr, pos);
    enemy.setContext(m_game);
    enemy.setScale(1.5f);
    enemy.setMaxHealth(enemy.getMaxHealth() * health_multiplier);
    enemy.setHealth(enemy.getMaxHealth());
    // Boss tier is always tier 3
    float exp_scale = 1.0f + (3 * 0.5f);
    enemy.setExpDropAmount(tier_exp_map[3] * exp_scale);
    m_game->getMapManager().registerObject(enemy_handle, enemy.getPosition(),
                                           false);
    break;
  }
  default:
    break;
  }
}

void EnemySpawner::spawnInCircle(int count, float radius,
                                 float health_multiplier) {
  if (!m_game || !m_game->getPlayer())
    return;

  glm::vec3 center = m_game->getPlayer()->getPosition();

  for (int i = 0; i < count; ++i) {
    float angle = Random::randFloat(0.0f, glm::two_pi<float>());
    glm::vec3 offset(std::cos(angle) * radius, 0.0f, std::sin(angle) * radius);
    spawnEnemy(center + offset, health_multiplier);
  }
}

void EnemySpawner::spawnMixed(int count, float health_multiplier) {
  if (!m_game || !m_game->getPlayer())
    return;

  glm::vec3 center = m_game->getPlayer()->getPosition();

  // Calculate how many spawn on-screen vs off-screen
  int on_screen_count = static_cast<int>(count * m_onScreenSpawnFraction);
  int off_screen_count = count - on_screen_count;

  // Spawn on-screen enemies (within m_onScreenRadius)
  for (int i = 0; i < on_screen_count; ++i) {
    float angle = Random::randFloat(0.0f, glm::two_pi<float>());
    float radius_variation = Random::randFloat(0.0f, m_onScreenRadius);
    glm::vec3 offset(std::cos(angle) * radius_variation, 0.0f,
                     std::sin(angle) * radius_variation);
    spawnEnemy(center + offset, health_multiplier);
  }

  // Spawn off-screen enemies (between m_onScreenRadius and m_offScreenRadius)
  for (int i = 0; i < off_screen_count; ++i) {
    float angle = Random::randFloat(0.0f, glm::two_pi<float>());
    float radius_variation =
        Random::randFloat(m_onScreenRadius, m_offScreenRadius);
    glm::vec3 offset(std::cos(angle) * radius_variation, 0.0f,
                     std::sin(angle) * radius_variation);
    spawnEnemy(center + offset, health_multiplier);
  }
}
