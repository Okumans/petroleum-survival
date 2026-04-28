#pragma once

#include "game/game_events.hpp"
#include "scene/i_entity_context.hpp"
#include <glm/glm.hpp>

class StatManager;
class Enemy;
class Player;
class GameObject;

class IEnemyContext : virtual public IEntityContext {
public:
  virtual ~IEnemyContext() = default;

  using IEntityContext::emit;
  virtual void emit(const GameEvents::PlayerDamageRequestedEvent &event) = 0;
};
