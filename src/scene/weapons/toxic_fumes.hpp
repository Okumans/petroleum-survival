#pragma once

#include "glm/fwd.hpp"
#include "scene/enemy/enemy.hpp"
#include "scene/weapon/weapon.hpp"
#include "utility/not_initialized.hpp"
#include <glm/glm.hpp>
#include <vector>

class ToxicFumes : public Weapon {
private:
  float m_auraRadius = 10.5f;
  std::vector<Enemy *> m_enemiesHitThisTick;

  NotInitialized<std::function<void(float range, uint32_t k,
                                    std::function<void(Enemy *)>)>>
      m_findTargets;

public:
  ToxicFumes() : Weapon(0.5f, 6.0f) {}

  bool fire(const glm::vec3 &playerPos,
            const glm::vec3 &playerForward) override {
    (void)playerPos;
    (void)playerForward;

    m_findTargets.ensureInitialized()(getRadius(), 100, [this](Enemy *enemy) {
      if (!enemy)
        return;

      emitEnemyDamage(GameEvents::EnemyDamageRequestedEvent{
          .enemy = enemy,
          .amount = getDamage(),
          .isCritical = false,
          .knockbackDirection = glm::vec3(0.0f),
          .knockbackStrength = 0.0f,
          .hitPosition = enemy->getPosition() + glm::vec3(0.0f, 1.0f, 0.0f),
          .hitEffect = GameEvents::ParticleEffectType::MAGIC_HIT,
      });
    });

    return true;
  }

  float getRadius() const {
    return m_auraRadius *
           (1.0f +
            m_stats.ensureInitialized()->getMultiplier(StatType::AREA) * 0.1f);
  }

  void setTargetingContext(
      std::function<void(float range, uint32_t k, std::function<void(Enemy *)>)>
          find_targets) {
    m_findTargets.init(find_targets);
  }
};
