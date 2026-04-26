#pragma once

#include "resource/model_manager.hpp"
#include <functional>
#include <glm/glm.hpp>

class Player;
class Item;
class Exp;
class GameObject;
class Enemy;
#include "scene/projectile.hpp"

namespace GameEvents {

struct ItemCollectedEvent {
  Player *player;
  Item *item;
  int value;
};

struct ExpCollectedEvent {
  Player *player;
  Exp *exp;
  float amount;
};

struct DespawnRequestedEvent {
  GameObject *object;
};

struct ParticleSpawnRequestedEvent {
  glm::vec3 position;
  int effectId;
};
struct ProjectileSpawnRequestedEvent {
  Projectile projectile;
};

struct EnemyKilledEvent {
  ::Enemy *enemy;
  glm::vec3 killerPosition;
};

} // namespace GameEvents
