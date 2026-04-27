#pragma once

#include "graphics/animation_state.hpp"
#include "scene/enemy/enemy.hpp"
#include "scene/game_factories.hpp"
#include "scene/projectile.hpp"
#include "scene/weapon/weapon.hpp"
#include "utility/not_initialized.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

class OrbitingCones : public Weapon {
private:
  struct OrbitingCone {
    std::shared_ptr<Projectile> projectile;
    float angle;       // Current angle in radians
    float orbitRadius; // Distance from player
    std::unordered_map<Enemy *, float>
        lastHitTime; // Track per-enemy damage cooldown
  };

  std::vector<OrbitingCone> m_orbitingCones;
  float m_orbitSpeed = 4.5f;                      // Radians per second
  float m_orbitRadius = 3.0f;                     // Distance from player
  uint32_t m_coneCount = 3;                       // Number of cones
  float m_coneHitRadius = 0.6f;                   // Collision radius for damage
  float m_damageCooldown = 0.3f;                  // Seconds between hits
  AnimationState<void> m_spawnCooldown{0.5f};     // Cooldown for spawning
  NotInitialized<std::function<void(float range, uint32_t k,
                                    std::function<void(Enemy *)>)>>
      m_findTargets;

public:
  OrbitingCones() : Weapon(0.5f, 18.0f) {}

  bool fire(const glm::vec3 &playerPos,
            const glm::vec3 &playerForward) override {
    (void)playerForward;
    if (!m_spawnProjectile)
      return false;

    // Spawn initial orbiting cones
    for (uint32_t i = 0; i < m_coneCount; ++i) {
      float angle =
          (glm::pi<float>() * 2.0f) * (i / static_cast<float>(m_coneCount));
      spawnConeAtAngle(playerPos, angle);
    }

    return true;
  }

  void update(double delta_time, const glm::vec3 &playerPos,
              const glm::vec3 &playerForward) override {
    (void)playerForward;

    float dt = static_cast<float>(delta_time);

    // Update spawn cooldown and respawn if needed
    m_spawnCooldown.updateTimer(dt);
    if (m_spawnCooldown.isFinished() && m_orbitingCones.size() < m_coneCount) {
      if (fire(playerPos, playerForward)) {
        m_spawnCooldown.reset();
        m_spawnCooldown.duration = getCooldown();
      }
    }

    std::vector<Enemy *> nearbyEnemies;
    m_findTargets.ensureInitialized()(m_orbitRadius + 6.0f, 100,
                                      [&](Enemy *enemy) {
                                        if (enemy)
                                          nearbyEnemies.push_back(enemy);
                                      });

    // Update orbit positions and apply per-cone damage
    for (auto &cone : m_orbitingCones) {
      if (!cone.projectile)
        continue;

      // Increment angle
      cone.angle += m_orbitSpeed * dt;
      if (cone.angle > glm::pi<float>() * 2.0f)
        cone.angle -= glm::pi<float>() * 2.0f;

      // Update position relative to player
      glm::vec3 newPos =
          playerPos + glm::vec3(std::cos(cone.angle) * cone.orbitRadius, 0.2f,
                                std::sin(cone.angle) * cone.orbitRadius);
      cone.projectile->setPosition(newPos);

      // Clean up expired hit cooldowns
      for (auto it = cone.lastHitTime.begin(); it != cone.lastHitTime.end();) {
        it->second -= dt;
        if (it->second <= 0.0f)
          it = cone.lastHitTime.erase(it);
        else
          ++it;
      }

      // Apply damage through central event path.
      for (Enemy *enemy : nearbyEnemies) {
        if (!enemy || enemy->isRemovalRequested())
          continue;

        glm::vec3 toEnemy = enemy->getPosition() - newPos;
        float distance = glm::length(toEnemy);
        if (distance > m_coneHitRadius)
          continue;

        auto hitIt = cone.lastHitTime.find(enemy);
        if (hitIt != cone.lastHitTime.end() && hitIt->second > 0.0f)
          continue;

        glm::vec3 knockbackDir =
            (distance < 0.001f) ? glm::vec3(0.0f, 0.0f, 1.0f)
                                : glm::normalize(toEnemy);

        emitEnemyDamage(GameEvents::EnemyDamageRequestedEvent{
            .enemy = enemy,
            .amount = getDamage(),
            .isCritical = false,
            .knockbackDirection = knockbackDir,
            .knockbackStrength = 1.0f,
            .hitPosition = enemy->getPosition() + glm::vec3(0.0f, 1.0f, 0.0f),
            .hitEffect = GameEvents::ParticleEffectType::MAGIC_HIT,
        });

        cone.lastHitTime[enemy] = m_damageCooldown;
      }
    }

    // Remove dead projectiles from tracking
    m_orbitingCones.erase(std::remove_if(m_orbitingCones.begin(),
                                         m_orbitingCones.end(),
                                         [](const OrbitingCone &cone) {
                                           return !cone.projectile ||
                                                  cone.projectile->isRemovalRequested();
                                         }),
                          m_orbitingCones.end());
  }

  void setTargetingContext(
      std::function<void(float range, uint32_t k, std::function<void(Enemy *)>)>
          findTargets) {
    m_findTargets.init(findTargets);
  }

private:
  void spawnConeAtAngle(const glm::vec3 &playerPos, float angle) {
    glm::vec3 spawnPos =
        playerPos + glm::vec3(std::cos(angle) * m_orbitRadius, 0.2f,
                              std::sin(angle) * m_orbitRadius);

    std::shared_ptr<Projectile> proj = std::make_shared<Projectile>(
        GameFactories::getProjectile(ModelName::TRAFFIC_CONE)
            .create([&](Projectile &p) {
              p.setPosition(spawnPos);
              p.setVelocity(
                  glm::vec3(0.0f)); // Velocity managed by orbit logic
              p.setDamage(getDamage());
              p.setLifetime(
                  100.0f); // Very long lifetime (managed externally)
              p.setScale(glm::vec3(0.5f));
              p.copyModel()->setEmissionColor(glm::vec3(1.0f, 0.5f, 0.0f) *
                                              0.8f);
            }));

    m_spawnProjectile(
        GameEvents::ProjectileSpawnRequestedEvent{.projectile = proj});

    m_orbitingCones.push_back({proj, angle, m_orbitRadius, {}});
  }
};
