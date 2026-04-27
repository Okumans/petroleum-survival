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

  bool fire(const glm::vec3 &playerPos,
            const glm::vec3 &playerForward) override {
    if (!m_spawnProjectile)
      return false;

    float randomYawDeg =
        Random::randFloat(-m_spreadAngleDegrees, m_spreadAngleDegrees);
    float randomYawRad = glm::radians(randomYawDeg);

    glm::vec3 spreadForward;
    spreadForward.x = playerForward.x * std::cos(randomYawRad) -
                      playerForward.z * std::sin(randomYawRad);
    spreadForward.y = playerForward.y;
    spreadForward.z = playerForward.x * std::sin(randomYawRad) +
                      playerForward.z * std::cos(randomYawRad);
    spreadForward = glm::normalize(spreadForward);

    glm::vec3 spawnPos = playerPos + spreadForward * 0.5f;
    glm::vec3 velocity = spreadForward * m_projectileSpeed;

    std::shared_ptr<Projectile> proj = std::make_shared<Projectile>(
        GameFactories::getProjectile(ModelName::WATER_BOTTLE)
            .create([&](Projectile &p) {
              p.setPosition(spawnPos);
              p.setVelocity(velocity);
              p.setDamage(getDamage());
              p.setLifetime(m_projectileLifetime);
              p.setUpdateLogic([](Projectile &p, double dt) {
                p.translate(p.getVelocity() * static_cast<float>(dt));
              });
              p.setScale(glm::vec3(0.005f));
            }));

    m_spawnProjectile(
        GameEvents::ProjectileSpawnRequestedEvent{.projectile = proj});

    return true;
  }
};
