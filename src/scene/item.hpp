#pragma once

#include "scene/game_object.hpp"

#include <glm/fwd.hpp>
#include <memory>

class Item : public GameObject {
private:
  float m_spinSpeedDegPerSec = 90.0f;
  bool m_isActive = true;

public:
  Item(std::shared_ptr<Model> model, glm::vec3 pos = glm::vec3(0.0f),
       glm::vec3 scale = glm::vec3(1.0f),
       glm::vec3 rotation = glm::vec3(0.0f),
       float spin_speed_deg_per_sec = 90.0f)
      : GameObject(model, pos, scale, rotation, false),
        m_spinSpeedDegPerSec(spin_speed_deg_per_sec) {}

  [[nodiscard]] GameObjectType getObjectType() const override {
    return GameObjectType::ITEM;
  }

  void update(double delta_time) override {
    if (!m_isActive) {
      return;
    }

    rotate({0.0f, m_spinSpeedDegPerSec * static_cast<float>(delta_time), 0.0f});
  }

  void setSpinSpeed(float spin_speed_deg_per_sec) {
    m_spinSpeedDegPerSec = spin_speed_deg_per_sec;
  }

  void setActive(bool is_active) { m_isActive = is_active; }
  [[nodiscard]] bool isActive() const { return m_isActive; }
};
