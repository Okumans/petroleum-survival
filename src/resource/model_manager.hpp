#pragma once

#include "graphics/model.hpp"
#include "utility/enum_map.hpp"
#include "utility/not_initialized.hpp"

#include <memory>

// Define model names here
enum class ModelName {
  KASANE_TETO,
  HATSUNE_MIKU,
  COIN,
  EXP_GEM_1,
  EXP_GEM_2,
  SPHERE
};

class ModelManager {
public:
  static SettableNotInitialized<
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
