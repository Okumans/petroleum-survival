#pragma once

#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "graphics/animation_state.hpp"
#include "resource/animation_manager.hpp"
#include "scene/enemy/enemy.hpp"
#include "scene/humanoid_entity.hpp"
#include "utility/enum_map.hpp"
#include "utility/not_initialized.hpp"
#include <algorithm>
#include <cassert>
#include <memory>

enum class EnemyAnimation { IDLE, WALKING };

class HumanoidEnemy : public HumaniodEntity<Enemy, EnemyAnimation> {
public:
  HumanoidEnemy(std::shared_ptr<Model> model, glm::vec3 pos = glm::vec3(0.0f),
                glm::vec3 scale = glm::vec3(1.0f),
                glm::vec3 rotation = glm::vec3(0.0f))
      : HumaniodEntity<Enemy, EnemyAnimation>(model, pos, scale, rotation) {
    m_iFrameState.duration = 0.0f;
    m_health = 25.0f;
    m_maxHealth = 25.0f;
    m_baseDamage = 5.0f;
    m_baseSpeed = 0.8f;
    m_knockbackResist = 0.0f;
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
  }

  virtual void update(double delta_time) override {
    _updateChaseState();

    HumaniodEntity<Enemy, EnemyAnimation>::update(delta_time);
  }

  inline virtual void _setupAnimationDuration() override {
    m_locomotion.setup(0.8f, 0.6f);
  }

private:
  void _updateChaseState() {
    glm::vec3 to_player =
        m_context.ensureInitialized()->getPlayerPosition() - m_position;
    to_player.y = 0.0f;

    float distance = glm::length(to_player);
    if (distance <= 0.05f) {
      return;
    }

    glm::vec3 chase_step =
        glm::normalize(to_player) * std::min(distance, m_baseSpeed);

    moveWithAnimation(chase_step);
  }
};
