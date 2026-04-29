#pragma once

#include "graphics/material.hpp"
#include "graphics/texture.hpp"
#include "resource/material_manager.hpp"
#include "resource/texture_manager.hpp"

#include <cmath>
#include <concepts>
#include <filesystem>
#include <format>
#include <memory>
#include <string>

namespace Utility {

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

    auto tex = TextureManager::load(
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
    MaterialManager::load(materialName, builder.create());
  }
}

template <typename T, std::invocable<T &> F>
inline constexpr T withBase(T base, F modifier) {
  modifier(base);
  return base;
}

// NOTE: not constexpr — std::fmod is not constexpr in MSVC's STL for this C++
// mode.
inline float lerpAngle(float start, float end, float t) {
  float diff = std::fmod(end - start + 180.0f, 360.0f) - 180.0f;
  if (diff < -180.0f)
    diff += 360.0f;
  return start + diff * t;
}

inline constexpr uint32_t fnv1a(std::string_view str) {
  uint32_t hash = 0x811c9dc5;
  for (char c : str) {
    hash ^= static_cast<uint8_t>(c);
    hash *= 0x01000193;
  }
  return hash;
}

template <typename Callback, std::ranges::range... Containers>
inline constexpr void concat(Callback &&callback, Containers &&...containers) {
  auto process_container = [&](auto &&container) {
    for (auto &&arg : container) {
      std::forward<Callback>(callback)(std::forward<decltype(arg)>(arg));
    }
  };

  (process_container(std::forward<Containers>(containers)), ...);
}

}; // namespace Utility

#include "enum_map.hpp" // IWYU pragma: keep
#include "random.hpp"   // IWYU pragma: keep
