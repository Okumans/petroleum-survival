#pragma once

#include "glm/fwd.hpp"
#include "resource/animation_manager.hpp"
#include "scene/humanoid_entity.hpp"
#include "utility/enum_map.hpp"
#include "utility/not_initialized.hpp"
#include <cassert>
#include <memory>

enum class PlayerAnimation { IDLE, WALKING, RUNNING };

class Player : public HumaniodEntity<PlayerAnimation> {
private:
  bool m_isDancing = false;

public:
  Player(std::shared_ptr<Model> model, glm::vec3 pos = glm::vec3(0.0f),
         glm::vec3 scale = glm::vec3(1.0f),
         glm::vec3 rotation = glm::vec3(0.0f))
      : HumaniodEntity<PlayerAnimation>(model, pos, scale, rotation) {
    m_iFrameState.duration.init(0.2f);
  }

  [[nodiscard]] GameObjectType getObjectType() const override {
    return GameObjectType::PLAYER;
  }

  void setup() override {
    AnimationManager::ensureInit();

    m_animations.set(PlayerAnimation::IDLE,
                     AnimationManager::copy(AnimationName::KASANE_TETO_IDLE));
    m_animations.set(
        PlayerAnimation::WALKING,
        AnimationManager::copy(AnimationName::KASANE_TETO_WALKING));
    m_animations.set(
        PlayerAnimation::RUNNING,
        AnimationManager::copy(AnimationName::KASANE_TETO_RUNNING));

    assert(m_animations.isInitialized());

    m_animator.init(m_animations.ensureInitialized()
                        .get_checked(PlayerAnimation::IDLE)
                        .get());

    HumaniodEntity<PlayerAnimation>::_setupAnimationDuration();
  }

  void moveWithAnimation(glm::vec3 vec) override {
    m_isDancing = false;
    HumaniodEntity<PlayerAnimation>::moveWithAnimation(vec);
  }

  void update(double delta_time) override {
    Entity::update(delta_time);

    _updateRotateAnimationState(delta_time);
    _updatePositionAnimationState(delta_time);

    if (!m_locomotion.isMoving()) {
      _setAnimation(PlayerAnimation::IDLE);
    }

    _updateAnimation(delta_time);
  }
};
