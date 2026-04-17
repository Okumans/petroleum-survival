#pragma once

#include "graphics/material.hpp"
#include "graphics/texture.hpp"
#include "resource/material_manager.hpp"
#include "resource/texture_manager.hpp"

#include <concepts>
#include <filesystem>
#include <format>
#include <memory>
#include <string>

inline void loadMaterialFolder(const std::string &materialName,
                               const std::string &folderPath) {
  namespace fs = std::filesystem;

  const std::vector<std::pair<std::string, TextureType>> textureTypes = {
      {"diffuse", TextureType::DIFFUSE},
      {"height", TextureType::HEIGHT},
      {"normal", TextureType::NORMAL},
      {"ao", TextureType::AO},
      {"roughness", TextureType::ROUGHNESS},
      {"metallic", TextureType::METALLIC}};

  auto builder = Material::builder();
  bool foundAny = false;

  for (const auto &[suffix, type] : textureTypes) {
    std::string baseFile = folderPath + "/" + suffix;
    std::string fullPath;

    if (fs::exists(baseFile + ".jpg")) {
      fullPath = baseFile + ".jpg";
    } else if (fs::exists(baseFile + ".png")) {
      fullPath = baseFile + ".png";
    } else {
      continue;
    }

    auto tex = TextureManager::loadTexture(
        TextureName(std::format("{}_{}", materialName, suffix)), type,
        fullPath.c_str());

    foundAny = true;
    switch (type) {
    case TextureType::DIFFUSE:
      builder.setDiffuse(tex);
      break;
    case TextureType::HEIGHT:
      builder.setHeight(tex);
      break;
    case TextureType::NORMAL:
      builder.setNormal(tex);
      break;
    case TextureType::AO:
      builder.setAO(tex);
      break;
    case TextureType::ROUGHNESS:
      builder.setRoughness(tex);
      break;
    case TextureType::METALLIC:
      builder.setMetallic(tex);
      break;
    default:
      break;
    }
  }

  if (foundAny) {
    MaterialManager::addMaterial(materialName, builder.create());
  }
}

template <typename T, std::invocable<T &> F>
constexpr T withBase(T base, F modifier) {
  modifier(base);
  return base;
}

#include "enum_map.hpp" // IWYU pragma: keep
#include "random.hpp"   // IWYU pragma: keep
