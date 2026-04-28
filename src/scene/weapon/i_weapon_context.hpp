#pragma once

#include "game/game_events.hpp"
#include <functional>
#include <glm/glm.hpp>

class StatManager;
class Enemy;
class Player;
class GameObject;

class IWeaponContext {
public:
  virtual ~IWeaponContext() = default;

  [[nodiscard]] virtual const StatManager *getStats() const = 0;
  [[nodiscard]] virtual const Player *getPlayer() const = 0;
  [[nodiscard]] virtual const Player *getPlayer() = 0;

  [[nodiscard]] virtual glm::vec3 getPlayerPosition() const = 0;
  [[nodiscard]] virtual glm::vec3 getPlayerForward() const = 0;

  virtual void emit(const GameEvents::ProjectileSpawnRequestedEvent &event) = 0;
  virtual void emit(const GameEvents::ParticleSpawnRequestedEvent &event) = 0;
  virtual void emit(const GameEvents::EnemyDamageRequestedEvent &event) = 0;

  using EnemyCallback = std::function<void(Enemy *)>;
  virtual void findTargets(float range, uint32_t k,
                           EnemyCallback callback) const = 0;

  [[nodiscard]] virtual const GameObject *
  resolveHandle(const ObjectHandle &handle) const = 0;

  [[nodiscard]] virtual GameObject *
  resolveHandle(const ObjectHandle &handle) = 0;
};
