#pragma once

#include "resource/model_manager.hpp"
#include "scene/enemy.hpp"
#include "scene/projectile.hpp"
#include "scene/weapon/projectile_weapon.hpp"
#include "utility/random.hpp"
#include <vector>

class MagicWand : public ProjectileWeapon {
private:
  float m_range = 15.0f;
  float m_projectileSpeed = 10.0f;
  float m_projectileLifetime = 2.0f;

  std::function<std::vector<Enemy *>(glm::vec3, float, uint32_t)>
      m_findClosestEnemies;

public:
  MagicWand()
      : ProjectileWeapon(1.0f, 0.05f, 25.0f, 100) {
  } // 1 attack per sec, 25 damage, 5 proj

  void setTargetingContext(
      std::function<std::vector<Enemy *>(glm::vec3, float, uint32_t)>
          findClosestEnemies) {
    m_findClosestEnemies = findClosestEnemies;
  }

  bool fire(const glm::vec3 &playerPos) override {
    if (!m_findClosestEnemies || !m_spawnProjectile)
      return false;

    auto targets = m_findClosestEnemies(playerPos, m_range, getAmount());

    if (targets.empty())
      return false;

    Enemy *target =
        targets[Random::randInt(0, static_cast<int>(targets.size()) - 1)];

    glm::vec3 spawnPos =
        playerPos; // playerPos is already passed as the centered spawn position

    AABB targetBox = target->getHitboxAABB();
    glm::vec3 targetPos =
        target->getPosition() +
        glm::vec3(0.0f, (targetBox.max.y - targetBox.min.y) * 0.5f, 0.0f);

    glm::vec3 toTarget = targetPos - spawnPos;

    if (glm::length(toTarget) < 0.001f)
      return false;

    glm::vec3 velocity = glm::normalize(toTarget) * m_projectileSpeed;

    auto model = ModelManager::copy(ModelName::SPHERE);
    model->setEmissionColor(glm::vec3(2.0f, 20.0f, 40.0f) *
                            0.5f); // Bright blue-ish glow

    Projectile proj(model, spawnPos, velocity, getDamage(),
                    m_projectileLifetime, [](Projectile &p, double dt) {
                      p.translate(p.getVelocity() * static_cast<float>(dt));
                    });
    proj.setScale(glm::vec3(0.2f));

    m_spawnProjectile(
        GameEvents::ProjectileSpawnRequestedEvent{.projectile = proj});

    return true;
  }
};
