#pragma once

#include "graphics/render_context.hpp"
#include "graphics/model.hpp"
#include "utility/aabb.hpp"

#include <glm/glm.hpp>
#include <memory>

enum class GameObjectType { PLAYER, ENEMY, ITEM, EXP };

class GameObject {
protected:
  std::shared_ptr<Model> m_model;

  bool m_removeRequested = false;

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
             glm::vec3 rotation = glm::vec3(0.0f),
             bool defer_aabb_calculation = false);

  virtual ~GameObject() = default;

  virtual void update(double delta_time) { (void)delta_time; }
  virtual void draw(const RenderContext &ctx) ;

  void requestRemoval() { m_removeRequested = true; }
  [[nodiscard]] bool isRemovalRequested() const { return m_removeRequested; }

  void setHitboxScaleFactor(glm::vec3 factor);
  void setEnableHitboxScaling(bool is_enable);

  void setPosition(const glm::vec3 &pos);
  void setRotation(const glm::vec3 &degrees);
  void setScale(const glm::vec3 &scale);
  void setScale(float scale);

  void translate(const glm::vec3 &offset);
  void rotate(const glm::vec3 &degrees);

  [[nodiscard]] glm::vec3 getPosition() const { return m_position; }
  [[nodiscard]] glm::vec3 getRotation() const { return m_rotation; }
  [[nodiscard]] glm::vec3 getScale() const { return m_scale; }
  [[nodiscard]] virtual GameObjectType getObjectType() const = 0;
  [[nodiscard]] bool isType(GameObjectType type) const {
    return getObjectType() == type;
  }

  [[nodiscard]] const Model &getModel() const { return *m_model; }
  [[nodiscard]] std::shared_ptr<Model> copyModel() const { return m_model; }
  
  void ensureTransformUpdated() const { _updateTransform(); }
  [[nodiscard]] const glm::mat4& getModelMatrix() const { return m_modelMatrix; }
  
  [[nodiscard]] virtual const class Animator* getAnimator() const { return nullptr; }

  [[nodiscard]] const AABB &getWorldAABB() const;
  [[nodiscard]] AABB getHitboxAABB() const; // Scaled for collisions

  [[nodiscard]] bool collidesWith(const GameObject &other) const;
  [[nodiscard]] bool collidesWith(const AABB &other_box) const;

  void forceRecalculateAABB();

protected:
  void _updateTransform() const;
  static AABB _calculateBaseAABB(const Model &model);
};
