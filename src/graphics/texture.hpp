#pragma once

#include <glad/gl.h>
#include <string>
#include <vector>

enum class TextureType : uint8_t {
  DIFFUSE,
  SPECULAR,
  NORMAL,
  HEIGHT,
  METALLIC,
  ROUGHNESS,
  AO,
  HDR_CUBEMAP,
  IRRADIANCE_MAP,
  UNDEFINED
};

class Texture {
private:
  bool m_ownTex = false;
  TextureType m_type = TextureType::UNDEFINED;
  GLuint m_texID = 0;

public:
  Texture(const std::vector<std::string> &faces);
  Texture(const char *path, TextureType type, bool flip_vertical = false);
  Texture(const void *data, size_t size, TextureType type,
          bool flip_vertical = false);
  Texture(GLuint tex_id, TextureType type, bool own = false);
  ~Texture();

  Texture(const Texture &) = delete;
  Texture &operator=(const Texture &) = delete;

  Texture(Texture &&other) noexcept;

  [[nodiscard]] GLuint getTexID() const { return m_texID; };
  [[nodiscard]] TextureType getType() const { return m_type; };

private:
  GLuint _loadTexture(const char *path, bool flip_vertical);
  GLuint _loadTextureFromMemory(const void *data, size_t size,
                                bool flip_vertical);
  GLuint _createTexture(unsigned char *data, int width, int height,
                        int nrComponents);
};
