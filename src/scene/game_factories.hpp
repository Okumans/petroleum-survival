#pragma once

#include "resource/model_manager.hpp"
#include "scene/enemy/car_enemy.hpp"
#include "scene/enemy/humanoid_enemy.hpp"
#include "scene/exp.hpp"
#include "scene/game_object_factory.hpp"
#include "scene/player.hpp"
#include "scene/projectile.hpp"
#include "scene/weapons/melee_projectile.hpp"
#include "utility/enum_map.hpp"
#include <optional>

class GameFactories {
public:
  static void init();

  static const GameObjectFactory<HumanoidEnemy> &getHumanoidEnemy();
  static const GameObjectFactory<Player> &getPlayer();
  static const GameObjectFactory<Exp> &getExp();
  static const GameObjectFactory<CarEnemy> &getCar(ModelName model);
  static const GameObjectFactory<Projectile> &getProjectile(ModelName model);
  static const GameObjectFactory<Projectile> &getProjectile();
  static const GameObjectFactory<MeleeProjectile> &getMeleeProjectile();

private:
  static std::optional<GameObjectFactory<HumanoidEnemy>> s_humanoidEnemy;
  static std::optional<GameObjectFactory<Player>> s_player;
  static std::optional<GameObjectFactory<Exp>> s_exp;
  static EnumMap<ModelName, std::optional<GameObjectFactory<CarEnemy>>> s_cars;
  static EnumMap<ModelName, std::optional<GameObjectFactory<Projectile>>>
      s_projectiles;
  static std::optional<GameObjectFactory<MeleeProjectile>> s_meleeProjectile;
};
