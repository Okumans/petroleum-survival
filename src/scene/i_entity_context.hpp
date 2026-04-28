#pragma once

#include "game/game_events.hpp"
#include "scene/game_object.hpp"
#include <glm/glm.hpp>

class StatManager;
class Enemy;
class Player;
class GameObject;

class IEntityContext {
public:
  virtual ~IEntityContext() = default;

  [[nodiscard]] virtual const StatManager *getStats() const = 0;
  [[nodiscard]] virtual const Player *getPlayer() const = 0;
  [[nodiscard]] virtual Player *getPlayer() = 0;

  [[nodiscard]] virtual glm::vec3 getPlayerPosition() const = 0;
  [[nodiscard]] virtual glm::vec3 getPlayerForward() const = 0;

  virtual void emit(const GameEvents::ProjectileSpawnRequestedEvent &event) = 0;
  virtual void emit(const GameEvents::ParticleSpawnRequestedEvent &event) = 0;

  [[nodiscard]] virtual float getGroundLevel(glm::vec3 pos) const = 0;

  [[nodiscard]] virtual const GameObject *
  resolveHandle(const ObjectHandle &handle) const = 0;

  [[nodiscard]] virtual GameObject *
  resolveHandle(const ObjectHandle &handle) = 0;
};
