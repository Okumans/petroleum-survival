#pragma once

#include "game/game_events.hpp"
#include "game/stat_manager.hpp"
#include "graphics/animation_state.hpp"
#include "utility/not_initialized.hpp"
#include <functional>
#include <glm/glm.hpp>

class Weapon {
protected:
  AnimationState<void> m_coolDownState;
  float m_baseCooldown;
  float m_baseDamage;
  NotInitialized<const StatManager *> m_stats;

  std::function<void(const GameEvents::ProjectileSpawnRequestedEvent &)>
      m_spawnProjectile;
    std::function<void(const GameEvents::EnemyDamageRequestedEvent &)>
      m_requestEnemyDamage;

public:
  Weapon(float cooldown, float damage)
      : m_coolDownState(cooldown), m_baseCooldown(cooldown),
        m_baseDamage(damage) {}

  virtual ~Weapon() = default;

  void setStats(const StatManager *stats) { m_stats.init(stats); }

  virtual float getDamage() const {
    return m_baseDamage *
           m_stats.ensureInitialized()->getMultiplier(StatType::MIGHT);
  }

  virtual float getCooldown() const {
    return m_baseCooldown *
           m_stats.ensureInitialized()->getMultiplier(StatType::COOLDOWN);
  }

  void setContext(
      std::function<void(const GameEvents::ProjectileSpawnRequestedEvent &)>
          spawnProjectile) {
    m_spawnProjectile = spawnProjectile;
  }

  void setDamageContext(
      std::function<void(const GameEvents::EnemyDamageRequestedEvent &)>
          requestEnemyDamage) {
    m_requestEnemyDamage = requestEnemyDamage;
  }

  void emitEnemyDamage(const GameEvents::EnemyDamageRequestedEvent &evt) const {
    if (m_requestEnemyDamage) {
      m_requestEnemyDamage(evt);
    }
  }

  virtual void update(double delta_time, const glm::vec3 &playerPos,
                      const glm::vec3 &playerForward) {
    m_coolDownState.updateTimer(static_cast<float>(delta_time));

    if (m_coolDownState.isFinished()) {
      if (fire(playerPos, playerForward)) {
        m_coolDownState.reset();
        m_coolDownState.duration = getCooldown();
      }
    }
  }

  // Returns true if successfully fired
  virtual bool fire(const glm::vec3 &playerPos,
                    const glm::vec3 &playerForward) = 0;
};
