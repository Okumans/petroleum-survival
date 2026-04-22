#pragma once

#include "glm/common.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/trigonometric.hpp"
#include "graphics/animation.hpp"
#include "scene/game_object.hpp"
#include "utility/enum_map.hpp"
#include "utility/not_initialized.hpp"
#include "utility/utility.hpp"
#include <concepts>

enum class PlayerAnimation { WALKING };

template <typename T, typename Counter = float>
  requires std::integral<Counter> || std::floating_point<Counter>
struct AnimationState {
  NotInitialized<Counter, "duration"> duration;
  Counter timer;

  T target;
  T start;

  bool animationStarted;

  Counter updateTimer(Counter delta_time) {
    timer += delta_time;
    return timer;
  };

  void startAnimation(T start, T target) {
    this->start = start;
    this->target = target;
    this->timer = 0;
    this->animationStarted = true;
  }

  void reset() {
    animationStarted = false;
    timer = 0;
  }

  [[nodiscard]] Counter getProgress() const noexcept {
    if (duration.ensureInitialized() <= 0)
      return 1.0f;

    Counter t = timer / duration.ensureInitialized();

    if (t > 1.0)
      return 1.0;
    if (t < 0.0)
      return 0.0;
    return t;
  }
};

class Player : public GameObject {
private:
  SettableNotInitialized<EnumMap<PlayerAnimation, Animation>> m_animations;

  AnimationState<float> m_rotateState;
  AnimationState<glm::vec3> m_positionState;

public:
  Player(std::shared_ptr<Model> model, glm::vec3 pos = glm::vec3(0.0f),
         glm::vec3 scale = glm::vec3(1.0f),
         glm::vec3 rotation = glm::vec3(0.0f))
      : GameObject(model, pos, scale, rotation) {
    m_positionState.duration.init(0.3f);
    m_rotateState.duration.init(0.2f);
  }

  void moveWithAnimation(glm::vec3 vec) {
    if (glm::length(vec) < 0.001f)
      return;

    glm::vec3 new_target_pos = m_position + vec;
    float target_yaw = glm::degrees(std::atan2(vec.x, vec.z));

    m_positionState.startAnimation(m_position, new_target_pos);
    m_rotateState.startAnimation(m_rotation.y, target_yaw);

    m_positionState.timer = 0;
    m_rotateState.timer = 0;
  }

  void move(glm::vec3 vec) {
    m_rotateState.reset();
    m_positionState.reset();

    m_position += vec;
    m_rotation = glm::normalize(m_position);
  }

  virtual void update(double delta_time) override {
    _updateRotateAnimationState(delta_time);
    _updatePositionAnimationState(delta_time);
  }

private:
  void _updateRotateAnimationState(double delta_time) {
    if (!m_rotateState.animationStarted)
      return;

    m_rotateState.updateTimer((float)delta_time);
    float t = m_rotateState.getProgress();

    float current_yaw = lerpAngle(m_rotateState.start, m_rotateState.target, t);

    setRotation({0.0f, current_yaw, 0.0f});

    if (t >= 1.0f) {
      m_rotateState.animationStarted = false;
      m_rotation.y = std::fmod(m_rotation.y, 360.0f);
    }
  }

  void _updatePositionAnimationState(double delta_time) {
    if (!m_positionState.animationStarted)
      return;

    m_positionState.updateTimer(delta_time);
    float t = m_positionState.getProgress();

    if (t == 1.0)
      m_positionState.animationStarted = false;

    glm::vec3 current_pos =
        glm::mix(m_positionState.start, m_positionState.target, t);

    setPosition(current_pos);
  }
};
