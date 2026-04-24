#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

class ShadowMap {
public:
  ShadowMap(unsigned int width = 4096, unsigned int height = 4096);
  ~ShadowMap();

  void bindForWriting();
  void bindForReading(unsigned int unit);

  void updateLightSpaceMatrix(const glm::vec3 &targetPos);

  [[nodiscard]] glm::mat4 getLightSpaceMatrix() const {
    return m_lightSpaceMatrix;
  }
  [[nodiscard]] GLuint getTexID() const { return m_shadowMapTex; }
  [[nodiscard]] unsigned int getWidth() const { return m_width; }
  [[nodiscard]] unsigned int getHeight() const { return m_height; }

private:
  GLuint m_shadowMapFBO;
  GLuint m_shadowMapTex;
  unsigned int m_width, m_height;
  glm::mat4 m_lightSpaceMatrix;
};
