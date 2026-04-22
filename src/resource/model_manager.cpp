#include "model_manager.hpp"
#include "utility/enum_map.hpp"
#include "utility/not_initialized.hpp"
#include <memory>

SettableNotInitialized<
    EnumMap<ModelName, std::shared_ptr<Model>>, "s_models",
    EnumMapValidator<EnumMap<ModelName, std::shared_ptr<Model>>>>
    ModelManager::s_models;

std::shared_ptr<Model>
ModelManager::load(ModelName name, const char *model_path, bool flip_vertical) {
  s_models.set(name, std::make_shared<Model>(model_path, flip_vertical));
  return ModelManager::s_models.getUnvalidated(name);
}

Model &ModelManager::get(ModelName name) {
  return *ModelManager::s_models.ensureInitialized()[name];
}

std::shared_ptr<Model> ModelManager::copy(ModelName name) {
  return ModelManager::s_models.ensureInitialized()[name];
}

Model *ModelManager::tryGet(ModelName name) {
  return ModelManager::s_models.getUnvalidated(name).get();
}

bool ModelManager::exists(ModelName name) {
  return ModelManager::s_models.ensureInitialized()[name] != nullptr;
}

void ModelManager::ensureInit() {
  (void)ModelManager::s_models.ensureInitialized();
}

void ModelManager::clear() {
  for (auto model_name : magic_enum::enum_values<ModelName>()) {
    ModelManager::s_models.set(model_name, nullptr);
  }
}
