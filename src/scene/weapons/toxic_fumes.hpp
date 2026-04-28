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

  AnimationState<void> m_visualPulseTimer{0.25f};

public:
  ToxicFumes() : Weapon(0.8f, 15.0f) { m_visualPulseTimer.startAnimation(); }

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
          .hitEffect = GameEvents::ParticleEffectType::FUME,
      });
    });

    return true;
  }

  void update(double delta_time) override {
    auto &ctx = *m_context.ensureInitialized();
    float dt = static_cast<float>(delta_time);

    m_visualPulseTimer.updateTimer(dt);
    if (m_visualPulseTimer.isFinished()) {
      emitParticle(GameEvents::ParticleSpawnRequestedEvent{
          .position = ctx.getPlayerPosition(),
          .length = getRadius(),
          .thickness = 0.6f,
          .effectId = GameEvents::ParticleEffectType::FUME,
      });
      m_visualPulseTimer.reset();
    }

    Weapon::update(delta_time);
  }
};
