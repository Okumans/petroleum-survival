#include "texture_manager.hpp"

#include <glad/gl.h>

#include <cstdint>
#include <memory>
#include <print>
#include <unordered_map>

namespace std {
template <> struct hash<TextureName> {
  std::size_t operator()(const TextureName &k) const noexcept {
    return std::hash<std::string>{}(k.name);
  }
};
} // namespace std

std::unordered_map<TextureName, std::shared_ptr<Texture>>
    TextureManager::textures;

std::shared_ptr<Texture> TextureManager::loadTexture(TextureName name,
                                                     TextureType type,
                                                     const char *texture_path,
                                                     bool flip_vertical) {
  textures[name] = std::make_shared<Texture>(texture_path, type, flip_vertical);

  return TextureManager::textures.at(name);
}

std::shared_ptr<Texture>
TextureManager::loadTexture(TextureName name, TextureType type,
                            const void *data, size_t size, bool flip_vertical) {
  textures[name] = std::make_shared<Texture>(data, size, type, flip_vertical);

  return TextureManager::textures.at(name);
}

Texture TextureManager::loadTexture(TextureType type, const char *texture_path,
                                    bool flip_vertical) {
  return Texture(texture_path, type, flip_vertical);
}

Texture TextureManager::loadTexture(TextureType type, const void *data,
                                    size_t size, bool flip_vertical) {
  return Texture(data, size, type, flip_vertical);
}

std::shared_ptr<Texture>
TextureManager::loadCubemap(TextureName name,
                            const std::vector<std::string> &faces) {
  TextureManager::textures[name] = std::make_shared<Texture>(faces);
  return TextureManager::textures.at(name);
}

std::shared_ptr<Texture> TextureManager::manage(TextureName name,
                                                Texture &&texture) {
  textures[name] = std::make_shared<Texture>(std::move(texture));
  return TextureManager::textures.at(name);
}

Texture TextureManager::generateStaticWhiteTexture() {
  GLuint white_texture;
  int size = 16;

  glCreateTextures(GL_TEXTURE_2D, 1, &white_texture);

  // 1 mip level, RGBA8 format, size x size
  glTextureStorage2D(white_texture, 1, GL_RGBA8, size, size);

  std::vector<uint8_t> white_data(size * size * 4, 0xFF);
  glTextureSubImage2D(white_texture, 0, 0, 0, size, size, GL_RGBA,
                      GL_UNSIGNED_BYTE, white_data.data());

  glTextureParameteri(white_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTextureParameteri(white_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTextureParameteri(white_texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTextureParameteri(white_texture, GL_TEXTURE_WRAP_T, GL_REPEAT);

  return Texture(white_texture, TextureType::DIFFUSE, true);
}

Texture TextureManager::generateStaticBlackTexture() {
  GLuint black_texture;
  int size = 16;
  glCreateTextures(GL_TEXTURE_2D, 1, &black_texture);
  glTextureStorage2D(black_texture, 1, GL_RGBA8, size, size);
  std::vector<uint8_t> black_data(size * size * 4, 0x00);
  glTextureSubImage2D(black_texture, 0, 0, 0, size, size, GL_RGBA,
                      GL_UNSIGNED_BYTE, black_data.data());
  glTextureParameteri(black_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTextureParameteri(black_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  return Texture(black_texture, TextureType::DIFFUSE, true);
}

Texture TextureManager::generateStaticNormalTexture() {
  GLuint normal_texture;
  int size = 16;
  glCreateTextures(GL_TEXTURE_2D, 1, &normal_texture);
  glTextureStorage2D(normal_texture, 1, GL_RGBA8, size, size);
  // Flat normal map color is (0.5, 0.5, 1.0), which is 0x80, 0x80, 0xFF
  std::vector<uint8_t> normal_data(size * size * 4);
  for (size_t i = 0; i < size * size; ++i) {
    normal_data[i * 4 + 0] = 0x80;
    normal_data[i * 4 + 1] = 0x80;
    normal_data[i * 4 + 2] = 0xFF;
    normal_data[i * 4 + 3] = 0xFF;
  }
  glTextureSubImage2D(normal_texture, 0, 0, 0, size, size, GL_RGBA,
                      GL_UNSIGNED_BYTE, normal_data.data());
  glTextureParameteri(normal_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTextureParameteri(normal_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  return Texture(normal_texture, TextureType::NORMAL, true);
}

Texture TextureManager::generateStaticPBRDefaultTexture() {
  GLuint pbr_texture;
  int size = 16;
  glCreateTextures(GL_TEXTURE_2D, 1, &pbr_texture);
  glTextureStorage2D(pbr_texture, 1, GL_RGBA8, size, size);
  // Default PBR: Roughness=1.0 (G=255), Metallic=0.0 (B=0)
  std::vector<uint8_t> pbr_data(size * size * 4);
  for (size_t i = 0; i < size * size; ++i) {
    pbr_data[i * 4 + 0] = 0x00; // R
    pbr_data[i * 4 + 1] = 0xFF; // G (Roughness)
    pbr_data[i * 4 + 2] = 0x00; // B (Metallic)
    pbr_data[i * 4 + 3] = 0xFF; // A
  }
  glTextureSubImage2D(pbr_texture, 0, 0, 0, size, size, GL_RGBA,
                      GL_UNSIGNED_BYTE, pbr_data.data());
  glTextureParameteri(pbr_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTextureParameteri(pbr_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  return Texture(pbr_texture, TextureType::METALLIC, true);
}

Texture &TextureManager::getTextureRef(TextureName name) {
  return *TextureManager::textures.at(name);
}

std::shared_ptr<Texture> TextureManager::getTexture(TextureName name) {
  if (!TextureManager::textures.contains(name)) {
    std::println("{} don't exists!?", name.name);
  }

  return TextureManager::textures.at(name);
}

bool TextureManager::exists(TextureName name) {
  return TextureManager::textures.find(name) != TextureManager::textures.end();
}
