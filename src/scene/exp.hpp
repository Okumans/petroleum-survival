#pragma once

#include "scene/game_object.hpp"

class Exp : public GameObject {
private:
  float m_amount = 10.0f;
  float m_spinSpeedDegPerSec = 180.0f;
  bool m_isCollected = false;
  float m_groundY = 0.0f;

public:
  Exp(std::shared_ptr<Model> model, float amount = 10.0f,
      glm::vec3 pos = glm::vec3(0.0f), glm::vec3 scale = glm::vec3(1.0f),
      glm::vec3 rotation = glm::vec3(0.0f))
      : GameObject(model, pos, scale, rotation, false), m_amount(amount) {}

  [[nodiscard]] GameObjectType getObjectType() const override {
    return GameObjectType::EXP;
  }

  void setGroundY(float y) { m_groundY = y; }

  void update(double delta_time) override {
    if (m_isCollected || m_removeRequested) {
      return;
    }

    rotate({0.0f, m_spinSpeedDegPerSec * static_cast<float>(delta_time), 0.0f});

    // Move towards groundY (interpolated for smoothness)
    float targetY = m_groundY;
    float dy = targetY - m_position.y;

    if (std::abs(dy) > 0.001f) {
      // Faster snapping to avoid sinking
      float moveStep = 10.0f * static_cast<float>(delta_time);
      if (std::abs(dy) < moveStep) {
        m_position.y = targetY;
      } else {
        m_position.y += dy * moveStep; // Wait, nosign is not a thing
        m_position.y += (dy > 0 ? 1.0f : -1.0f) * moveStep;
      }
      m_isTransformDirty = true;
    }
  }

  [[nodiscard]] float getAmount() const { return m_amount; }
  void setAmount(float amount) { m_amount = amount; }

  void setCollected() {
    m_isCollected = true;
    requestRemoval();
  }
};
