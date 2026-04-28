#pragma once

#include "scene/game_object.hpp"
#include "graphics/model.hpp"

#include <memory>
#include <glm/glm.hpp>

/**
 * @class StaticProp
 * @brief Represents a static environmental object (trees, rocks, bushes, etc.)
 * 
 * Static props are non-interactive environment props that serve as visual clutter
 * and environmental detail. They don't move, don't collide with entities, and don't
 * participate in combat.
 */
class StaticProp : public GameObject {
public:
  /**
   * @brief Construct a static prop
   * @param model The 3D model to use for rendering
   * @param pos The world position
   * @param scale The scale factor
   * @param rotation The rotation in degrees (pitch, yaw, roll)
   */
  StaticProp(std::shared_ptr<Model> model,
             glm::vec3 pos = glm::vec3(0.0f),
             glm::vec3 scale = glm::vec3(1.0f),
             glm::vec3 rotation = glm::vec3(0.0f));

  ~StaticProp() = default;

  /**
   * @brief Get the object type
   * @return GameObjectType::STATIC_PROP
   */
  [[nodiscard]] GameObjectType getObjectType() const override {
    return GameObjectType::STATIC_PROP;
  }

  /**
   * @brief Update is a no-op for static props
   * @param delta_time Time elapsed since last update (unused)
   */
  void update(double delta_time) override { (void)delta_time; }
};
