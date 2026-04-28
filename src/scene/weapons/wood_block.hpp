#pragma once

#include "game/game_events.hpp"
#include "graphics/animation_state.hpp"
#include "scene/enemy/enemy.hpp"
#include "scene/weapon/weapon.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <unordered_set>
#include <string>

class SolidWoodBlock : public Weapon {
private:
  float m_sweepRange = 2.2f;
  float m_sweepHalfAngleDeg = 55.0f;
  float m_knockbackStrength = 2.0f;
  float m_sweepDuration = 0.18f;
  float m_sweepEmitInterval = 0.02f;

  AnimationState<void> m_sweepState{0.18f};
  AnimationState<void> m_sweepEmitTimer{0.02f};
  std::unordered_set<Enemy *> m_sweepHit;
  float m_lastSweepRange = 2.2f;

public:
  SolidWoodBlock() : Weapon(1.0f, 20.0f) {
    m_id = "solid_wood_block";
    m_name = "Solid Wood Block";
    m_description = "Sweeps a heavy block in front of you.";
    m_iconName = "icon_solid_wood_block";
    m_maxLevel = 8;
    m_sweepState.duration = m_sweepDuration;
    m_sweepEmitTimer.duration = m_sweepEmitInterval;
    m_sweepState.timer = m_sweepState.duration;
  }

  std::string getLevelDescription(uint32_t level) const override {
    switch (level) {
    case 1:
      return "Swings a heavy block in front of you.";
    case 2:
      return "Cooldown -10%.";
    case 3:
      return "Damage +10.";
    case 4:
      return "Area +15%.";
    case 5:
      return "Cooldown -10%.";
    case 6:
      return "Damage +10.";
    case 7:
      return "Area +15%.";
    case 8:
      return "Damage +15.";
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
      m_baseDamage += 10.0f;
      break;
    case 4:
      m_sweepRange *= 1.15f;
      break;
    case 5:
      setBaseCooldown(getBaseCooldown() * 0.9f);
      break;
    case 6:
      m_baseDamage += 10.0f;
      break;
    case 7:
      m_sweepRange *= 1.15f;
      break;
    case 8:
      m_baseDamage += 15.0f;
      break;
    default:
      m_baseDamage *= 1.1f;
      break;
    }
  }

  bool fire() override {
    auto &ctx = *m_context.ensureInitialized();
    glm::vec3 forward = ctx.getPlayerForward();
    float forward_len = glm::length(forward);
    if (forward_len < 0.001f) {
      return false;
    }
    forward /= forward_len;

    if (m_sweepState.animationStarted && !m_sweepState.isFinished()) {
      // Already sweeping.
      return false;
    }

    float area_multiplier = ctx.getStats()->getMultiplier(StatType::AREA);
    m_lastSweepRange = m_sweepRange * (1.0f + area_multiplier * 0.12f);

    m_sweepHit.clear();
    m_sweepState.duration = m_sweepDuration;
    m_sweepEmitTimer.duration = m_sweepEmitInterval;
    m_sweepState.startAnimation();
    m_sweepEmitTimer.startAnimation();
    return true;
  }

  void update(double delta_time) override {
    float dt = static_cast<float>(delta_time);
    auto &ctx = *m_context.ensureInitialized();

    m_coolDownState.updateTimer(dt);
    const bool isSweeping =
        m_sweepState.animationStarted && !m_sweepState.isFinished();
    if (!isSweeping) {
      if (m_coolDownState.isFinished()) {
        if (fire()) {
          m_coolDownState.reset();
          m_coolDownState.duration = getCooldown();
        }
      }
      return;
    }

    m_sweepState.updateTimer(dt);
    m_sweepEmitTimer.updateTimer(dt);

    glm::vec3 player_pos = ctx.getPlayerPosition();
    glm::vec3 forward = ctx.getPlayerForward();
    float forward_len = glm::length(forward);
    if (forward_len < 0.001f) {
      forward = glm::vec3(0.0f, 0.0f, 1.0f);
    } else {
      forward /= forward_len;
    }

    // Sweep from left to right over the sweep duration.
    float progress = static_cast<float>(m_sweepState.getProgress());

    float angle_deg =
        glm::mix(-m_sweepHalfAngleDeg, m_sweepHalfAngleDeg, progress);

    float yaw = glm::radians(angle_deg);
    glm::vec3 sweep_dir;
    sweep_dir.x = forward.x * std::cos(yaw) - forward.z * std::sin(yaw);
    sweep_dir.y = 0.0f;
    sweep_dir.z = forward.x * std::sin(yaw) + forward.z * std::cos(yaw);
    float sweep_len = glm::length(sweep_dir);
    if (sweep_len < 0.001f) {
      sweep_dir = forward;
    } else {
      sweep_dir /= sweep_len;
    }

    const float cos_angle = std::cos(glm::radians(12.0f)); // thin blade slice

    // Emit sweep particles at a fixed interval while sweeping.
    if (m_sweepEmitTimer.isFinished()) {
      emitParticle(GameEvents::ParticleSpawnRequestedEvent{
          .position = player_pos,
          .direction = sweep_dir,
          .length = m_lastSweepRange,
          .thickness = glm::max(0.35f, m_lastSweepRange * 0.22f),
          .effectId = GameEvents::ParticleEffectType::WOOD_SWEEP,
      });

      m_sweepEmitTimer.reset();
      m_sweepEmitTimer.duration = m_sweepEmitInterval;
    }

    // Damage pass: only damage each enemy once per sweep.
    ctx.findTargets(m_lastSweepRange, 1000, [&](Enemy *enemy) {
      if (!enemy || enemy->isRemovalRequested()) {
        return;
      }
      if (m_sweepHit.contains(enemy)) {
        return;
      }

      AABB enemy_box = enemy->getHitboxAABB();
      glm::vec3 closest = enemy_box.getClosestPoint(player_pos);
      glm::vec3 to_enemy = closest - player_pos;
      float dist = glm::length(to_enemy);
      if (dist < 0.001f || dist > m_lastSweepRange) {
        return;
      }

      glm::vec3 dir = to_enemy / dist;
      if (glm::dot(sweep_dir, dir) < cos_angle) {
        return;
      }

      m_sweepHit.insert(enemy);

      auto dmg = calculateDamage();
      emitEnemyDamage(GameEvents::EnemyDamageRequestedEvent{
          .enemy = enemy,
          .amount = dmg.amount,
          .isCritical = dmg.isCritical,
          .knockbackDirection = dir,
          .knockbackStrength = m_knockbackStrength,
          .hitPosition = closest + glm::vec3(0.0f, 0.8f, 0.0f),
          .hitEffect = GameEvents::ParticleEffectType::PHYSICAL_HIT,
      });
    });
  }
};
