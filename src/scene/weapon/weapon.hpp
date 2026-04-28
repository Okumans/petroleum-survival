#pragma once

#include "game/game_events.hpp"
#include "game/stat_manager.hpp"
#include "graphics/animation_state.hpp"
#include "scene/weapon/i_weapon_context.hpp"
#include "utility/not_initialized.hpp"
#include <glm/glm.hpp>

class Weapon {
protected:
  AnimationState<void> m_coolDownState;
  float m_baseCooldown;
  float m_baseDamage;

  NotInitialized<IWeaponContext *> m_context;

public:
  Weapon(float cooldown, float damage)
      : m_coolDownState(cooldown), m_baseCooldown(cooldown),
        m_baseDamage(damage) {}

  virtual ~Weapon() = default;

  void setContext(IWeaponContext *context) { m_context.init(context); }

  virtual float getDamage() const {
    return m_baseDamage *
           m_context.ensureInitialized()->getStats()->getMultiplier(
               StatType::MIGHT);
  }

  virtual float getCooldown() const {
    return m_baseCooldown *
           m_context.ensureInitialized()->getStats()->getMultiplier(
               StatType::COOLDOWN);
  }

  // Setters for upgrade system
  void setBaseDamage(float damage) { m_baseDamage = damage; }
  void setBaseCooldown(float cooldown) {
    m_baseCooldown = cooldown;
    m_coolDownState.duration = cooldown;
  }

  [[nodiscard]] float getBaseDamage() const { return m_baseDamage; }
  [[nodiscard]] float getBaseCooldown() const { return m_baseCooldown; }

  virtual void update(double delta_time) {
    m_coolDownState.updateTimer(static_cast<float>(delta_time));
    if (m_coolDownState.isFinished()) {
      if (fire()) {
        m_coolDownState.reset();
        m_coolDownState.duration = getCooldown();
      }
    }
  }

  virtual bool fire() = 0;

protected:
  void emitProjectile(const GameEvents::ProjectileSpawnRequestedEvent &event) {
    m_context.ensureInitialized()->emit(event);
  }

  void emitParticle(const GameEvents::ParticleSpawnRequestedEvent &event) {
    m_context.ensureInitialized()->emit(event);
  }

  void emitEnemyDamage(const GameEvents::EnemyDamageRequestedEvent &event) {
    m_context.ensureInitialized()->emit(event);
  }
};
