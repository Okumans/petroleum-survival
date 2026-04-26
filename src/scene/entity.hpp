#pragma once

#include "graphics/animation_state.hpp"
#include "scene/game_object.hpp"
#include <algorithm>
#include <print>
#include <vector>

struct ItemDrop {
  int itemId;
  float probability;
};

class Entity : public GameObject {
protected:
  float m_health = 100.0f;
  float m_maxHealth = 100.0f;

  float m_knockbackResist = 0.0f; // 0.0 to 1.0 (1.0 = immune)

  float m_baseDamage = 10.0f;
  float m_critMultiplier = 1.5f;
  float m_critProbability = 0.05f; // 5%

  float m_expDropAmount = 10.0f;
  std::vector<ItemDrop> m_itemDrops;

  bool m_isDead = false;

  AnimationState<void> m_iFrameState;

public:
  Entity(std::shared_ptr<Model> model, glm::vec3 pos = glm::vec3(0.0f),
         glm::vec3 scale = glm::vec3(1.0f),
         glm::vec3 rotation = glm::vec3(0.0f),
         bool defer_aabb_calculation = false)
      : GameObject(model, pos, scale, rotation, defer_aabb_calculation) {}

  virtual ~Entity() = default;

  // Stats getters/setters
  [[nodiscard]] float getHealth() const { return m_health; }

  virtual void update(double delta_time) override {
    m_iFrameState.updateTimer(static_cast<float>(delta_time));
  }

  [[nodiscard]] float getMaxHealth() const { return m_maxHealth; }
  [[nodiscard]] bool isDead() const { return m_isDead; }

  void setMaxHealth(float maxHealth) { m_maxHealth = maxHealth; }
  void setHealth(float health) {
    m_health = std::clamp(health, 0.0f, m_maxHealth);
    if (m_health <= 0.0f && !m_isDead) {
      onDeath();
    }
  }

  [[nodiscard]] float getBaseDamage() const { return m_baseDamage; }
  void setBaseDamage(float dmg) { m_baseDamage = dmg; }

  [[nodiscard]] float getExpDropAmount() const { return m_expDropAmount; }
  void setExpDropAmount(float exp) { m_expDropAmount = exp; }

  [[nodiscard]] float getKnockbackResist() const { return m_knockbackResist; }
  void setKnockbackResist(float resist) {
    m_knockbackResist = std::clamp(resist, 0.0f, 1.0f);
  }

  virtual void takeDamage(float amount, bool isCritical,
                          glm::vec3 knockbackDir = glm::vec3(0.0f),
                          float knockbackForce = 0.0f) {
    if (m_isDead || m_removeRequested || !m_iFrameState.isFinished())
      return;

    m_health -= amount;
    m_iFrameState.reset();

    // Apply knockback
    if (knockbackForce > 0.0f && m_knockbackResist < 1.0f) {
      float actualKnockback = knockbackForce * (1.0f - m_knockbackResist);
      translate(knockbackDir * actualKnockback);
    }

    // TODO: Emit damage number event for UI

    if (m_health <= 0.0f) {
      m_health = 0.0f;
      onDeath();
    }
  }

  virtual void heal(float amount) {
    if (m_isDead || m_removeRequested)
      return;
    m_health = std::min(m_health + amount, m_maxHealth);
  }

  virtual void onDeath() {
    m_isDead = true;
    std::println("you are pretty much dead");
    // Basic default behavior, the Game loop should listen for this and handle
    // drops
    // requestRemoval(); -> will broke the game, game need player to exists.
  }
};
