#pragma once

#include "resource/model_manager.hpp"
#include <functional>
#include <glm/glm.hpp>

class Player;
class Item;
class Exp;
class GameObject;
class Projectile;

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
  glm::vec3 position;
  glm::vec3 velocity;
  float damage;
  float lifetime;
  ModelName modelName;
  glm::vec3 scale = glm::vec3(1.0f);
  std::function<void(Projectile &, double)> behaviorCallback = nullptr;
};

} // namespace GameEvents
