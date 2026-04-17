#pragma once

#include "graphics/material.hpp"
#include <string>
#include <unordered_map>

class MaterialManager {
public:
  static std::unordered_map<std::string, Material> materials;

  static void addMaterial(const std::string &name, const Material &material);
  static const Material &getMaterial(const std::string &name);
  static bool exists(const std::string &name);
};
