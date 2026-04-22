#pragma once

#include "graphics/texture.hpp"

#include <glad/gl.h>

#include <memory>
#include <string>
#include <unordered_map>

struct TextureName {
  const std::string name;

  constexpr TextureName(const std::string &name) : name(name) {}
  constexpr TextureName(std::string &&name) : name(name) {}

  bool operator==(const TextureName &) const = default;
};

const TextureName STATIC_WHITE_TEXTURE = TextureName("STATIC_WHITE_TEXTURE");
const TextureName STATIC_BLACK_TEXTURE = TextureName("STATIC_BLACK_TEXTURE");
const TextureName STATIC_NORMAL_TEXTURE = TextureName("STATIC_NORMAL_TEXTURE");
const TextureName STATIC_PBR_DEFAULT_TEXTURE =
    TextureName("STATIC_PBR_DEFAULT_TEXTURE");

class TextureManager {
public:
  static std::unordered_map<TextureName, std::shared_ptr<Texture>> textures;

  static std::shared_ptr<Texture> load(TextureName name, TextureType type,
                                       const char *texture_path,
                                       bool flip_vertical = false);
  static std::shared_ptr<Texture> load(TextureName name, TextureType type,
                                       const void *data, size_t size,
                                       bool flip_vertical = false);
  static Texture load(TextureType type, const char *texture_path,
                      bool flip_vertical = false);
  static Texture load(TextureType type, const void *data, size_t size,
                      bool flip_vertical = false);

  static Texture &get(TextureName name);
  static std::shared_ptr<Texture> copy(TextureName name);
  static Texture *tryGet(TextureName name);
  static bool exists(TextureName name);
  static std::shared_ptr<Texture> manage(TextureName name, Texture &&texture);
  static void clear();

  static std::shared_ptr<Texture>
  loadCubemap(TextureName name, const std::vector<std::string> &faces);

  static Texture generateStaticWhiteTexture();
  static Texture generateStaticBlackTexture();
  static Texture generateStaticNormalTexture();
  static Texture generateStaticPBRDefaultTexture();
};
