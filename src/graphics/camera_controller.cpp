#include "camera_controller.hpp"
#include "utility/random.hpp"
#include <glm/gtc/matrix_transform.hpp>

void CameraController::update(float deltaTime) {
  // Smoothly move base position towards target + offset
  glm::vec3 desiredPosition = m_targetPosition + m_offset;
  m_basePosition =
      glm::mix(m_basePosition, desiredPosition, deltaTime * m_lerpFactor);

  // Handle shake
  glm::vec3 shakeOffset(0.0f);
  if (m_shakeTime > 0.0f) {
    m_shakeTime -= deltaTime;
    float currentIntensity = (m_shakeTime / m_shakeDuration) * m_shakeIntensity;
    shakeOffset =
        glm::vec3(Utility::Random::randFloat(-1.0f, 1.0f) * currentIntensity,
                  Utility::Random::randFloat(-1.0f, 1.0f) * currentIntensity,
                  Utility::Random::randFloat(-1.0f, 1.0f) * currentIntensity);
  } else {
    m_shakeTime = 0.0f;
  }

  // Update camera position
  m_camera.position = m_basePosition + shakeOffset;
}

void CameraController::setTarget(glm::vec3 targetPos, bool immediate) {
  m_targetPosition = targetPos;
  if (immediate) {
    m_basePosition = targetPos + m_offset;
    m_camera.position = m_basePosition;
  }
}

void CameraController::shake(float intensity, float duration) {
  m_shakeIntensity = intensity;
  m_shakeDuration = duration;
  m_shakeTime = duration;
}
