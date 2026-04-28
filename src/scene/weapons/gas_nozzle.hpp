#pragma once

#include "game/game_events.hpp"
#include "graphics/animation_state.hpp"
#include "scene/enemy/enemy.hpp"
#include "scene/weapon/weapon.hpp"
#include <cmath>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

class GasNozzle : public Weapon {
private:
  float m_coneRange = 5.5f;
  float m_coneHalfAngle = 35.0f;
  float m_tickInterval = 0.15f;
  AnimationState<void> m_tickCooldown{0.15f};
  std::function<void(float range, uint32_t k, std::function<void(Enemy *)>)>
      m_findTargets;

public:
  GasNozzle() : Weapon(0.05f, 12.0f) {}

  bool fire(const glm::vec3 &playerPos,
            const glm::vec3 &playerForward) override {
    (void)playerPos;
    (void)playerForward;
    return true;
  }

  void update(double delta_time, const glm::vec3 &playerPos,
              const glm::vec3 &playerForward) override {
    float dt = static_cast<float>(delta_time);
    m_tickCooldown.updateTimer(dt);

    if (m_tickCooldown.isFinished()) {
      glm::vec3 normalizedForward = playerForward;
      float forwardLength = glm::length(normalizedForward);
      if (forwardLength < 0.001f) {
        m_tickCooldown.reset();
        m_tickInterval = getCooldown();
        m_tickCooldown.duration = m_tickInterval;
        return;
      }
      normalizedForward = glm::normalize(normalizedForward);

      float areaMultiplier =
          m_stats.ensureInitialized()->getMultiplier(StatType::AREA);
      float coneRange = m_coneRange * (1.0f + areaMultiplier * 0.12f);
      float coneHalfAngle = m_coneHalfAngle + areaMultiplier * 2.0f;
      float cosConeAngle = std::cos(glm::radians(coneHalfAngle));

      if (m_findTargets) {
        m_findTargets(coneRange, 1000, [&](Enemy *enemy) {
          if (!enemy || enemy->isRemovalRequested())
            return;

          glm::vec3 toEnemy = enemy->getPosition() - playerPos;
          float distToEnemy = glm::length(toEnemy);
          if (distToEnemy < 0.001f || distToEnemy > coneRange)
            return;

          glm::vec3 directionToEnemy = glm::normalize(toEnemy);
          if (glm::dot(normalizedForward, directionToEnemy) < cosConeAngle)
            return;

          float falloff = 1.0f - (distToEnemy / coneRange);
          emitEnemyDamage(GameEvents::EnemyDamageRequestedEvent{
              .enemy = enemy,
              .amount = getDamage() * (0.65f + falloff * 0.35f),
              .isCritical = false,
              .knockbackDirection = normalizedForward,
              .knockbackStrength = 0.5f + falloff * 0.35f,
              .hitPosition = enemy->getPosition() + glm::vec3(0.0f, 1.0f, 0.0f),
              .hitEffect = GameEvents::ParticleEffectType::FLAME,
          });
        });
      }

      emitParticle(GameEvents::ParticleSpawnRequestedEvent{
          .position = playerPos + normalizedForward * 0.8f +
                      glm::vec3(0.0f, 0.2f, 0.0f),
          .effectId = GameEvents::ParticleEffectType::FLAME,
      });

      for (int i = 0; i < 2; ++i) {
        float spread = 0.15f * static_cast<float>(i + 1);
        emitParticle(GameEvents::ParticleSpawnRequestedEvent{
            .position = playerPos + normalizedForward * (0.9f + spread) +
                        glm::vec3(0.0f, 0.1f + spread * 0.2f, 0.0f),
            .effectId = GameEvents::ParticleEffectType::FLAME,
        });
      }

      m_tickCooldown.reset();
      m_tickInterval = getCooldown();
      m_tickCooldown.duration = m_tickInterval;
    }
  }

  void setTargetingContext(
      std::function<void(float range, uint32_t k, std::function<void(Enemy *)>)>
          findTargets) {
    m_findTargets = std::move(findTargets);
  }
};
