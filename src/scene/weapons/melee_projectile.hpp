#pragma once

#include "scene/projectile.hpp"

class MeleeProjectile : public Projectile {
private:
  float m_maxLifetime;

public:
  MeleeProjectile(std::shared_ptr<Model> model, glm::vec3 pos, glm::vec3 dir,
                  float speed, float damage, float lifetime = 0.2f)
      : Projectile(model, pos, dir * speed, damage, -1 /* infinite pierce */),
        m_maxLifetime(lifetime) {
    m_lifetime = 0.0f;
  }

  void setMaxLifetime(float lifetime) { m_maxLifetime = lifetime; }

  void update(double delta_time) override {
    // Melee projectiles just sit in the position they were spawned
    // (or relative to player, but for now we just let them move very slightly
    // or stay still)
    m_position += m_velocity * static_cast<float>(delta_time);
    m_lifetime += delta_time;

    if (m_lifetime >= m_maxLifetime) {
      m_removeRequested = true;
    }

    _updateTransform();
  }
};
