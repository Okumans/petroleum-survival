#include "shadow_map.hpp"
#include "resource/lighting_manager.hpp"
#include <glad/gl.h>

ShadowMap::ShadowMap(unsigned int width, unsigned int height)
    : m_width(width), m_height(height), m_lightSpaceMatrix(1.0f) {
  glGenFramebuffers(1, &m_shadowMapFBO);
  glGenTextures(1, &m_shadowMapTex);
  glBindTexture(GL_TEXTURE_2D, m_shadowMapTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_width, m_height, 0,
               GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  float border_color[] = {1.0, 1.0, 1.0, 1.0};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);

  glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         m_shadowMapTex, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

ShadowMap::~ShadowMap() {
  glDeleteFramebuffers(1, &m_shadowMapFBO);
  glDeleteTextures(1, &m_shadowMapTex);
}

void ShadowMap::bindForWriting() {
  glViewport(0, 0, m_width, m_height);
  glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFBO);
  glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::bindForReading(unsigned int unit) {
  glBindTextureUnit(unit, m_shadowMapTex);
}

void ShadowMap::updateLightSpaceMatrix(const glm::vec3 &targetPos) {
  m_lightSpaceMatrix = LightingManager::calculateLightSpaceMatrix(targetPos);
}
