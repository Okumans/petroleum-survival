#include "scene/static_prop.hpp"

StaticProp::StaticProp(std::shared_ptr<Model> model,
                       glm::vec3 pos,
                       glm::vec3 scale,
                       glm::vec3 rotation)
    : GameObject(model, pos, scale, rotation) {
  // Disable hitbox scaling for static props - they don't interact with physics
  m_enableHitboxScaling = false;
}
