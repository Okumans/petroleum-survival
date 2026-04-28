#pragma once

#include "scene/weapon/weapon.hpp"
#include "scene/enemy/enemy.hpp"
#include <vector>

class ProjectileWeapon : public Weapon {
protected:
  uint32_t m_amount;
  uint32_t m_amountShoot = 0;
  AnimationState<void> m_subCooldownState;

public:
  ProjectileWeapon(float cooldown, float sub_cooldown, float damage,
                   uint32_t amount = 1)
      : Weapon(cooldown, damage), m_amount(amount),
        m_subCooldownState(sub_cooldown) {}

  uint32_t getAmount() const {
    return m_amount +
           static_cast<uint32_t>(
               m_context.ensureInitialized()->getStats()->getMultiplier(
                   StatType::AMOUNT));
  }

  std::vector<Enemy *> acquireTargets(float range, uint32_t k) const {
    std::vector<Enemy *> targets;
    m_context.ensureInitialized()->findTargets(
        range, k, [&](Enemy *enemy) { targets.push_back(enemy); });
    return targets;
  }

  void update(double delta_time) override {
    m_coolDownState.updateTimer(static_cast<float>(delta_time));
    m_subCooldownState.updateTimer(static_cast<float>(delta_time));

    if (m_coolDownState.isFinished()) {
      if (m_subCooldownState.isFinished() && m_amountShoot < getAmount()) {
        if (fire()) {
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
