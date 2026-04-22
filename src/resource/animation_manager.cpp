#include "animation_manager.hpp"

#include "utility/enum_map.hpp"
#include "utility/not_initialized.hpp"

#include <memory>

SettableNotInitialized<
    EnumMap<AnimationName, std::shared_ptr<Animation>>, "s_animations",
    EnumMapValidator<EnumMap<AnimationName, std::shared_ptr<Animation>>>>
    AnimationManager::s_animations;

std::shared_ptr<Animation> AnimationManager::load(AnimationName name,
                                                  const char *animation_path,
                                                  Model *model) {
  s_animations.set(name, std::make_shared<Animation>(animation_path, model));
  return AnimationManager::s_animations.getUnvalidated(name);
}

Animation &AnimationManager::get(AnimationName name) {
  return *AnimationManager::s_animations.ensureInitialized()[name];
}

std::shared_ptr<Animation> AnimationManager::copy(AnimationName name) {
  return AnimationManager::s_animations.ensureInitialized()[name];
}

Animation *AnimationManager::tryGet(AnimationName name) {
  return AnimationManager::s_animations.getUnvalidated(name).get();
}

bool AnimationManager::exists(AnimationName name) {
  return AnimationManager::s_animations.ensureInitialized()[name] != nullptr;
}

void AnimationManager::ensureInit() {
  (void)AnimationManager::s_animations.ensureInitialized();
}

void AnimationManager::clear() {
  for (auto animation_name : magic_enum::enum_values<AnimationName>()) {
    AnimationManager::s_animations.set(animation_name, nullptr);
  }
}
