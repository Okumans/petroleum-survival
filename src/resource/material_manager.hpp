#pragma once

#include "graphics/material.hpp"
#include <string>
#include <unordered_map>

class MaterialManager {
public:
  static std::unordered_map<std::string, Material> s_materials;

  static void load(const std::string &name, const Material &material);
  static const Material &get(const std::string &name);
  static const Material *tryGet(const std::string &name);
  static bool exists(const std::string &name);
  static void clear();
};
