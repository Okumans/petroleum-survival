#pragma once

#include "game/game_events.hpp"
#include "glm/fwd.hpp"
#include "scene/enemy/enemy.hpp"
#include "scene/weapon/weapon.hpp"
#include "utility/not_initialized.hpp"
#include <glm/glm.hpp>
#include <vector>

class ToxicFumes : public Weapon {
private:
  float m_auraRadius = 3.5f;

public:
  ToxicFumes() : Weapon(0.8f, 6.0f) {}

  bool fire() override {
    glm::vec3 player_pos = m_context.ensureInitialized()->getPlayerPosition();

    m_context.ensureInitialized()->findTargets(
        getRadius(), 100, [this](Enemy *enemy) {
          if (!enemy) {
            return;
          }

          emitEnemyDamage(GameEvents::EnemyDamageRequestedEvent{
              .enemy = enemy,
              .amount = getDamage(),
              .isCritical = false,
              .knockbackDirection = glm::vec3(0.0f),
              .knockbackStrength = 0.0f,
              .hitPosition = enemy->getPosition() + glm::vec3(0.0f, 1.0f, 0.0f),
              .hitEffect = GameEvents::ParticleEffectType::MAGIC_HIT,
          });
        });

    emitParticle(GameEvents::ParticleSpawnRequestedEvent{
        .position = player_pos,
        .direction = glm::vec3(0.0f, 1.0f, 0.0f),
        .length = getRadius(),
        .thickness = glm::max(0.35f, getRadius() * 0.12f),
        .effectId = GameEvents::ParticleEffectType::FUME,
    });

    return true;
  }

  float getRadius() const {
    return m_auraRadius *
           (1.0f + m_context.ensureInitialized()->getStats()->getMultiplier(
                       StatType::AREA) *
                       0.1f);
  }
};
