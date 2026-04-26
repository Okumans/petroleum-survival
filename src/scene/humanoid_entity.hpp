#pragma once

#include "glm/common.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "graphics/animation.hpp"
#include "graphics/animator.hpp"
#include "scene/entity.hpp"
#include "scene/humanoid_locomotion_state.hpp"
#include "utility/not_initialized.hpp"
#include "utility/utility.hpp"
#include <cassert>
#include <memory>
#include <type_traits>

template <typename AnimationTypes>
  requires std::is_enum_v<AnimationTypes> && requires {
    AnimationTypes::IDLE;
    AnimationTypes::WALKING;
  }
class HumaniodEntity : public Entity {
protected:
  SettableNotInitialized<
      EnumMap<AnimationTypes, std::shared_ptr<Animation>>, "m_animations",
      EnumMapValidator<EnumMap<AnimationTypes, std::shared_ptr<Animation>>>>
      m_animations;
  NotInitialized<Animator, "m_animator"> m_animator;
  AnimationTypes m_playingAnimation = AnimationTypes::IDLE;
  HumanoidLocomotionState m_locomotion;
  glm::vec3 m_knockbackVelocity = glm::vec3(0.0f);

public:
  HumaniodEntity(std::shared_ptr<Model> model, glm::vec3 pos = glm::vec3(0.0f),
                 glm::vec3 scale = glm::vec3(1.0f),
                 glm::vec3 rotation = glm::vec3(0.0f),
                 bool defer_aabb_calculation = false)
      : Entity(model, pos, scale, rotation, defer_aabb_calculation) {}

  virtual void setup() = 0;
  virtual void moveToWithAnimation(glm::vec3 target) {
    moveWithAnimation(target - m_position);
  }
  virtual void moveTo(glm::vec3 target) { move(target - m_position); }
  virtual void moveWithAnimation(glm::vec3 vec) {
    if (glm::length(vec) < 0.001f)
      return;

    _setAnimation(AnimationTypes::WALKING);

    m_locomotion.startMove(m_position, m_rotation, vec);
  }

  virtual void move(glm::vec3 vec) {
    m_locomotion.reset();

    translate(vec);
  }

  virtual void takeDamage(float amount, bool isCritical,
                          glm::vec3 knockbackDir = glm::vec3(0.0f),
                          float knockbackForce = 0.0f) override {
    // Call base takeDamage but prevent it from translating automatically
    Entity::takeDamage(amount, isCritical, glm::vec3(0.0f), 0.0f);

    if (m_isDead || m_removeRequested)
      return;

    if (knockbackForce > 0.0f && m_knockbackResist < 1.0f) {
      float actualKnockback = knockbackForce * (1.0f - m_knockbackResist);

      // Interrupt current movement to prevent setPosition overwriting knockback
      m_locomotion.reset();

      // Set initial high velocity for knockback (e.g. force * scalar)
      m_knockbackVelocity += knockbackDir * (actualKnockback * 10.0f);
    }
  }

  virtual void update(double delta_time) override {
    Entity::update(delta_time);

    _updateRotateAnimationState(delta_time);
    _updatePositionAnimationState(delta_time);

    // Apply knockback with deceleration (lerp)
    if (glm::length(m_knockbackVelocity) > 0.01f) {
      translate(m_knockbackVelocity * static_cast<float>(delta_time));

      // Decelerate: higher velocity at start, slower at stop
      m_knockbackVelocity =
          glm::mix(m_knockbackVelocity, glm::vec3(0.0f), 10.0f * delta_time);
    } else {
      m_knockbackVelocity = glm::vec3(0.0f);
    }

    if (!m_locomotion.isMoving() && glm::length(m_knockbackVelocity) < 0.5f) {
      _setAnimation(AnimationTypes::IDLE);
    }

    _updateAnimation(delta_time);
  }

  [[nodiscard]] const Animator *getAnimator() const override {
    if (m_animator.isInitialized()) {
      return &m_animator.ensureInitialized();
    }
    return nullptr;
  }

protected:
  virtual void _setAnimation(AnimationTypes animation) {
    if (animation != m_playingAnimation) {
      m_playingAnimation = animation;
      float blend_duration =
          (m_playingAnimation == AnimationTypes::WALKING) ? 0.12f : 0.18f;
      m_animator.ensureInitialized().playAnimation(
          m_animations.ensureInitialized()
              .get_checked(m_playingAnimation)
              .get(),
          blend_duration);
    }
  }

  virtual void _updateRotateAnimationState(double delta_time) {
    if (!m_locomotion.rotateState.animationStarted)
      return;

    m_locomotion.rotateState.updateTimer((float)delta_time);
    float t = m_locomotion.rotateState.getProgress();

    float current_yaw = lerpAngle(m_locomotion.rotateState.start,
                                  m_locomotion.rotateState.target, t);

    setRotation({0.0f, current_yaw, 0.0f});

    if (t >= 1.0f) {
      m_locomotion.rotateState.animationStarted = false;
      m_rotation.y = std::fmod(m_rotation.y, 360.0f);
    }
  }

  virtual void _updatePositionAnimationState(double delta_time) {
    if (!m_locomotion.positionState.animationStarted)
      return;

    m_locomotion.positionState.updateTimer(delta_time);
    float t = m_locomotion.positionState.getProgress();

    if (t == 1.0)
      m_locomotion.positionState.animationStarted = false;

    glm::vec3 current_pos = glm::mix(m_locomotion.positionState.start,
                                     m_locomotion.positionState.target, t);

    setPosition(current_pos);
  }

  virtual void _updateAnimation(double delta_time) {
    Animator &animator = m_animator.ensureInitialized();
    animator.updateAnimation(delta_time);
  }

  [[nodiscard]] bool isMoving() const { return m_locomotion.isMoving(); }

  inline virtual void _setupAnimationDuration() {
    m_locomotion.setup(0.3f, 0.2f);
  }
};
