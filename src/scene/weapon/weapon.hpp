#pragma once

#include "game/game_events.hpp"
#include "game/stat_manager.hpp"
#include "graphics/animation_state.hpp"
#include "scene/weapon/i_weapon_context.hpp"
#include "utility/not_initialized.hpp"
#include "utility/random.hpp"
#include <glm/glm.hpp>

struct CalculatedDamage {
  float amount;
  bool isCritical;
};

class Weapon {
protected:
  AnimationState<void> m_coolDownState;
  float m_baseCooldown;
  float m_baseDamage;

  uint32_t m_level = 1;
  uint32_t m_maxLevel = 8;
  std::string m_id = "unknown_weapon";
  std::string m_name = "Unknown Weapon";
  std::string m_description = "A mysterious weapon.";
  std::string m_iconName = "icon_cone"; // Default icon

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

  virtual bool canCrit() const { return true; }

  virtual CalculatedDamage calculateDamage() const {
    return calculateDamage(getDamage());
  }

  virtual CalculatedDamage calculateDamage(float baseDmg) const {
    if (!canCrit()) {
      return {baseDmg, false};
    }

    auto stats = m_context.ensureInitialized()->getStats();
    float critChance = stats->getMultiplier(StatType::CRIT_CHANCE);
    float critMult = stats->getMultiplier(StatType::CRIT_MULTIPLIER);

    bool isCrit = Random::randChance(critChance);
    float finalDmg = isCrit ? baseDmg * critMult : baseDmg;

    return {finalDmg, isCrit};
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

  [[nodiscard]] uint32_t getLevel() const { return m_level; }
  [[nodiscard]] uint32_t getMaxLevel() const { return m_maxLevel; }
  [[nodiscard]] std::string getId() const { return m_id; }
  [[nodiscard]] std::string getName() const { return m_name; }
  [[nodiscard]] std::string getDescription() const { return m_description; }
  [[nodiscard]] std::string getIconName() const { return m_iconName; }

  // Virtual methods for weapon-specific upgrade logic and descriptions
  virtual std::string getLevelDescription(uint32_t level) const {
    return std::format("Upgrade {} to level {}", m_name, level);
  }

  virtual void onLevelUp(uint32_t newLevel) {
    (void)newLevel;
    m_baseDamage *= 1.1f; // small generic boost as fallback
  }

  void upgrade() {
    if (m_level < m_maxLevel) {
      m_level++;
      onLevelUp(m_level);
    }
  }

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
