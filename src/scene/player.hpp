#pragma once

#include "glm/fwd.hpp"
#include "resource/animation_manager.hpp"
#include "scene/humanoid_entity.hpp"
#include "scene/weapon/weapon.hpp"
#include "utility/enum_map.hpp"
#include "utility/not_initialized.hpp"
#include <cassert>
#include <memory>
#include <vector>

enum class PlayerAnimation { IDLE, WALKING, RUNNING };

class Player : public HumaniodEntity<Entity, PlayerAnimation> {
private:
  std::vector<std::shared_ptr<Weapon>> m_weapons;
  bool m_isRunning = false;
  float m_currentSpeedMultiplier = 1.0f;

public:
  Player(std::shared_ptr<Model> model, glm::vec3 pos = glm::vec3(0.0f),
         glm::vec3 scale = glm::vec3(1.0f),
         glm::vec3 rotation = glm::vec3(0.0f))
      : HumaniodEntity<Entity, PlayerAnimation>(model, pos, scale, rotation) {
    m_iFrameState.duration = 0.2f;
  }

  void addWeapon(std::shared_ptr<Weapon> weapon) {
    m_weapons.push_back(weapon);
  }

  void setRunning(bool running) { m_isRunning = running; }

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

    HumaniodEntity::_setupAnimationDuration();
  }

  void moveWithAnimation(glm::vec3 vec,
                         float speed_multiplier = 1.0f) override {
    (void)speed_multiplier;

    if (glm::length(vec) < 0.001f)
      return;

    if (m_isRunning) {
      _setAnimation(PlayerAnimation::RUNNING);
    } else {
      _setAnimation(PlayerAnimation::WALKING);
    }

    m_locomotion.startMove(this->m_position, this->m_rotation, vec,
                           m_currentSpeedMultiplier);
  }

  void update(double delta_time) override {
    Entity::update(delta_time);

    // Smooth speed multiplier transition (exponential approach)
    float targetMultiplier = m_isRunning ? 1.8f : 1.0f;
    float k = 5.0f;

    m_currentSpeedMultiplier +=
        (targetMultiplier - m_currentSpeedMultiplier) *
        (1.0f - std::exp(-k * static_cast<float>(delta_time)));

    for (auto &w : m_weapons) {
      AABB box = getHitboxAABB();
      glm::vec3 spawnPos =
          m_position + glm::vec3(0.0f, (box.max.y - box.min.y) * 0.5f, 0.0f);

      // Calculate forward vector based on Y rotation (assuming Y is up)
      float rad = glm::radians(m_rotation.y);
      glm::vec3 forward = glm::vec3(sin(rad), 0.0f, cos(rad));

      w->update(delta_time, spawnPos, forward);
    }

    _updateRotateAnimationState(delta_time);
    _updatePositionAnimationState(delta_time);

    if (!m_locomotion.isMoving()) {
      _setAnimation(PlayerAnimation::IDLE);
    }

    _updateAnimation(delta_time);
  }

  void onDeath() override { m_isDead = true; }
};
