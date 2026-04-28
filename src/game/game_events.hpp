#pragma once

#include "scene/item.hpp"
#include <functional>
#include <glm/glm.hpp>
#include <memory>

class Enemy;
struct ObjectHandle;

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
  MAGIC_HIT,
  FLAME,
  FUME
};

struct ParticleSpawnRequestedEvent {
  glm::vec3 position;
  glm::vec3 direction{0.0f, 1.0f, 0.0f};
  float length{1.0f};
  float thickness{1.0f};
  ParticleEffectType effectId;
};
struct ProjectileSpawnRequestedEvent {
  std::shared_ptr<Projectile> projectile;
  std::function<void(const ObjectHandle &)> onSpawned = nullptr;
};

struct EnemyDamageRequestedEvent {
  Enemy *enemy;
  float amount;
  bool isCritical;
  glm::vec3 knockbackDirection;
  float knockbackStrength;
  glm::vec3 hitPosition;
  ParticleEffectType hitEffect;
};

struct EnemyKilledEvent {
  Enemy *enemy;
  glm::vec3 killerPosition;
};

} // namespace GameEvents
