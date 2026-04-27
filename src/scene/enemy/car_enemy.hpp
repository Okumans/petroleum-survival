#pragma once

#include "scene/enemy/enemy.hpp"
#include "scene/humanoid_locomotion_state.hpp"
#include "utility/utility.hpp"
#include <glm/glm.hpp>
#include <algorithm>

class CarEnemy : public Enemy {
private:
  HumanoidLocomotionState m_locomotion;
  glm::vec3 m_playerPosition = glm::vec3(0.0f);
  bool m_hasPlayerPosition = false;

  float m_aggroRange = 25.0f;
  glm::vec3 m_knockbackVelocity = glm::vec3(0.0f);

public:
  CarEnemy(std::shared_ptr<Model> model, glm::vec3 pos = glm::vec3(0.0f),
           glm::vec3 scale = glm::vec3(1.0f), glm::vec3 rotation = glm::vec3(0.0f))
      : Enemy(model, pos, scale, rotation) {
    m_baseSpeed = 1.5f;
    m_knockbackResist = 0.7f;
    m_baseDamage = 20.0f;
    m_health = 300.0f;
    m_maxHealth = 300.0f;
    
    // Cars move with momentum and smooth rotation
    m_locomotion.setup(0.05f, 0.5f); 
  }

  void setPlayerPosition(const glm::vec3 &player_position) override {
    m_playerPosition = player_position;
    m_hasPlayerPosition = true;
  }

  void moveWithAnimation(glm::vec3 vec) {
    if (glm::length(vec) < 0.001f)
      return;

    m_locomotion.startMove(this->m_position, this->m_rotation, vec);
  }

  void update(double delta_time) override {
    Enemy::update(delta_time);

    if (m_hasPlayerPosition) {
      _updateChaseState();
    }

    _updateRotateAnimationState(delta_time);
    _updatePositionAnimationState(delta_time);

    // Apply knockback with deceleration (lerp)
    if (glm::length(m_knockbackVelocity) > 0.01f) {
      this->translate(m_knockbackVelocity * static_cast<float>(delta_time));
      m_knockbackVelocity =
          glm::mix(m_knockbackVelocity, glm::vec3(0.0f), 10.0f * delta_time);
    } else {
      m_knockbackVelocity = glm::vec3(0.0f);
    }
  }

  void takeDamage(float amount, bool isCritical,
                  glm::vec3 knockbackDir = glm::vec3(0.0f),
                  float knockbackForce = 0.0f) override {
    // Call base takeDamage but prevent it from translating automatically
    // We handle translation via m_knockbackVelocity for smooth movement
    Enemy::takeDamage(amount, isCritical, glm::vec3(0.0f), 0.0f);

    if (this->m_isDead || this->m_removeRequested || !m_iFrameState.isFinished())
      return;

    if (knockbackForce > 0.0f && this->m_knockbackResist < 1.0f) {
      float actualKnockback = knockbackForce * (1.0f - this->m_knockbackResist);
      m_locomotion.reset();
      m_knockbackVelocity += knockbackDir * (actualKnockback * 10.0f);
    }
  }

private:
  void _updateChaseState() {
    float distance_to_player = glm::distance(m_position, m_playerPosition);
    if (distance_to_player > m_aggroRange) {
        return;
    }

    glm::vec3 to_player = m_playerPosition - m_position;
    to_player.y = 0.0f;

    float distance = glm::length(to_player);
    if (distance <= 0.1f)
      return;

    glm::vec3 chase_step =
        glm::normalize(to_player) * std::min(distance, m_baseSpeed);

    moveWithAnimation(chase_step);
  }

  void _updateRotateAnimationState(double delta_time) {
    if (!m_locomotion.rotateState.animationStarted)
      return;

    m_locomotion.rotateState.updateTimer((float)delta_time);
    float t = m_locomotion.rotateState.getProgress();

    float current_yaw = lerpAngle(m_locomotion.rotateState.start,
                                  m_locomotion.rotateState.target, t);

    this->setRotation({0.0f, current_yaw, 0.0f});

    if (t >= 1.0f) {
      m_locomotion.rotateState.animationStarted = false;
      this->m_rotation.y = std::fmod(this->m_rotation.y, 360.0f);
    }
  }

  void _updatePositionAnimationState(double delta_time) {
    if (!m_locomotion.positionState.animationStarted)
      return;

    m_locomotion.positionState.updateTimer(delta_time);
    float t = m_locomotion.positionState.getProgress();

    if (t == 1.0)
      m_locomotion.positionState.animationStarted = false;

    glm::vec3 current_pos = glm::mix(m_locomotion.positionState.start,
                                     m_locomotion.positionState.target, t);

    this->setPosition(current_pos);
  }
};
