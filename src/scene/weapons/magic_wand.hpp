#pragma once

#include "scene/game_factories.hpp"
#include "scene/enemy/enemy.hpp"
#include "scene/projectile.hpp"
#include "scene/weapon/projectile_weapon.hpp"
#include "utility/random.hpp"
#include <memory>
#include <vector>

class MagicWand : public ProjectileWeapon {

private:
  float m_range = 15.0f;
  float m_projectileSpeed = 10.0f;
  float m_projectileLifetime = 2.0f;

public:
  MagicWand() : ProjectileWeapon(1.0f, 0.05f, 25.0f, 3) {}

  bool fire() override {
    auto targets = acquireTargets(m_range, getAmount());

    if (targets.empty()) {
      return false;
    }

    Enemy *target =
        targets[Random::randInt(0, static_cast<int>(targets.size()) - 1)];

    glm::vec3 spawn_pos = m_context.ensureInitialized()->getPlayerPosition();

    AABB target_box = target->getHitboxAABB();
    glm::vec3 target_pos =
        target->getPosition() +
        glm::vec3(0.0f, (target_box.max.y - target_box.min.y) * 0.5f, 0.0f);

    glm::vec3 to_target = target_pos - spawn_pos;

    if (glm::length(to_target) < 0.001f) {
      return false;
    }

    glm::vec3 velocity = glm::normalize(to_target) * m_projectileSpeed;

    std::shared_ptr<Projectile> proj = std::make_shared<Projectile>(
        GameFactories::getProjectile().create([&](Projectile &p) {
          p.setPosition(spawn_pos);
          p.setVelocity(velocity);
          p.setDamage(getDamage());
          p.setLifetime(m_projectileLifetime);
          p.setUpdateLogic([](Projectile &p, double dt) {
            p.translate(p.getVelocity() * static_cast<float>(dt));
          });
          p.setScale(glm::vec3(0.2f));
          // Sphere color
          p.copyModel()->setEmissionColor(glm::vec3(2.0f, 20.0f, 40.0f) * 0.5f);
        }));

    m_context.ensureInitialized()->emit(
        GameEvents::ProjectileSpawnRequestedEvent{.projectile = proj});

    return true;
  }
};
