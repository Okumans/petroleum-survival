#pragma once

#include "scene/entity.hpp"

class Enemy : public Entity {
protected:
  float m_baseDamage = 5.0f;
  float m_baseSpeed = 0.8f;

public:
  Enemy(std::shared_ptr<Model> model, glm::vec3 pos = glm::vec3(0.0f),
        glm::vec3 scale = glm::vec3(1.0f), glm::vec3 rotation = glm::vec3(0.0f),
        bool defer_aabb_calculation = false)
      : Entity(model, pos, scale, rotation, defer_aabb_calculation) {}

  virtual ~Enemy() = default;

  [[nodiscard]] GameObjectType getObjectType() const override {
    return GameObjectType::ENEMY;
  }

  virtual float getBaseDamage() const { return m_baseDamage; }
  virtual float getBaseSpeed() const { return m_baseSpeed; }
};
