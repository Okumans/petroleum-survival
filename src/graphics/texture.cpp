#include "texture.hpp"
#include <print>

#define STB_IMAGE_IMPLEMENTATION

#include "external/stb_image.h"

Texture::Texture(const std::vector<std::string> &faces)
    : m_ownTex(true), m_type(TextureType::HDR_CUBEMAP) {
  glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_texID);

  int width, height, nrChannels;
  stbi_set_flip_vertically_on_load(false); // Cubemaps should NOT be flipped

  bool isHDR = false;
  if (!faces.empty()) {
    std::string_view firstFace = faces[0];
    if (firstFace.ends_with(".hdr")) {
      isHDR = true;
    }
  }

  for (unsigned int i = 0; i < faces.size(); i++) {
    void *data = nullptr;
    if (isHDR) {
      data = stbi_loadf(faces[i].c_str(), &width, &height, &nrChannels, 0);
    } else {
      // Force 4 channels (RGBA) to ensure consistency for LDR
      data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 4);
    }

    if (data) {
      if (i == 0) {
        // Calculate mipmap levels
        int levels =
            static_cast<int>(std::floor(std::log2(std::max(width, height)))) +
            1;
        // Allocate immutable storage for 6 faces
        if (isHDR) {
          // Use GL_RGB16F for HDR cubemaps
          glTextureStorage2D(m_texID, levels, GL_RGB16F, width, height);
        } else {
          glTextureStorage2D(m_texID, levels, GL_RGBA8, width, height);
        }
      }

      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      if (isHDR) {
        glTextureSubImage3D(m_texID,
                            0,
                            0,
                            0,
                            i,
                            width,
                            height,
                            1,
                            GL_RGB,
                            GL_FLOAT,
                            data);
      } else {
        glTextureSubImage3D(m_texID,
                            0,
                            0,
                            0,
                            i,
                            width,
                            height,
                            1,
                            GL_RGBA,
                            GL_UNSIGNED_BYTE,
                            data);
      }
      glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

      stbi_image_free(data);
    } else {
      std::println(stderr,
                   "Cubemap texture failed to load at path: {}",
                   faces[i]);
    }
  }

  glTextureParameteri(m_texID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTextureParameteri(m_texID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTextureParameteri(m_texID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(m_texID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTextureParameteri(m_texID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  // Generate mipmaps
  glGenerateTextureMipmap(m_texID);
}

Texture::Texture(const char *path, TextureType type, bool flip_vertical)
    : m_ownTex(true), m_type(type) {
  m_texID = _loadTexture(path, flip_vertical);
}

Texture::Texture(const void *data,
                 size_t size,
                 TextureType type,
                 bool flip_vertical)
    : m_ownTex(true), m_type(type) {
  m_texID = _loadTextureFromMemory(data, size, flip_vertical);
}

Texture::Texture(GLuint tex_id, TextureType type, bool own)
    : m_ownTex(own), m_type(type), m_texID(tex_id) {}

Texture::Texture(Texture &&other) noexcept
    : m_ownTex(other.m_ownTex), m_type(other.m_type), m_texID(other.m_texID) {
  other.m_type = static_cast<TextureType>(0);
  other.m_ownTex = false;
  other.m_texID = 0;
}

Texture::~Texture() {
  if (m_ownTex && m_texID)
    glDeleteTextures(1, &m_texID);
}

GLuint Texture::_loadTexture(const char *path, bool flip_vertical) {
  int width, height, nrComponents;

  stbi_set_flip_vertically_on_load(flip_vertical);
  unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);

  if (!data) {
    std::println(stderr, "ERROR::TEXTURE::LOAD_FAILED: {}", path);
    return 0;
  }

  GLuint textureID = _createTexture(data, width, height, nrComponents);
  stbi_image_free(data);
  return textureID;
}

GLuint Texture::_loadTextureFromMemory(const void *data,
                                       size_t size,
                                       bool flip_vertical) {
  int width, height, nrComponents;

  stbi_set_flip_vertically_on_load(flip_vertical);
  unsigned char *pixels =
      stbi_load_from_memory(static_cast<const stbi_uc *>(data),
                            static_cast<int>(size),
                            &width,
                            &height,
                            &nrComponents,
                            0);

  if (!pixels) {
    std::println(stderr, "ERROR::TEXTURE::LOAD_FROM_MEMORY_FAILED");
    return 0;
  }

  GLuint textureID = _createTexture(pixels, width, height, nrComponents);
  stbi_image_free(pixels);
  return textureID;
}

GLuint Texture::_createTexture(unsigned char *data,
                               int width,
                               int height,
                               int nrComponents) {
  // 1. Determine Formats
  GLenum internalFormat, dataFormat;
  if (nrComponents == 1) {
    internalFormat = GL_R8;
    dataFormat = GL_RED;
  } else if (nrComponents == 3) {
    internalFormat = GL_RGB8;
    dataFormat = GL_RGB;
  } else { // nrComponents == 4
    internalFormat = GL_RGBA8;
    dataFormat = GL_RGBA;
  }

  // 2. Create Texture Object (DSA)
  GLuint textureID;
  glCreateTextures(GL_TEXTURE_2D, 1, &textureID);

  // 3. Calculate Mipmap Levels
  int levels =
      static_cast<int>(std::floor(std::log2(std::max(width, height)))) + 1;

  // 4. Allocate Immutable Storage (DSA)
  glTextureStorage2D(textureID, levels, internalFormat, width, height);

  // 5. Handle Alignment for non-power-of-two/odd dimensions (418px, 350px)
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  // 6. Upload Data (DSA)
  glTextureSubImage2D(textureID,
                      0,
                      0,
                      0,
                      width,
                      height,
                      dataFormat,
                      GL_UNSIGNED_BYTE,
                      data);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // Reset to default

  // 7. Generate Mipmaps (DSA)
  glGenerateTextureMipmap(textureID);

  // 8. Set Parameters (DSA) - Using REPEAT for your scaling and NEAREST for the
  // Crossy vibe
  glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // Use NEAREST for that blocky Crossy Road look, or LINEAR for smooth
  glTextureParameteri(textureID,
                      GL_TEXTURE_MIN_FILTER,
                      GL_NEAREST_MIPMAP_LINEAR);
  glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  return textureID;
}
