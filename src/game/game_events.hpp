#pragma once

#include "scene/item.hpp"
#include <glm/glm.hpp>
#include <memory>

class Enemy;

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

enum class ParticleEffectType {
  EXP_COLLECT,
  ITEM_COLLECT,
  ENEMY_DEATH,
  PLAYER_BLOOD,
  MAGIC_HIT
};

struct ParticleSpawnRequestedEvent {
  glm::vec3 position;
  ParticleEffectType effectId;
};
struct ProjectileSpawnRequestedEvent {
  std::shared_ptr<Projectile> projectile;
};

struct EnemyKilledEvent {
  Enemy *enemy;
  glm::vec3 killerPosition;
};

} // namespace GameEvents
