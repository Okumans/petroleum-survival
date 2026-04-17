#pragma once

#include "graphics/idrawable.hpp"
#include "graphics/model.hpp"
#include "utility/aabb.hpp"

#include <glm/glm.hpp>
#include <memory>

class GameObject : public IDrawable {
protected:
  std::shared_ptr<Model> m_model;

  AABB m_baseAABB;          // Raw un-transformed AABB from model
  mutable AABB m_worldAABB; // Transformed to world coordinates

  // Forgiving hitbox support
  glm::vec3 m_hitboxScaleFactor{0.8f, 1.0f, 0.7f};
  bool m_enableHitboxScaling{true};

  glm::vec3 m_position{0.0f};
  glm::vec3 m_rotation{0.0f}; // Pitch, Yaw, Roll in degrees
  glm::vec3 m_scale{1.0f};

  mutable glm::mat4 m_modelMatrix{1.0f};
  mutable bool m_isTransformDirty = true;

public:
  GameObject(std::shared_ptr<Model> model, glm::vec3 pos = glm::vec3(0.0f),
             glm::vec3 scale = glm::vec3(1.0f),
             glm::vec3 rotation = glm::vec3(0.0f));

  virtual ~GameObject() = default;

  virtual void update(double delta_time) { (void)delta_time; }
  virtual void draw(const RenderContext &ctx) override;

  void setHitboxScaleFactor(glm::vec3 factor);
  void setEnableHitboxScaling(bool is_enable);

  void setPosition(const glm::vec3 &pos);
  void setRotation(const glm::vec3 &degrees);
  void setScale(const glm::vec3 &scale);
  void setScale(float scale);

  void translate(const glm::vec3 &offset);
  void rotate(const glm::vec3 &degrees);

  glm::vec3 getPosition() const { return m_position; }
  glm::vec3 getRotation() const { return m_rotation; }
  glm::vec3 getScale() const { return m_scale; }

  std::shared_ptr<Model> getModel() const { return m_model; }

  const AABB &getWorldAABB() const;
  AABB getHitboxAABB() const; // Scaled for collisions

  bool collidesWith(const GameObject &other) const;
  bool collidesWith(const AABB &other_box) const;

  void forceRecalculateAABB();

protected:
  void _updateTransform() const;
  static AABB _calculateBaseAABB(const Model &model);
};
