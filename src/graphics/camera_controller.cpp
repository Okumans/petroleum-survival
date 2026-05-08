#include "camera_controller.hpp"
#include "utility/random.hpp"
#include <glm/gtc/matrix_transform.hpp>

void CameraController::update(float delta_time) {
  // Smoothly move base position towards target + offset
  glm::vec3 desired_position = m_targetPosition + m_offset;
  m_basePosition =
      glm::mix(m_basePosition, desired_position, delta_time * m_lerpFactor);

  // Handle shake
  glm::vec3 shake_offset(0.0f);
  if (m_shakeTime > 0.0f) {
    m_shakeTime -= delta_time;
    float current_intensity =
        (m_shakeTime / m_shakeDuration) * m_shakeIntensity;
    shake_offset =
        glm::vec3(Utility::Random::randFloat(-1.0f, 1.0f) * current_intensity,
                  Utility::Random::randFloat(-1.0f, 1.0f) * current_intensity,
                  Utility::Random::randFloat(-1.0f, 1.0f) * current_intensity);
  } else {
    m_shakeTime = 0.0f;
  }

  // Update camera position
  m_camera.position = m_basePosition + shake_offset;
}

void CameraController::setTarget(glm::vec3 target_pos, bool immediate) {
  m_targetPosition = target_pos;
  if (immediate) {
    m_basePosition = target_pos + m_offset;
    m_camera.position = m_basePosition;
  }
}

void CameraController::shake(float intensity, float duration) {
  m_shakeIntensity = intensity;
  m_shakeDuration = duration;
  m_shakeTime = duration;
}
