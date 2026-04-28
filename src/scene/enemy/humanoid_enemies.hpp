#pragma once

#include "scene/enemy/humanoid_enemy.hpp"

/**
 * @brief Enemy variant using the Hatsune Miku model.
 */
class MikuEnemy : public HumanoidEnemy {
public:
  MikuEnemy(std::shared_ptr<Model> model, glm::vec3 pos = glm::vec3(0.0f),
            glm::vec3 scale = glm::vec3(1.0f),
            glm::vec3 rotation = glm::vec3(0.0f))
      : HumanoidEnemy(model, pos, scale, rotation) {
    m_idleAnimName = AnimationName::HATSUNE_MIKU_IDLE;
    m_walkingAnimName = AnimationName::HATSUNE_MIKU_WALKING;
  }
};

/**
 * @brief Enemy variant using the Militia Human model.
 */
class MilitiaEnemy : public HumanoidEnemy {
public:
  MilitiaEnemy(std::shared_ptr<Model> model, glm::vec3 pos = glm::vec3(0.0f),
               glm::vec3 scale = glm::vec3(1.0f),
               glm::vec3 rotation = glm::vec3(0.0f))
      : HumanoidEnemy(model, pos, scale, rotation) {
    m_idleAnimName = AnimationName::HUMAN_IDLE;
    m_walkingAnimName = AnimationName::HUMAN_WALKING;
    m_baseSpeed = 1.0f; // Slightly faster than Miku
  }
};
