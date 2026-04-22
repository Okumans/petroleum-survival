#pragma once

#include "camera.hpp"
#include <glm/glm.hpp>

class CameraController {
private:
  Camera &m_camera;

  glm::vec3 m_basePosition;
  glm::vec3 m_targetPosition;
  glm::vec3 m_offset;

  // Shake parameters
  float m_shakeTime = 0.0f;
  float m_shakeDuration = 0.0f;
  float m_shakeIntensity = 0.0f;

  float m_lerpFactor = 5.0f;

public:
  CameraController(Camera &camera,
                   glm::vec3 offset = glm::vec3(8.0f, 8.0f, 8.0f))
      : m_camera(camera), m_offset(offset) {
    m_basePosition = m_camera.position;
    m_targetPosition = glm::vec3(0.0f);
  }

  void update(float deltaTime);

  void setTarget(glm::vec3 targetPos, bool immediate = false);
  void follow(glm::vec3 targetPos) { m_targetPosition = targetPos; }
  void shake(float intensity, float duration);

  void setLerpFactor(float factor) { m_lerpFactor = factor; }
  void setOffset(glm::vec3 offset) { m_offset = offset; }
};
