#pragma once

#include "game/game_events.hpp"
#include "graphics/animation_state.hpp"
#include "resource/model_manager.hpp"
#include "scene/enemy.hpp"
#include "scene/projectile.hpp"
#include <functional>
#include <glm/glm.hpp>
#include <print>

class Weapon {
protected:
  AnimationState<void> m_coolDownState;
  float m_damage;

  std::function<Enemy *(glm::vec3, float)> m_findClosestEnemy;
  std::function<void(const GameEvents::ProjectileSpawnRequestedEvent &)>
      m_spawnProjectile;

public:
  Weapon(float cooldown, float damage)
      : m_coolDownState(cooldown), m_damage(damage) {}

  virtual ~Weapon() = default;

  void setContext(
      std::function<Enemy *(glm::vec3, float)> findClosestEnemy,
      std::function<void(const GameEvents::ProjectileSpawnRequestedEvent &)>
          spawnProjectile) {
    m_findClosestEnemy = findClosestEnemy;
    m_spawnProjectile = spawnProjectile;
  }

  virtual void update(double delta_time, const glm::vec3 &playerPos) {
    m_coolDownState.updateTimer(static_cast<float>(delta_time));

    if (m_coolDownState.isFinished()) {
      if (fire(playerPos)) {
        m_coolDownState.reset();
      }
    }
  }

  // Returns true if successfully fired
  virtual bool fire(const glm::vec3 &playerPos) = 0;
};

class MagicWand : public Weapon {
private:
  float m_range = 15.0f;
  float m_projectileSpeed = 10.0f;
  float m_projectileLifetime = 2.0f;

public:
  MagicWand() : Weapon(1.0f, 25.0f) {} // 1 attack per sec, 25 damage

  bool fire(const glm::vec3 &playerPos) override {
    if (!m_findClosestEnemy || !m_spawnProjectile)
      return false;

    Enemy *target = m_findClosestEnemy(playerPos, m_range);

    if (!target)
      return false;

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

    m_spawnProjectile(GameEvents::ProjectileSpawnRequestedEvent{
        .position = spawnPos,
        .velocity = velocity,
        .damage = m_damage,
        .lifetime = m_projectileLifetime,
        .modelName = ModelName::SPHERE,
        .scale = glm::vec3(0.5f),
        .behaviorCallback = [](Projectile &proj, double dt) {
          // Straight line movement, with a slight vertical bobbing effect
          proj.translate(proj.getVelocity() * static_cast<float>(dt));

          // Example of advanced path calculation:
          // Add a small sine wave offset to the Y coordinate to make it look
          // like a flying orb float timeAlive = 2.0f - proj.m_lifetime; //
          // pseudo time We can just use the straightforward translation for
          // now, but behaviorCallback is available!
        }});

    return true;
  }
};
