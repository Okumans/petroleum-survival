#pragma once

#include "scene/game_object.hpp"

class Exp : public GameObject {
private:
    float m_amount = 10.0f;
    float m_spinSpeedDegPerSec = 180.0f;
    bool m_isCollected = false;

public:
    Exp(std::shared_ptr<Model> model, float amount = 10.0f, glm::vec3 pos = glm::vec3(0.0f),
        glm::vec3 scale = glm::vec3(1.0f), glm::vec3 rotation = glm::vec3(0.0f))
        : GameObject(model, pos, scale, rotation, false), m_amount(amount) {}

    [[nodiscard]] GameObjectType getObjectType() const override {
        return GameObjectType::EXP;
    }

    void update(double delta_time) override {
        if (m_isCollected || m_removeRequested) {
            return;
        }

        rotate({0.0f, m_spinSpeedDegPerSec * static_cast<float>(delta_time), 0.0f});
    }

    [[nodiscard]] float getAmount() const { return m_amount; }
    
    void setCollected() {
        m_isCollected = true;
        requestRemoval();
    }
};
