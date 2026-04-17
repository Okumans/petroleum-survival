#include "material_manager.hpp"
#include <print>

std::unordered_map<std::string, Material> MaterialManager::materials;

void MaterialManager::addMaterial(const std::string &name,
                                  const Material &material) {
  MaterialManager::materials.insert({name, material});
}

const Material &MaterialManager::getMaterial(const std::string &name) {
  if (!MaterialManager::materials.contains(name)) {
    std::println("{} doesn't exist!?", name);
  }

  return MaterialManager::materials.at(name);
}

bool MaterialManager::exists(const std::string &name) {
  return MaterialManager::materials.contains(name);
}
