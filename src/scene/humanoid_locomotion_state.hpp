#pragma once

#include "graphics/animation_state.hpp"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

struct HumanoidLocomotionState {
private:
  float m_positionDurationPerUnit = 0.3f;
  float m_rotateDurationPer180Deg = 0.2f;
  float m_minPositionDuration = 0.05f;
  float m_minRotateDuration = 0.05f;

public:
  AnimationState<float> rotateState;
  AnimationState<glm::vec3> positionState;

  void setup(float positionDuration, float rotateDuration) {
    m_positionDurationPerUnit = positionDuration;
    m_rotateDurationPer180Deg = rotateDuration;

    positionState.duration = positionDuration;
    rotateState.duration = rotateDuration;

    reset();
  }

  [[nodiscard]] bool isMoving() const {
    return positionState.animationStarted || rotateState.animationStarted;
  }

  void startMove(const glm::vec3 &currentPosition,
                 const glm::vec3 &currentRotation,
                 const glm::vec3 &targetOffset) {
    float distance = glm::length(targetOffset);
    glm::vec3 new_target_pos = currentPosition + targetOffset;
    float target_yaw = glm::degrees(std::atan2(targetOffset.x, targetOffset.z));

    float yaw_delta = std::remainder(target_yaw - currentRotation.y, 360.0f);
    float abs_yaw_delta = std::abs(yaw_delta);

    float position_duration =
        std::max(m_minPositionDuration, distance * m_positionDurationPerUnit);
    float rotate_duration =
        std::max(m_minRotateDuration,
                 (abs_yaw_delta / 180.0f) * m_rotateDurationPer180Deg);

    positionState.duration = position_duration;
    rotateState.duration = rotate_duration;

    positionState.startAnimation(currentPosition, new_target_pos);
    rotateState.startAnimation(currentRotation.y, target_yaw);

    positionState.timer = 0;
    rotateState.timer = 0;
  }

  void reset() {
    rotateState.reset();
    positionState.reset();
  }
};
