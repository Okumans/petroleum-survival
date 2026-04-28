#pragma once

#include "game/game_events.hpp"
#include "graphics/animation_state.hpp"
#include "scene/enemy/enemy.hpp"
#include "scene/player.hpp"
#include "scene/weapon/weapon.hpp"
#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

class GasNozzle : public Weapon {
private:
  float m_coneRange = 5.5f;
  float m_coneHalfAngle = 35.0f;
  float m_tickInterval = 0.15f;
  AnimationState<void> m_tickCooldown{0.15f};

public:
  GasNozzle() : Weapon(0.05f, 12.0f) {
    m_id = "gas_nozzle_e20";
    m_name = "Gas Nozzle (E20)";
    m_description = "Sprays E20 gas in a cone.";
    m_iconName = "icon_e20_gas_nozzle";
    m_maxLevel = 8;
  }

  std::string getLevelDescription(uint32_t level) const override {
    switch (level) {
    case 1:
      return "Sprays E20 gas in a cone.";
    case 2:
      return "Cooldown -10%.";
    case 3:
      return "Area +10%.";
    case 4:
      return "Damage +3.";
    case 5:
      return "Cooldown -10%.";
    case 6:
      return "Area +10%.";
    case 7:
      return "Damage +3.";
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
      m_coneRange *= 1.1f;
      break;
    case 4:
      m_baseDamage += 3.0f;
      break;
    case 5:
      setBaseCooldown(getBaseCooldown() * 0.9f);
      break;
    case 6:
      m_coneRange *= 1.1f;
      break;
    case 7:
      m_baseDamage += 3.0f;
      break;
    case 8:
      m_coneRange *= 1.15f;
      break;
    default:
      m_baseDamage *= 1.1f;
      break;
    }
  }

  bool fire() override { return true; }

  void update(double delta_time) override {
    float dt = static_cast<float>(delta_time);
    m_tickCooldown.updateTimer(dt);

    if (m_tickCooldown.isFinished()) {
      glm::vec3 player_pos = m_context.ensureInitialized()->getPlayerPosition();
      glm::vec3 player_forward =
          m_context.ensureInitialized()->getPlayerForward();

      glm::vec3 normalized_forward = player_forward;
      float forward_length = glm::length(normalized_forward);
      if (forward_length < 0.001f) {
        m_tickCooldown.reset();
        m_tickInterval = getCooldown();
        m_tickCooldown.duration = m_tickInterval;
        return;
      }
      normalized_forward = glm::normalize(normalized_forward);

      float area_multiplier =
          m_context.ensureInitialized()->getStats()->getMultiplier(
              StatType::AREA);
      float cone_range = m_coneRange * (1.0f + area_multiplier * 0.12f);
      float cone_half_angle = m_coneHalfAngle + area_multiplier * 2.0f;
      float cos_cone_angle = std::cos(glm::radians(cone_half_angle));

      emitParticle(GameEvents::ParticleSpawnRequestedEvent{
          .position = player_pos + (normalized_forward * 1.0f),
          .direction = normalized_forward,
          .length = cone_range,
          .thickness = glm::max(0.18f, cone_range * 0.12f),
          .effectId = GameEvents::ParticleEffectType::FLAME,
      });

      m_context.ensureInitialized()->findTargets(
          cone_range, 1000, [&](Enemy *enemy) {
            if (!enemy || enemy->isRemovalRequested()) {
              return;
            }

            const Player *player = m_context.ensureInitialized()->getPlayer();

            AABB player_box = player->getHitboxAABB();
            AABB enemy_box = enemy->getHitboxAABB();

            glm::vec3 closest_on_player = glm::clamp(
                enemy_box.getCenter(), player_box.min, player_box.max);
            glm::vec3 closest_on_enemy =
                glm::clamp(closest_on_player, enemy_box.min, enemy_box.max);

            glm::vec3 to_closest = closest_on_enemy - closest_on_player;
            float dist_to_enemy = glm::length(to_closest);

            if (dist_to_enemy > cone_range)
              return;

            glm::vec3 direction_to_closest = glm::normalize(to_closest);
            if (glm::dot(normalized_forward, direction_to_closest) <
                cos_cone_angle)
              return;

            float falloff = 1.0f - (dist_to_enemy / cone_range);
            emitEnemyDamage(GameEvents::EnemyDamageRequestedEvent{
                .enemy = enemy,
                .amount = getDamage() * (0.65f + falloff * 0.35f),
                .isCritical = false,
                .knockbackDirection = normalized_forward,
                .knockbackStrength = 0.5f + falloff * 0.35f,
                .hitPosition = closest_on_enemy + glm::vec3(0.0f, 1.0f, 0.0f),
                .hitEffect = GameEvents::ParticleEffectType::FLAME,
            });
          });

      m_tickCooldown.reset();
      m_tickInterval = getCooldown();
      m_tickCooldown.duration = m_tickInterval;
    }
  }
};
