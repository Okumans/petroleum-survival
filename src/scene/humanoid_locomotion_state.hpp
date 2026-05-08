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

  void setup(float position_duration, float rotate_duration) {
    m_positionDurationPerUnit = position_duration;
    m_rotateDurationPer180Deg = rotate_duration;

    positionState.duration = position_duration;
    rotateState.duration = rotate_duration;

    reset();
  }

  [[nodiscard]] bool isMoving() const {
    return positionState.animationStarted || rotateState.animationStarted;
  }

  void startMove(const glm::vec3 &current_position,
                 const glm::vec3 &current_rotation,
                 const glm::vec3 &target_offset,
                 float speed_multiplier = 1.0f) {
    float distance = glm::length(target_offset);
    glm::vec3 new_target_pos = current_position + target_offset;
    float target_yaw =
        glm::degrees(std::atan2(target_offset.x, target_offset.z));

    float yaw_delta = std::remainder(target_yaw - current_rotation.y, 360.0f);
    float abs_yaw_delta = std::abs(yaw_delta);

    float position_duration =
        std::max(m_minPositionDuration,
                 (distance * m_positionDurationPerUnit) / speed_multiplier);
    float rotate_duration =
        std::max(m_minRotateDuration,
                 ((abs_yaw_delta / 180.0f) * m_rotateDurationPer180Deg) /
                     speed_multiplier);

    positionState.duration = position_duration;
    rotateState.duration = rotate_duration;

    positionState.startAnimation(current_position, new_target_pos);
    rotateState.startAnimation(current_rotation.y, target_yaw);

    positionState.timer = 0;
    rotateState.timer = 0;
  }

  void reset() {
    rotateState.reset();
    positionState.reset();
  }
};
