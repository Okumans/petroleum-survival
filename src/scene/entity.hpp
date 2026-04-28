#pragma once

#include "graphics/animation_state.hpp"
#include "scene/enemy/i_enemy_context.hpp"
#include "scene/game_object.hpp"
#include "utility/not_initialized.hpp"
#include <algorithm>
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
  AnimationState<float> m_damageFlashState;

  NotInitialized<IEnemyContext *> m_context;

public:
  Entity(std::shared_ptr<Model> model, glm::vec3 pos = glm::vec3(0.0f),
         glm::vec3 scale = glm::vec3(1.0f),
         glm::vec3 rotation = glm::vec3(0.0f),
         bool defer_aabb_calculation = false)
      : GameObject(model, pos, scale, rotation, defer_aabb_calculation) {
    m_damageFlashState.duration = 0.2f;
  }

  virtual ~Entity() = default;

  // Stats getters/setters
  [[nodiscard]] float getHealth() const { return m_health; }

  virtual void update(double delta_time) override {
    m_iFrameState.updateTimer(static_cast<float>(delta_time));
    m_damageFlashState.updateTimer(static_cast<float>(delta_time));
  }

  [[nodiscard]] glm::vec3 getEmissionColor() const override {
    glm::vec3 base = m_emissionColor;
    if (m_damageFlashState.isFinished())
      return base;

    float p = m_damageFlashState.getProgress();
    // Inverse exponential-like decay: (1-p)^4
    float intensity = std::pow(1.0f - p, 4.0f) * 100.0f;
    return base + glm::vec3(intensity);
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

  void setContext(IEnemyContext *context) { m_context.init(context); }

  virtual void takeDamage(float amount, bool is_critical,
                          glm::vec3 knockback_dir = glm::vec3(0.0f),
                          float knockback_force = 0.0f) {
    if (m_isDead || m_removeRequested || !m_iFrameState.isFinished())
      return;

    m_health -= amount;
    m_iFrameState.reset();
    m_damageFlashState.startAnimation(0.0f, 1.0f);

    // Apply knockback
    if (knockback_force > 0.0f && m_knockbackResist < 1.0f) {
      float actual_knockback = knockback_force * (1.0f - m_knockbackResist);
      translate(knockback_dir * actual_knockback);
    }

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
    requestRemoval();
  }
};
