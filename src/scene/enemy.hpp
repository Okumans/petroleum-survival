#pragma once

#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "graphics/animation_state.hpp"
#include "resource/animation_manager.hpp"
#include "scene/humanoid_entity.hpp"
#include "utility/enum_map.hpp"
#include "utility/not_initialized.hpp"
#include "utility/random.hpp"
#include <algorithm>
#include <cassert>
#include <memory>

enum class EnemyAnimation { IDLE, WALKING };
enum class EnemyMode { PATROL, CHASE };
enum class EnemyPatrolState { WAITING, WANDERING };

class Enemy : public HumaniodEntity<EnemyAnimation> {
private:
  AnimationState<float> m_waitingState;

  EnemyMode m_mode = EnemyMode::PATROL;
  EnemyPatrolState m_patrolState = EnemyPatrolState::WAITING;

  glm::vec3 m_playerPosition = glm::vec3(0.0f);
  bool m_hasPlayerPosition = false;

  float m_aggroRange = 5.0f;
  float m_deaggroRange = 8.25f;
  float m_wanderRange = 3.0f;
  float m_chaseStep = 0.8f;

public:
  Enemy(std::shared_ptr<Model> model, glm::vec3 pos = glm::vec3(0.0f),
        glm::vec3 scale = glm::vec3(1.0f), glm::vec3 rotation = glm::vec3(0.0f))
      : HumaniodEntity<EnemyAnimation>(model, pos, scale, rotation) {
    m_iFrameState.duration.init(0.0f);
  }

  [[nodiscard]] GameObjectType getObjectType() const override {
    return GameObjectType::ENEMY;
  }

  void setPlayerPosition(const glm::vec3 &player_position) {
    m_playerPosition = player_position;
    m_hasPlayerPosition = true;
  }

  void setup() override {
    AnimationManager::ensureInit();

    m_animations.set(EnemyAnimation::IDLE,
                     AnimationManager::copy(AnimationName::HATSUNE_MIKU_IDLE));
    m_animations.set(
        EnemyAnimation::WALKING,
        AnimationManager::copy(AnimationName::HATSUNE_MIKU_WALKING));

    assert(m_animations.isInitialized());

    m_animator.init(m_animations.ensureInitialized()
                        .get_checked(EnemyAnimation::IDLE)
                        .get());

    _setupAnimationDuration();
    _enterWaiting();
  }

  virtual void update(double delta_time) override {
    _updateModeBySensing();

    if (m_mode == EnemyMode::CHASE) {
      _updateChaseState();
    } else {
      _updatePatrolState();
    }

    _updateWaitingAnimationState(delta_time);
    HumaniodEntity<EnemyAnimation>::update(delta_time);
  }

  virtual void _updateWaitingAnimationState(double delta_time) {
    if (!m_waitingState.animationStarted)
      return;

    m_waitingState.updateTimer(delta_time);
    float t = m_waitingState.getProgress();

    if (t == 1.0) {
      m_waitingState.animationStarted = false;
    }
  }

  inline virtual void _setupAnimationDuration() override {
    m_locomotion.setup(0.8f, 0.6f);
    m_waitingState.duration.init(0.0f);
  }

private:
  void _updateModeBySensing() {
    if (!m_hasPlayerPosition) {
      return;
    }

    float distance_to_player = glm::distance(m_position, m_playerPosition);

    if (m_mode != EnemyMode::CHASE && distance_to_player <= m_aggroRange) {
      m_mode = EnemyMode::CHASE;
      m_waitingState.reset();
      return;
    }

    if (m_mode == EnemyMode::CHASE && distance_to_player > m_deaggroRange) {
      m_mode = EnemyMode::PATROL;
      _enterWaiting();
    }
  }

  void _updatePatrolState() {
    if (m_patrolState == EnemyPatrolState::WAITING) {
      if (!m_waitingState.animationStarted) {
        _enterWander();
      }
      return;
    }

    if (m_patrolState == EnemyPatrolState::WANDERING && !isMoving()) {
      _enterWaiting();
    }
  }

  void _updateChaseState() {
    if (!m_hasPlayerPosition) {
      return;
    }

    glm::vec3 to_player = m_playerPosition - m_position;
    to_player.y = 0.0f;

    float distance = glm::length(to_player);
    if (distance <= 0.05f) {
      return;
    }

    glm::vec3 chase_step =
        glm::normalize(to_player) * std::min(distance, m_chaseStep);

    moveWithAnimation(chase_step);
  }

  void _enterWaiting() {
    m_patrolState = EnemyPatrolState::WAITING;
    m_waitingState.duration.ensureInitialized() = Random::randFloat(2.0f, 3.0f);
    m_waitingState.startAnimation(0.0f,
                                  m_waitingState.duration.ensureInitialized());
  }

  void _enterWander() {
    m_patrolState = EnemyPatrolState::WANDERING;

    float x = Random::randFloat(-m_wanderRange, m_wanderRange);
    float z = Random::randFloat(-m_wanderRange, m_wanderRange);
    moveWithAnimation({x, 0.0f, z});
  }
};
