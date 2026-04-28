#pragma once

#include "graphics/animation.hpp"
#include "utility/enum_map.hpp"
#include "utility/not_initialized.hpp"

#include <memory>

enum class AnimationName {
  KASANE_TETO_IDLE,
  KASANE_TETO_WALKING,
  KASANE_TETO_RUNNING,
  KASANE_TETO_DANCING,
  HATSUNE_MIKU_IDLE,
  HATSUNE_MIKU_WALKING,
  HUMAN_IDLE,
  HUMAN_WALKING,
};

class AnimationManager {
public:
  static SettableNotInitialized<
      EnumMap<AnimationName, std::shared_ptr<Animation>>, "s_animations",
      EnumMapValidator<EnumMap<AnimationName, std::shared_ptr<Animation>>>>
      s_animations;

  static std::shared_ptr<Animation>
  load(AnimationName name, const char *animation_path, Model *model);
  [[nodiscard]] static Animation &get(AnimationName name);
  [[nodiscard]] static std::shared_ptr<Animation> copy(AnimationName name);
  [[nodiscard]] static Animation *tryGet(AnimationName name);
  [[nodiscard]] static bool exists(AnimationName name);
  static void ensureInit();
  static void clear();
};
