#pragma once

#include "scene/game_factories.hpp"
#include "scene/weapon/weapon.hpp"
#include "scene/weapons/melee_projectile.hpp"
#include <glm/glm.hpp>

class SolidWoodBlock : public Weapon {
private:
  float m_meleeRange;
  float m_lifetime;

public:
  // Base stats: 1.5s cooldown, 10 damage, 2.0 range, 0.2s sweep duration
  SolidWoodBlock(float cooldown = 1.5f, float damage = 10.0f,
                 float range = 2.0f, float sweep_duration = 0.2f)
      : Weapon(cooldown, damage), m_meleeRange(range),
        m_lifetime(sweep_duration) {}

  bool fire(const glm::vec3 &playerPos,
            const glm::vec3 &playerForward) override {
    if (!m_spawnProjectile)
      return false;

    // Spawn the hitbox just in front of the player
    glm::vec3 spawnPos = playerPos + (playerForward * (m_meleeRange * 0.5f));

    std::shared_ptr<Projectile> proj = std::make_shared<MeleeProjectile>(
        GameFactories::getMeleeProjectile().create([&](MeleeProjectile &p) {
          p.setPosition(spawnPos);
          p.setVelocity(playerForward * 0.0f); // stays relative
          p.setDamage(getDamage());
          p.setMaxLifetime(m_lifetime);
          p.setScale(glm::vec3(m_meleeRange, 1.0f, m_meleeRange * 0.5f));
        }));

    m_spawnProjectile(
        GameEvents::ProjectileSpawnRequestedEvent{.projectile = proj});

    return true;
  }
};
