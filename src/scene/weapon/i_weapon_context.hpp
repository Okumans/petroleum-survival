#pragma once

#include "game/game_events.hpp"
#include "scene/i_entity_context.hpp"
#include <functional>
#include <glm/glm.hpp>

class StatManager;
class Enemy;
class Player;
class GameObject;

class IWeaponContext : virtual public IEntityContext {
public:
  virtual ~IWeaponContext() = default;

  using IEntityContext::emit;
  virtual void emit(const GameEvents::EnemyDamageRequestedEvent &event) = 0;

  using EnemyCallback = std::function<void(Enemy *)>;
  virtual void findTargets(float range, uint32_t k,
                           EnemyCallback callback) const = 0;
};
