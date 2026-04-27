#pragma once

#include "game/game_events.hpp"
#include "game/stat_manager.hpp"
#include "graphics/animation_state.hpp"
#include "resource/model_manager.hpp"
#include "scene/enemy.hpp"
#include "scene/projectile.hpp"
#include "utility/random.hpp"
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <print>
#include <vector>

class Weapon {
protected:
  AnimationState<void> m_coolDownState;
  float m_baseCooldown;
  float m_baseDamage;
  const StatManager *m_stats = nullptr;

  std::function<void(const GameEvents::ProjectileSpawnRequestedEvent &)>
      m_spawnProjectile;

public:
  Weapon(float cooldown, float damage)
      : m_coolDownState(cooldown), m_baseCooldown(cooldown),
        m_baseDamage(damage) {}

  virtual ~Weapon() = default;

  void setStats(const StatManager *stats) { m_stats = stats; }

  float getDamage() const {
    return m_baseDamage *
           (m_stats ? m_stats->getMultiplier(StatType::MIGHT) : 1.0f);
  }

  float getCooldown() const {
    return m_baseCooldown *
           (m_stats ? m_stats->getMultiplier(StatType::COOLDOWN) : 1.0f);
  }

  void setContext(
      std::function<void(const GameEvents::ProjectileSpawnRequestedEvent &)>
          spawnProjectile) {
    m_spawnProjectile = spawnProjectile;
  }

  virtual void update(double delta_time, const glm::vec3 &playerPos) {
    m_coolDownState.updateTimer(static_cast<float>(delta_time));

    if (m_coolDownState.isFinished()) {
      if (fire(playerPos)) {
        m_coolDownState.reset();
        m_coolDownState.duration = getCooldown();
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
  uint32_t m_amount = 5;

  // State for multiple shoot on one cooldown
  uint32_t m_amountShoot = 0;
  AnimationState<void> m_subCooldownState{
      0.02f}; // cooldown between continuos shooting (amount)

  std::function<std::vector<Enemy *>(glm::vec3, float, uint32_t)>
      m_findClosestEnemies;

public:
  MagicWand() : Weapon(1.0f, 25.0f) {} // 1 attack per sec, 25 damage

  void setTargetingContext(
      std::function<std::vector<Enemy *>(glm::vec3, float, uint32_t)>
          findClosestEnemies) {
    m_findClosestEnemies = findClosestEnemies;
  }

  virtual void update(double delta_time, const glm::vec3 &playerPos) override {
    m_coolDownState.updateTimer(static_cast<float>(delta_time));
    m_subCooldownState.updateTimer(static_cast<float>(delta_time));

    if (m_coolDownState.isFinished()) {
      if (m_subCooldownState.isFinished() && m_amountShoot < getAmount()) {
        if (fire(playerPos)) {
          m_subCooldownState.reset();
          m_amountShoot++;
        }
      }

      if (m_amountShoot >= getAmount()) {
        m_coolDownState.reset();
        m_coolDownState.duration = getCooldown();
        m_amountShoot = 0;
      }
    }
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

  uint32_t getAmount() const {
    return m_amount + (m_stats ? m_stats->getMultiplier(StatType::AMOUNT) : 1);
  }
};
