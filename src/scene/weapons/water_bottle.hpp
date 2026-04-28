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
  float m_projectileSpeed = 12.0f;
  float m_projectileLifetime = 2.5f;
  float m_spreadAngleDegrees = 10.0f; // Random spread: ±10 degrees

public:
  WaterBottle()
      : ProjectileWeapon(0.8f, 0.05f, 12.0f, 1) {
  } // 0.8s cooldown, 0.05s sub-cooldown, 12 damage, 1 projectile per shot

  bool fire() override {
    glm::vec3 player_pos = m_context.ensureInitialized()->getPlayerPosition();
    glm::vec3 player_forward =
        m_context.ensureInitialized()->getPlayerForward();

    float random_yaw_deg =
        Random::randFloat(-m_spreadAngleDegrees, m_spreadAngleDegrees);
    float random_yaw_rad = glm::radians(random_yaw_deg);

    glm::vec3 spread_forward;
    spread_forward.x = player_forward.x * std::cos(random_yaw_rad) -
                       player_forward.z * std::sin(random_yaw_rad);
    spread_forward.y = player_forward.y;
    spread_forward.z = player_forward.x * std::sin(random_yaw_rad) +
                       player_forward.z * std::cos(random_yaw_rad);
    spread_forward = glm::normalize(spread_forward);

    glm::vec3 spawn_pos = player_pos + spread_forward * 0.5f;
    glm::vec3 velocity = spread_forward * m_projectileSpeed;

    std::shared_ptr<Projectile> proj = std::make_shared<Projectile>(
        GameFactories::getProjectile(ModelName::WATER_BOTTLE)
            .create([&](Projectile &p) {
              p.setPosition(spawn_pos);
              p.setVelocity(velocity);
              p.setDamage(getDamage());
              p.setLifetime(m_projectileLifetime);
              p.setUpdateLogic([](Projectile &p, double dt) {
                p.translate(p.getVelocity() * static_cast<float>(dt));
              });
              p.setScale(glm::vec3(0.005f));
            }));

    emitProjectile(
        GameEvents::ProjectileSpawnRequestedEvent{.projectile = proj});

    return true;
  }
};
