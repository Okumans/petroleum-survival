#include "scene/game_object.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>

GameObject::GameObject(std::shared_ptr<Model> model, glm::vec3 pos,
                       glm::vec3 scale, glm::vec3 rotation)
    : m_model(model), m_position(pos), m_rotation(rotation), m_scale(scale) {
  if (!m_model) {
    throw std::runtime_error("Attempted to construct GameObject with null model");
  }
  
  m_baseAABB = _calculateBaseAABB(*m_model);
  m_isTransformDirty = true;
}

void GameObject::draw(const RenderContext &ctx) {
  if (!m_model) return;

  _updateTransform();

  ctx.shader.setMat4("u_Model", m_modelMatrix);
  m_model->draw(ctx);
}

void GameObject::setHitboxScaleFactor(glm::vec3 factor) {
  m_hitboxScaleFactor = factor;
}

void GameObject::setEnableHitboxScaling(bool is_enable) {
  m_enableHitboxScaling = is_enable;
}

void GameObject::setPosition(const glm::vec3 &pos) {
  m_position = pos;
  m_isTransformDirty = true;
}

void GameObject::setRotation(const glm::vec3 &degrees) {
  m_rotation = degrees;
  m_isTransformDirty = true;
}

void GameObject::setScale(const glm::vec3 &scale) {
  m_scale = scale;
  m_isTransformDirty = true;
}

void GameObject::setScale(float scale) {
  m_scale = glm::vec3(scale);
  m_isTransformDirty = true;
}

void GameObject::translate(const glm::vec3 &offset) {
  m_position += offset;
  m_isTransformDirty = true;
}

void GameObject::rotate(const glm::vec3 &degrees) {
  m_rotation += degrees;
  m_isTransformDirty = true;
}

const AABB &GameObject::getWorldAABB() const {
  _updateTransform();
  return m_worldAABB;
}

AABB GameObject::getHitboxAABB() const {
  const AABB &world = getWorldAABB();
  if (!m_enableHitboxScaling) return world;

  AABB hitbox = world;
  glm::vec3 center = hitbox.getCenter();
  glm::vec3 size = hitbox.getSize();

  // Apply scale directly to min/max from center
  glm::vec3 scaledHalfSize = (size * 0.5f) * m_hitboxScaleFactor;
  hitbox.min = center - scaledHalfSize;
  hitbox.max = center + scaledHalfSize;

  return hitbox;
}

bool GameObject::collidesWith(const GameObject &other) const {
  return getHitboxAABB().intersects(other.getHitboxAABB());
}

bool GameObject::collidesWith(const AABB &other_box) const {
  return getHitboxAABB().intersects(other_box);
}

void GameObject::forceRecalculateAABB() {
  m_baseAABB = _calculateBaseAABB(*m_model);
  m_isTransformDirty = true;
}

void GameObject::_updateTransform() const {
  if (!m_isTransformDirty) return;

  m_modelMatrix = glm::mat4(1.0f);
  m_modelMatrix = glm::translate(m_modelMatrix, m_position);

  // Apply rotations (Y, then X, then Z, or adjust based on convention)
  m_modelMatrix =
      glm::rotate(m_modelMatrix, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
  m_modelMatrix =
      glm::rotate(m_modelMatrix, glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
  m_modelMatrix =
      glm::rotate(m_modelMatrix, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

  m_modelMatrix = glm::scale(m_modelMatrix, m_scale);

  // Recalculate World AABB
  m_worldAABB = m_baseAABB;
  m_worldAABB.transform(m_modelMatrix);

  m_isTransformDirty = false;
}

AABB GameObject::_calculateBaseAABB(const Model &model) {
  AABB aabb = AABB::empty();
  
  for (const auto &mesh : model.getMeshes()) {
    for (const auto &vertex : mesh.getVertices()) {
      aabb.grow(vertex.position);
    }
  }

  return aabb;
}
