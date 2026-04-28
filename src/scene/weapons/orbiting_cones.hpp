#pragma once

#include "graphics/animation_state.hpp"
#include "scene/enemy/enemy.hpp"
#include "scene/game_factories.hpp"
#include "scene/game_object_manager.hpp"
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
    std::shared_ptr<ObjectHandle> projectileHandle;
    float angle;       // Current angle in radians
    float orbitRadius; // Distance from player
    std::unordered_map<Enemy *, float>
        lastHitTime; // Track per-enemy damage cooldown
  };

  std::vector<OrbitingCone> m_orbitingCones;
  float m_orbitSpeed = 4.5f;                  // Radians per second
  float m_orbitRadius = 3.0f;                 // Distance from player
  uint32_t m_coneCount = 3;                   // Number of cones
  float m_coneHitRadius = 0.6f;               // Collision radius for damage
  float m_damageCooldown = 0.3f;              // Seconds between hits
  AnimationState<void> m_spawnCooldown{0.5f}; // Cooldown for spawning

public:
  OrbitingCones() : Weapon(0.5f, 18.0f) {
    m_id = "orbiting_cones";
    m_name = "Orbiting Cones";
    m_description = "Traffic cones orbit around you.";
    m_iconName = "icon_cone";
    m_maxLevel = 8;
  }

  virtual bool canCrit() const override { return false; }

  std::string getLevelDescription(uint32_t level) const override {
    switch (level) {
    case 1:
      return "Traffic cones orbit around you.";
    case 2:
      return "Cooldown -10%.";
    case 3:
      return "Damage +5.";
    case 4:
      return "+1 cone.";
    case 5:
      return "Cooldown -10%.";
    case 6:
      return "Damage +5.";
    case 7:
      return "Area +10%.";
    case 8:
      return "+1 cone.";
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
      m_baseDamage += 5.0f;
      break;
    case 4:
      m_coneCount += 1;
      break;
    case 5:
      setBaseCooldown(getBaseCooldown() * 0.9f);
      break;
    case 6:
      m_baseDamage += 5.0f;
      break;
    case 7:
      m_orbitRadius *= 1.1f;
      break;
    case 8:
      m_coneCount += 1;
      break;
    default:
      m_baseDamage *= 1.1f;
      break;
    }
  }

  bool fire() override {
    glm::vec3 player_pos = m_context.ensureInitialized()->getPlayerPosition();

    // Spawn initial orbiting cones
    for (uint32_t i = 0; i < m_coneCount; ++i) {
      float angle =
          (glm::pi<float>() * 2.0f) * (i / static_cast<float>(m_coneCount));
      spawnConeAtAngle(player_pos, angle);
    }

    return true;
  }

  void update(double delta_time) override {
    float dt = static_cast<float>(delta_time);

    glm::vec3 player_pos = m_context.ensureInitialized()->getPlayerPosition();

    // Update spawn cooldown and respawn if needed
    m_spawnCooldown.updateTimer(dt);
    if (m_spawnCooldown.isFinished() && m_orbitingCones.size() < m_coneCount) {
      if (fire()) {
        m_spawnCooldown.reset();
        m_spawnCooldown.duration = getCooldown();
      }
    }

    // Update orbit positions and apply per-cone damage
    for (auto &cone : m_orbitingCones) {
      if (!cone.projectileHandle || !cone.projectileHandle->isValid()) {
        continue;
      }

      GameObject *obj =
          m_context.ensureInitialized()->resolveHandle(*cone.projectileHandle);
      Projectile *live_projectile = dynamic_cast<Projectile *>(obj);

      if (!live_projectile || live_projectile->isRemovalRequested()) {
        continue;
      }

      // Increment angle
      cone.angle += m_orbitSpeed * dt;
      if (cone.angle > glm::pi<float>() * 2.0f) {
        cone.angle -= glm::pi<float>() * 2.0f;
      }

      // Update position relative to player
      glm::vec3 new_pos =
          player_pos + glm::vec3(std::cos(cone.angle) * cone.orbitRadius, 0.2f,
                                 std::sin(cone.angle) * cone.orbitRadius);
      live_projectile->setPosition(new_pos);

      // Clean up expired hit cooldowns
      for (auto it = cone.lastHitTime.begin(); it != cone.lastHitTime.end();) {
        it->second -= dt;
        if (it->second <= 0.0f) {
          it = cone.lastHitTime.erase(it);
        } else {
          ++it;
        }
      }

      m_context.ensureInitialized()->findTargets(
          m_orbitRadius + 6.0f, 100, [&](Enemy *enemy) {
            if (!enemy || enemy->isRemovalRequested()) {
              return;
            }

            glm::vec3 to_enemy = enemy->getPosition() - new_pos;
            float distance = glm::length(to_enemy);
            if (distance > m_coneHitRadius) {
              return;
            }

            auto hit_it = cone.lastHitTime.find(enemy);
            if (hit_it != cone.lastHitTime.end() && hit_it->second > 0.0f) {
              return;
            }

            glm::vec3 knockback_dir = (distance < 0.001f)
                                          ? glm::vec3(0.0f, 0.0f, 1.0f)
                                          : glm::normalize(to_enemy);

            auto dmg = calculateDamage();
            emitEnemyDamage(GameEvents::EnemyDamageRequestedEvent{
                .enemy = enemy,
                .amount = dmg.amount,
                .isCritical = dmg.isCritical,
                .knockbackDirection = knockback_dir,
                .knockbackStrength = 1.0f,
                .hitPosition =
                    enemy->getPosition() + glm::vec3(0.0f, 1.0f, 0.0f),
                .hitEffect = GameEvents::ParticleEffectType::MAGIC_HIT,
            });

            cone.lastHitTime[enemy] = m_damageCooldown;
          });
    }

    // Remove dead projectiles from tracking
    m_orbitingCones.erase(
        std::remove_if(
            m_orbitingCones.begin(), m_orbitingCones.end(),
            [this](const OrbitingCone &cone) {
              if (!cone.projectileHandle || !cone.projectileHandle->isValid()) {
                return false;
              }

              GameObject *obj = m_context.ensureInitialized()->resolveHandle(
                  *cone.projectileHandle);
              Projectile *live_projectile = dynamic_cast<Projectile *>(obj);
              return !live_projectile || live_projectile->isRemovalRequested();
            }),
        m_orbitingCones.end());
  }

private:
  void spawnConeAtAngle(const glm::vec3 &player_pos, float angle) {
    glm::vec3 spawn_pos =
        player_pos + glm::vec3(std::cos(angle) * m_orbitRadius, 0.2f,
                               std::sin(angle) * m_orbitRadius);

    auto spawned_handle = std::make_shared<ObjectHandle>();

    std::shared_ptr<Projectile> proj = std::make_shared<Projectile>(
        GameFactories::getProjectile(ModelName::TRAFFIC_CONE)
            .create([&](Projectile &p) {
              p.setPosition(spawn_pos);
              p.setVelocity(glm::vec3(0.0f));
              p.setDamage(getDamage());
              p.setLifetime(100.0f);
              p.setScale(glm::vec3(0.05f));
              p.setHitboxScaleFactor({1.0f, 5.0f, 1.0f});
            }));

    emitProjectile(GameEvents::ProjectileSpawnRequestedEvent{
        .projectile = proj,
        .onSpawned =
            [spawned_handle](const ObjectHandle &handle) {
              *spawned_handle = handle;
            },
    });

    m_orbitingCones.push_back({spawned_handle, angle, m_orbitRadius, {}});
  }
};
