#pragma once

#include "scene/enemy/enemy.hpp"
#include "scene/game_factories.hpp"
#include "scene/weapon/weapon.hpp"
#include "graphics/animation_state.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <memory>
#include <vector>
#include <cmath>

class GasNozzle : public Weapon {
private:
  float m_coneRange = 5.5f;        // Distance of cone ahead of player
  float m_coneHalfAngle = 35.0f;   // Degrees, half-angle of cone
  float m_damageTick = 8.0f;       // Damage per tick
  float m_tickInterval = 0.15f;    // Tick frequency (fast for spray feel)
  AnimationState<void> m_tickCooldown{0.15f};
  std::vector<Enemy*> m_enemiesInCone;

  // Context callback for target acquisition
  std::function<void(float range, uint32_t k, std::function<void(Enemy*)>)> m_findTargets;

public:
  GasNozzle()
      : Weapon(0.0f, 0.0f) {}  // No traditional cooldown/damage; ticks managed internally

  bool fire(const glm::vec3 &playerPos,
            const glm::vec3 &playerForward) override {
    // This weapon doesn't "fire" in traditional sense
    (void)playerPos;
    (void)playerForward;
    return false;
  }

  void update(double delta_time, const glm::vec3 &playerPos,
              const glm::vec3 &playerForward) override {
    m_tickCooldown.updateTimer(static_cast<float>(delta_time));

    if (m_tickCooldown.isFinished()) {
      m_enemiesInCone.clear();

      // Normalize forward direction (safety check)
      glm::vec3 normalizedForward = playerForward;
      float forwardLength = glm::length(normalizedForward);
      if (forwardLength < 0.001f) {
        // Default to no damage if direction is invalid
        m_tickCooldown.reset();
        m_tickCooldown.duration = m_tickInterval;
        return;
      }
      normalizedForward = glm::normalize(normalizedForward);

      // Query enemies in range
      if (m_findTargets) {
        m_findTargets(m_coneRange, 1000, [this](Enemy *enemy) {
          if (enemy)
            m_enemiesInCone.push_back(enemy);
        });
      }

      // Filter for cone: check if enemy is within half-angle of forward direction
      float halfAngleRad = glm::radians(m_coneHalfAngle);
      float cosConeAngle = std::cos(halfAngleRad);

      std::vector<Enemy*> filteredEnemies;
      for (Enemy *enemy : m_enemiesInCone) {
        if (!enemy)
          continue;

        glm::vec3 toEnemy = enemy->getPosition() - playerPos;
        float distToEnemy = glm::length(toEnemy);

        if (distToEnemy < 0.001f)
          continue;

        toEnemy = glm::normalize(toEnemy);
        float dotProduct = glm::dot(normalizedForward, toEnemy);

        // Check if within cone
        if (dotProduct > cosConeAngle) {
          filteredEnemies.push_back(enemy);
        }
      }

      // Apply damage to all enemies in cone
      float damageMultiplier = m_stats.ensureInitialized()->getMultiplier(StatType::MIGHT);
      for (Enemy *enemy : filteredEnemies) {
        if (!enemy)
          continue;

        emitEnemyDamage(GameEvents::EnemyDamageRequestedEvent{
            .enemy = enemy,
            .amount = m_damageTick * damageMultiplier,
            .isCritical = false,
            .knockbackDirection = normalizedForward,
            .knockbackStrength = 0.5f,
            .hitPosition = enemy->getPosition() + glm::vec3(0.0f, 1.0f, 0.0f),
            .hitEffect = GameEvents::ParticleEffectType::MAGIC_HIT,
        });
      }

      m_tickCooldown.reset();
      m_tickCooldown.duration = m_tickInterval;
    }
  }

  void setTargetingContext(std::function<void(float range, uint32_t k, std::function<void(Enemy*)>)> findTargets) {
    m_findTargets = findTargets;
  }
};
