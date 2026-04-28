#pragma once

#include "game/game_events.hpp"
#include "scene/game_factories.hpp"
#include "scene/projectile.hpp"
#include "scene/weapon/projectile_weapon.hpp"
#include "utility/random.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <string>

class Lighter : public ProjectileWeapon {
private:
  float m_projectileSpeed = 11.0f;
  float m_projectileLifetime = 1.4f;
  float m_spreadAngleDegrees = 6.0f;

public:
  Lighter() : ProjectileWeapon(1.0f, 0.08f, 12.0f, 1) {
    m_id = "lighter";
    m_name = "Lighter";
    m_description = "Fires small sparks forward.";
    m_iconName = "icon_lighter";
    m_maxLevel = 8;
  }

  std::string getLevelDescription(uint32_t level) const override {
    switch (level) {
    case 1:
      return "Fires small sparks forward.";
    case 2:
      return "Cooldown -10%.";
    case 3:
      return "Damage +6.";
    case 4:
      return "+1 spark.";
    case 5:
      return "Cooldown -10%.";
    case 6:
      return "Damage +6.";
    case 7:
      return "+1 spark.";
    case 8:
      return "Damage +10.";
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
      m_baseDamage += 6.0f;
      break;
    case 4:
      m_amount += 1;
      break;
    case 5:
      setBaseCooldown(getBaseCooldown() * 0.9f);
      break;
    case 6:
      m_baseDamage += 6.0f;
      break;
    case 7:
      m_amount += 1;
      break;
    case 8:
      m_baseDamage += 10.0f;
      break;
    default:
      m_baseDamage *= 1.1f;
      break;
    }
  }

  bool fire() override {
    glm::vec3 player_pos = m_context.ensureInitialized()->getPlayerPosition();
    glm::vec3 forward = m_context.ensureInitialized()->getPlayerForward();

    float forward_len = glm::length(forward);
    if (forward_len < 0.001f) {
      return false;
    }
    forward = forward / forward_len;

    float random_yaw_deg =
        Random::randFloat(-m_spreadAngleDegrees, m_spreadAngleDegrees);
    float random_yaw_rad = glm::radians(random_yaw_deg);

    glm::vec3 dir;
    dir.x = forward.x * std::cos(random_yaw_rad) - forward.z * std::sin(random_yaw_rad);
    dir.y = 0.0f;
    dir.z = forward.x * std::sin(random_yaw_rad) + forward.z * std::cos(random_yaw_rad);
    float dir_len = glm::length(dir);
    if (dir_len < 0.001f) {
      return false;
    }
    dir /= dir_len;

    glm::vec3 spawn_pos = player_pos + dir * 0.8f + glm::vec3(0.0f, 0.2f, 0.0f);
    glm::vec3 velocity = dir * m_projectileSpeed;

    std::shared_ptr<Projectile> proj = std::make_shared<Projectile>(
        GameFactories::getProjectile().create([&](Projectile &p) {
          p.setPosition(spawn_pos);
          p.setVelocity(velocity);
          p.setDamage(getDamage());
          p.setLifetime(m_projectileLifetime);
          p.setScale(glm::vec3(0.18f));
          p.copyModel()->setEmissionColor(glm::vec3(8.0f, 4.0f, 1.0f) * 0.5f);
        }));

    emitProjectile(GameEvents::ProjectileSpawnRequestedEvent{.projectile = proj});

    emitParticle(GameEvents::ParticleSpawnRequestedEvent{
        .position = spawn_pos,
        .direction = dir,
        .length = 0.6f,
        .thickness = 0.16f,
        .effectId = GameEvents::ParticleEffectType::FLAME,
    });

    return true;
  }
};

