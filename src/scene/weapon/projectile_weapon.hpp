#pragma once

#include "scene/weapon/weapon.hpp"
#include "scene/enemy/enemy.hpp"
#include <vector>

class ProjectileWeapon : public Weapon {
public:
  using EnemyCallback = std::function<void(Enemy*)>;
  using TargetAcquirer = std::function<void(float range, uint32_t k, EnemyCallback)>;

protected:
  uint32_t m_amount;
  uint32_t m_amountShoot = 0;
  AnimationState<void> m_subCooldownState;
  TargetAcquirer m_findTargets;

public:
  ProjectileWeapon(float cooldown, float sub_cooldown, float damage,
                   uint32_t amount = 1)
      : Weapon(cooldown, damage), m_amount(amount),
        m_subCooldownState(sub_cooldown) {}

  void setTargetingContext(TargetAcquirer findTargets) {
    m_findTargets = findTargets;
  }

  uint32_t getAmount() const {
    return m_amount +
           static_cast<uint32_t>(
               m_stats.ensureInitialized()->getMultiplier(StatType::AMOUNT));
  }

  std::vector<Enemy*> acquireTargets(float range, uint32_t k) const {
    std::vector<Enemy*> targets;
    if (m_findTargets) {
      m_findTargets(range, k, [&](Enemy* e) { targets.push_back(e); });
    }
    return targets;
  }

  virtual void update(double delta_time, const glm::vec3 &playerPos, const glm::vec3 &playerForward) override {
    m_coolDownState.updateTimer(static_cast<float>(delta_time));
    m_subCooldownState.updateTimer(static_cast<float>(delta_time));

    if (m_coolDownState.isFinished()) {
      if (m_subCooldownState.isFinished() && m_amountShoot < getAmount()) {
        if (fire(playerPos, playerForward)) {
          m_subCooldownState.reset();
          m_amountShoot++;
        }
      }

      if (m_amountShoot >= getAmount()) {
        m_coolDownState.reset();
        m_coolDownState.duration = getCooldown();
        m_amountShoot = 0;
      }
    }
  }
};
