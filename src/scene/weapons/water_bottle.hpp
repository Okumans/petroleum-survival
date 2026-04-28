#pragma once

#include "scene/game_factories.hpp"
#include "scene/projectile.hpp"
#include "scene/weapon/projectile_weapon.hpp"
#include "utility/random.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <memory>

class WaterBottle : public ProjectileWeapon {
private:
  float m_range = 15.0f;
  float m_projectileSpeed = 12.0f;
  float m_projectileLifetime = 2.5f;
  float m_gravity = -18.0f;          // Units/s^2
  float m_spreadAngleDegrees = 8.0f; // Small horizontal variance

public:
  WaterBottle() : ProjectileWeapon(1.5f, 0.1f, 15.0f, 1) {
    m_id = "water_bottle";
    m_name = "Water Bottle";
    m_description = "Throws a water bottle in an arc.";
    m_iconName = "icon_water_bottle";
    m_maxLevel = 8;
  }

  std::string getLevelDescription(uint32_t level) const override {
    switch (level) {
    case 1:
      return "Throws a water bottle forward.";
    case 2:
      return "Cooldown -10%.";
    case 3:
      return "Damage +10.";
    case 4:
      return "+1 bottle.";
    case 5:
      return "Cooldown -10%.";
    case 6:
      return "Damage +10.";
    case 7:
      return "+1 bottle.";
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
      m_amount += 1;
      break;
    case 5:
      setBaseCooldown(getBaseCooldown() * 0.9f);
      break;
    case 6:
      m_baseDamage += 10.0f;
      break;
    case 7:
      m_amount += 1;
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
    glm::vec3 player_pos = m_context.ensureInitialized()->getPlayerPosition();

    auto targets = acquireTargets(m_range, getAmount());
    if (targets.empty()) {
      return false;
    }

    Enemy *target =
        targets[Random::randInt(0, static_cast<int>(targets.size()) - 1)];

    AABB target_box = target->getHitboxAABB();
    float center_y_offset = (target_box.max.y + target_box.min.y) * 0.5f;
    glm::vec3 target_pos =
        target->getPosition() + glm::vec3(0.0f, center_y_offset, 0.0f);

    glm::vec3 to_target = target_pos - player_pos;
    float dist = glm::length(to_target);
    if (dist < 0.001f) {
      return false;
    }

    glm::vec3 dir = to_target / dist;
    float random_yaw_rad = glm::radians(
        Random::randFloat(-m_spreadAngleDegrees, m_spreadAngleDegrees));

    // Cleaner GLM rotation around the Y-axis
    glm::vec3 spread_dir =
        glm::angleAxis(random_yaw_rad, glm::vec3(0.0f, 1.0f, 0.0f)) * dir;

    glm::vec3 spawn_pos = player_pos + spread_dir * 0.6f;

    float up_boost = glm::clamp(dist * 0.18f, 2.0f, 6.0f);
    glm::vec3 velocity =
        (spread_dir * m_projectileSpeed) + glm::vec3(0.0f, up_boost, 0.0f);

    const glm::vec3 spin_deg_per_sec =
        glm::vec3(Random::randFloat(-90.0f, 90.0f),
                  Random::randFloat(-1080.0f, 1080.0f), // Y-axis: Yaw spin
                  Random::randFloat(-720.0f, 720.0f)    // Z-axis: Roll spin
        );

    std::shared_ptr<Projectile> proj = std::make_shared<Projectile>(
        GameFactories::getProjectile(ModelName::WATER_BOTTLE)
            .create([&](Projectile &p) {
              p.setPosition(spawn_pos);
              p.setVelocity(velocity);
              p.setDamage(getDamage());
              p.setLifetime(m_projectileLifetime);
              p.setRotation(glm::vec3(Random::randFloat(0.0f, 360.0f),
                                      Random::randFloat(0.0f, 360.0f),
                                      Random::randFloat(0.0f, 360.0f)));
              p.setScale(glm::vec3(5.0f));

              // Lighter capture payload to avoid heap allocations
              p.setUpdateLogic([spin = spin_deg_per_sec,
                                gravity = m_gravity](Projectile &p, double dt) {
                float fdt = static_cast<float>(dt);
                glm::vec3 vel = p.getVelocity();

                vel.y += gravity * fdt;
                p.setVelocity(vel);

                p.translate(vel * fdt);
                p.rotate(spin * fdt);
              });
            }));

    emitProjectile(
        GameEvents::ProjectileSpawnRequestedEvent{.projectile = proj});
    return true;
  }
};
