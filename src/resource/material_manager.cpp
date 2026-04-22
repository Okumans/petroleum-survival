#include "material_manager.hpp"

std::unordered_map<std::string, Material> MaterialManager::s_materials;

void MaterialManager::load(const std::string &name, const Material &material) {
  MaterialManager::s_materials[name] = material;
}

const Material *MaterialManager::tryGet(const std::string &name) {
  auto it = MaterialManager::s_materials.find(name);
  if (it == MaterialManager::s_materials.end()) {
    return nullptr;
  }

  return &it->second;
}

void MaterialManager::clear() { MaterialManager::s_materials.clear(); }

const Material &MaterialManager::get(const std::string &name) {
  return MaterialManager::s_materials.at(name);
}

bool MaterialManager::exists(const std::string &name) {
  return MaterialManager::s_materials.contains(name);
}
