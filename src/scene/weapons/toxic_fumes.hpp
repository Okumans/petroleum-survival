#pragma once

#include "game/game_events.hpp"
#include "graphics/animation_state.hpp"
#include "scene/enemy/enemy.hpp"
#include "scene/weapon/weapon.hpp"
#include <glm/glm.hpp>

class ToxicFumes : public Weapon {
private:
  float m_auraRadius = 3.5f;
  float m_knockbackStrength = 2.5f;

  AnimationState<void> m_visualPulseTimer{0.1f};
  AnimationState<void> m_attackPulseTimer{0.15f};

public:
  ToxicFumes() : Weapon(0.8f, 15.0f) {
    m_id = "toxic_fumes";
    m_name = "Toxic Fumes";
    m_description = "Emits toxic fumes in an aura.";
    m_iconName = "icon_gas_fume";
    m_maxLevel = 8;
    m_visualPulseTimer.startAnimation();
  }

  std::string getLevelDescription(uint32_t level) const override {
    switch (level) {
    case 1:
      return "Emits toxic fumes in an aura.";
    case 2:
      return "Cooldown -10%.";
    case 3:
      return "Area +10%.";
    case 4:
      return "Damage +5.";
    case 5:
      return "Cooldown -10%.";
    case 6:
      return "Area +10%.";
    case 7:
      return "Damage +5.";
    case 8:
      return "Area +15%.";
    default:
      return "Upgrade " + m_name + " to level " + std::to_string(level) + ".";
    }
  }

  void onLevelUp(uint32_t newLevel) override {
    switch (newLevel) {
    case 2:
      setBaseCooldown(getBaseCooldown() * 0.9f);
      break;
    case 3:
      m_auraRadius *= 1.1f;
      break;
    case 4:
      m_baseDamage += 5.0f;
      break;
    case 5:
      setBaseCooldown(getBaseCooldown() * 0.9f);
      break;
    case 6:
      m_auraRadius *= 1.1f;
      break;
    case 7:
      m_baseDamage += 5.0f;
      break;
    case 8:
      m_auraRadius *= 1.15f;
      break;
    default:
      m_baseDamage *= 1.1f;
      break;
    }
  }

  [[nodiscard]] float getRadius() const {
    auto &ctx = *m_context.ensureInitialized();
    float area_multiplier = ctx.getStats()->getMultiplier(StatType::AREA);
    return m_auraRadius * (1.0f + area_multiplier * 0.15f);
  }

  bool fire() override {
    auto &ctx = *m_context.ensureInitialized();
    glm::vec3 player_pos = ctx.getPlayerPosition();

    float radius = getRadius();
    float damage = getDamage();

    ctx.findTargets(radius, 1000, [&](Enemy *enemy) {
      if (!enemy || enemy->isRemovalRequested()) {
        return;
      }

      glm::vec3 enemy_pos = enemy->getHitboxAABB().getClosestPoint(player_pos);
      glm::vec3 to_enemy = enemy_pos - player_pos;
      float dist = glm::length(to_enemy);

      glm::vec3 knockback_dir =
          (dist < 0.001f) ? glm::vec3(0.0f, 0.0f, 1.0f) : to_enemy / dist;

      float normalized_dist = glm::clamp(dist / radius, 0.0f, 1.0f);
      float falloff_multiplier = glm::mix(1.0f, 0.2f, normalized_dist);

      float calculated_knockback = m_knockbackStrength * falloff_multiplier;

      emitEnemyDamage(GameEvents::EnemyDamageRequestedEvent{
          .enemy = enemy,
          .amount = damage,
          .isCritical = false,
          .knockbackDirection = knockback_dir,
          .knockbackStrength = calculated_knockback,
          .hitPosition = enemy_pos + glm::vec3(0.0f, 1.0f, 0.0f),
          .hitEffect = GameEvents::ParticleEffectType::FUME_IDLE,
      });
    });

    return true;
  }

  void update(double delta_time) override {
    auto &ctx = *m_context.ensureInitialized();

    m_visualPulseTimer.updateTimer(static_cast<float>(delta_time));
    m_coolDownState.updateTimer(static_cast<float>(delta_time));

    if (m_attackPulseTimer.isFinished()) {
      m_coolDownState.reset();
    }

    if (m_visualPulseTimer.isFinished()) {
      emitParticle(GameEvents::ParticleSpawnRequestedEvent{
          .position = ctx.getPlayerPosition(),
          .length = getRadius(),
          .thickness = 0.6f,
          .effectId = GameEvents::ParticleEffectType::FUME_IDLE,
      });
      m_visualPulseTimer.reset();
    }

    if (m_coolDownState.isFinished()) {
      if (fire()) {
        m_coolDownState.reset();
        m_coolDownState.duration = getCooldown();

        emitParticle(GameEvents::ParticleSpawnRequestedEvent{
            .position = ctx.getPlayerPosition(),
            .length = getRadius(),
            .thickness = 0.6f,
            .effectId = GameEvents::ParticleEffectType::FUME_ATTACk,
        });
      }
    }
  }
};
