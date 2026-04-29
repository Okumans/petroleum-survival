#include "scene/game_factories.hpp"
#include "utility/random.hpp"

std::optional<GameObjectFactory<BudhistEnemy>> GameFactories::s_budhistEnemy;
std::optional<GameObjectFactory<MilitiaEnemy>> GameFactories::s_militiaEnemy;
std::optional<GameObjectFactory<Player>> GameFactories::s_player;
std::optional<GameObjectFactory<Exp>> GameFactories::s_exp;
EnumMap<ModelName, std::optional<GameObjectFactory<CarEnemy>>>
    GameFactories::s_cars;
EnumMap<ModelName, std::optional<GameObjectFactory<Projectile>>>
    GameFactories::s_projectiles;
std::optional<GameObjectFactory<MeleeProjectile>>
    GameFactories::s_meleeProjectile;

void GameFactories::init() {
  // 1. Humanoid Enemy Factory
  s_budhistEnemy = GameObjectFactory<BudhistEnemy>::create_factory([]() {
    BudhistEnemy enemy(ModelManager::copy(ModelName::BUDHIST_CHARACTER));
    enemy.setScale(13000.0f);
    enemy.setup();
    return enemy;
  });

  s_militiaEnemy = GameObjectFactory<MilitiaEnemy>::create_factory([]() {
    MilitiaEnemy enemy(ModelManager::copy(ModelName::MILITIA_HUMAN));
    enemy.setScale(300.0f);
    enemy.setup();
    return enemy;
  });

  // 2. Player Factory
  s_player = GameObjectFactory<Player>::create_factory([]() {
    Player player(ModelManager::copy(ModelName::THE_WITCH));
    player.setScale(300.0f);
    player.setup();
    return player;
  });

  // 3. Exp Factory (Gem/Money)
  s_exp = GameObjectFactory<Exp>::create_factory([]() {
    Exp exp(ModelManager::copy(ModelName::MONEY_20));
    exp.setScale(1.0f);
    exp.setEmissionColor(glm::vec3(0.0f));
    return exp;
  });

  // 4. Car Factories
  ModelName carModels[] = {ModelName::CAR_SEDAN,        ModelName::CAR_MUSCLE,
                           ModelName::CAR_PICKUP,       ModelName::CAR_TAXI,
                           ModelName::CAR_POLICE,       ModelName::CAR_BUS,
                           ModelName::CAR_MONSTER_TRUCK};

  for (auto model : carModels) {
    s_cars[model] = GameObjectFactory<CarEnemy>::create_factory([model]() {
      CarEnemy car(ModelManager::copy(model));
      car.setScale(0.8f);
      return car;
    });
  }

  // 5. Projectile Factories (per model)
  ModelName projectileModels[] = {
      ModelName::SPHERE,
      ModelName::WATER_BOTTLE,
      ModelName::TRAFFIC_CONE,
  };

  for (auto model : projectileModels) {
    s_projectiles[model] =
        GameObjectFactory<Projectile>::create_factory([model]() {
          return Projectile(ModelManager::copy(model), glm::vec3(0.0f),
                            glm::vec3(0.0f), 10.0f, 1.0f);
        });
  }

  // 6. Melee Projectile Factory
  s_meleeProjectile = GameObjectFactory<MeleeProjectile>::create_factory([]() {
    return MeleeProjectile(ModelManager::copy(ModelName::CUBE), glm::vec3(0.0f),
                           glm::vec3(0.0f, 0.0f, 1.0f), 0.0f, 10.0f, 1.0f);
  });
}

const GameObjectFactory<HumanoidEnemy> &GameFactories::getHumanoidEnemy() {
  if (Utility::Random::randFloat() < 0.5f) {
    return *reinterpret_cast<const GameObjectFactory<HumanoidEnemy> *>(
        &s_budhistEnemy.value());
  }
  return *reinterpret_cast<const GameObjectFactory<HumanoidEnemy> *>(
      &s_militiaEnemy.value());
}

const GameObjectFactory<Player> &GameFactories::getPlayer() {
  return *s_player;
}

const GameObjectFactory<Exp> &GameFactories::getExp() { return *s_exp; }

const GameObjectFactory<CarEnemy> &GameFactories::getCar(ModelName model) {
  return *s_cars[model];
}

const GameObjectFactory<Projectile> &
GameFactories::getProjectile(ModelName model) {
  if (s_projectiles[model].has_value()) {
    return *s_projectiles[model];
  }
  return *s_projectiles[ModelName::SPHERE];
}

const GameObjectFactory<Projectile> &GameFactories::getProjectile() {
  return getProjectile(ModelName::SPHERE);
}

const GameObjectFactory<MeleeProjectile> &GameFactories::getMeleeProjectile() {
  return *s_meleeProjectile;
}
