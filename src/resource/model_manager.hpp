#pragma once

#include "graphics/model.hpp"
#include "utility/enum_map.hpp"
#include "utility/not_initialized.hpp"

#include <memory>

// Define model names here
enum class ModelName {
  THE_WITCH,
  BUDHIST_CHARACTER,
  MONEY_20,
  MONEY_100,
  MONEY_500,
  MONEY_1000,
  CAR_SEDAN,
  CAR_MUSCLE,
  CAR_PICKUP,
  CAR_TAXI,
  CAR_POLICE,
  CAR_BUS,
  CAR_MONSTER_TRUCK,
  SPHERE,
  CUBE,
  WATER_BOTTLE,
  TRAFFIC_CONE,
  MILITIA_HUMAN,
  TREE_1,
  TREE_2,
  BUSH_1,
  BUSH_2,
  ROCK_1,
};

class ModelManager {
public:
  static Utility::SettableNotInitialized<
      EnumMap<ModelName, std::shared_ptr<Model>>, "s_models",
      EnumMapValidator<EnumMap<ModelName, std::shared_ptr<Model>>>>
      s_models;

  static std::shared_ptr<Model> load(ModelName name, const char *model_path,
                                     bool flip_vertical = false);
  [[nodiscard]] static Model &get(ModelName name);
  [[nodiscard]] static std::shared_ptr<Model> copy(ModelName name);
  [[nodiscard]] static Model *tryGet(ModelName name);
  [[nodiscard]] static bool exists(ModelName name);
  static void ensureInit();
  static void clear();
};
