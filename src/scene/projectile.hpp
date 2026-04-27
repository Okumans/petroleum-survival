#pragma once

#include "scene/game_object.hpp"
#include <functional>
#include <glm/glm.hpp>

class Projectile : public GameObject {
protected:
  glm::vec3 m_velocity;
  float m_damage;
  float m_lifetime;
  std::function<void(Projectile &, double)> m_behaviorCallback;

public:
  Projectile(std::shared_ptr<Model> model, glm::vec3 pos, glm::vec3 velocity,
             float damage, float lifetime,
             std::function<void(Projectile &, double)> behavior = nullptr)
      : GameObject(model, pos, glm::vec3(1.0f), glm::vec3(0.0f), false),
        m_velocity(velocity), m_damage(damage), m_lifetime(lifetime),
        m_behaviorCallback(behavior) {}

  [[nodiscard]] GameObjectType getObjectType() const override {
    return GameObjectType::PLAYER_PROJECTILE;
  }

  void update(double delta_time) override {
    if (m_removeRequested)
      return;

    m_lifetime -= static_cast<float>(delta_time);
    if (m_lifetime <= 0.0f) {
      requestRemoval();
      return;
    }

    if (m_behaviorCallback) {
      m_behaviorCallback(*this, delta_time);
    } else {
      translate(m_velocity * static_cast<float>(delta_time));
    }
  }

  [[nodiscard]] float getDamage() const { return m_damage; }
  [[nodiscard]] glm::vec3 getVelocity() const { return m_velocity; }
};
