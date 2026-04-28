#pragma once

#include "game/game_events.hpp"
#include "game/map_manager.hpp"
#include "graphics/particle_system.hpp"
#include "scene/enemy/enemy.hpp"
#include "scene/exp.hpp"
#include "scene/game_factories.hpp"
#include "scene/game_object_manager.hpp"
#include "utility/event_bus.hpp"
#include "utility/random.hpp"

#include <glm/glm.hpp>

class VFXHandler {
private:
  ParticleSystem &m_particleSystem;
  GameObjectManager &m_objects;
  MapManager &m_mapManager;
  EventBus &m_eventBus;

public:
  VFXHandler(ParticleSystem &ps, GameObjectManager &gom, MapManager &mm,
             EventBus &eb)
      : m_particleSystem(ps), m_objects(gom), m_mapManager(mm), m_eventBus(eb) {
  }

  void handleParticleSpawn(const GameEvents::ParticleSpawnRequestedEvent &evt) {
    using namespace GameEvents;

    switch (evt.effectId) {
    case ParticleEffectType::EXP_COLLECT:
    case ParticleEffectType::ITEM_COLLECT:
      _emitCollectBurst(evt.position);
      break;
    case ParticleEffectType::ENEMY_DEATH:
      _emitDeathBurst(evt.position);
      break;
    case ParticleEffectType::PLAYER_BLOOD:
      _emitBloodSplatter(evt.position);
      break;
    case ParticleEffectType::MAGIC_HIT:
      _emitMagicHit(evt.position);
      break;
    case ParticleEffectType::FLAME:
      _emitFlameBurst(evt);
      break;
    case ParticleEffectType::FUME:
      _emitFumeCloud(evt);
      break;
    }
  }

  void handleEnemyKilled(const GameEvents::EnemyKilledEvent &evt) {
    Enemy *enemy = static_cast<Enemy *>(evt.enemy);

    // 1. Emit death particles
    m_eventBus.emit(GameEvents::ParticleSpawnRequestedEvent{
        .position = enemy->getPosition(),
        .effectId = GameEvents::ParticleEffectType::ENEMY_DEATH});

    // 2. Spawn XP gem
    glm::vec3 spawnPos = enemy->getHitboxAABB().getCenter();
    spawnPos += Random::randVec3(-0.5f, 0.5f);

    auto [exp, exp_handle] =
        m_objects.createWithHandle<Exp>(GameFactories::getExp(), [&](Exp &e) {
          e.setPosition(spawnPos);
          e.setAmount(enemy->getExpDropAmount());
        });

    m_mapManager.registerObject(exp_handle, exp.getPosition(), true);
  }

private:
  void _emitCollectBurst(const glm::vec3 &position) {
    for (int i = 0; i < 20; ++i) {
      glm::vec3 randColor(Random::randFloat(0.0f, 1.0f),
                          Random::randFloat(0.0f, 1.0f),
                          Random::randFloat(0.0f, 1.0f));
      randColor = glm::normalize(randColor + glm::vec3(0.2f));
      m_particleSystem.emit({.position = position,
                             .velocity = glm::vec3(0.0f, 5.0f, 0.0f),
                             .velocityVariation = glm::vec3(5.0f, 5.0f, 5.0f),
                             .colorBegin = glm::vec4(randColor, 1.0f),
                             .colorEnd = glm::vec4(randColor * 0.5f, 0.0f),
                             .sizeBegin = 0.5f,
                             .sizeEnd = 0.1f,
                             .sizeVariation = 0.2f,
                             .lifeTime = 0.5f});
    }
  }

  void _emitDeathBurst(const glm::vec3 &position) {
    for (int i = 0; i < 30; ++i) {
      m_particleSystem.emit({.position = position + glm::vec3(0.0f, 1.0f, 0.0f),
                             .velocity = glm::vec3(0.0f, 2.0f, 0.0f),
                             .velocityVariation = glm::vec3(4.0f, 4.0f, 4.0f),
                             .colorBegin = glm::vec4(0.5f, 0.0f, 0.8f, 1.0f),
                             .colorEnd = glm::vec4(0.1f, 0.0f, 0.2f, 0.0f),
                             .sizeBegin = 0.6f,
                             .sizeEnd = 0.0f,
                             .sizeVariation = 0.3f,
                             .lifeTime = 0.6f});
    }
  }

  void _emitBloodSplatter(const glm::vec3 &position) {
    for (int i = 0; i < 20; ++i) {
      m_particleSystem.emit({.position = position + glm::vec3(0.0f, 1.0f, 0.0f),
                             .velocity = glm::vec3(0.0f, 2.0f, 0.0f),
                             .velocityVariation = glm::vec3(8.0f, 6.0f, 8.0f),
                             .colorBegin = glm::vec4(0.4f, 0.0f, 0.0f, 0.8f),
                             .colorEnd = glm::vec4(0.1f, 0.0f, 0.0f, 0.0f),
                             .sizeBegin = 0.3f,
                             .sizeEnd = 0.05f,
                             .sizeVariation = 0.15f,
                             .lifeTime = 0.3f});
    }
  }

  void _emitMagicHit(const glm::vec3 &position) {
    for (int i = 0; i < 10; ++i) {
      m_particleSystem.emit({.position = position,
                             .velocity = glm::vec3(0.0f),
                             .velocityVariation = glm::vec3(6.0f),
                             .colorBegin = glm::vec4(0.2f, 0.6f, 1.0f, 1.0f),
                             .colorEnd = glm::vec4(0.0f, 0.2f, 0.8f, 0.0f),
                             .sizeBegin = 0.3f,
                             .sizeEnd = 0.0f,
                             .sizeVariation = 0.1f,
                             .lifeTime = 0.2f});
    }
  }

  void _emitFlameBurst(const GameEvents::ParticleSpawnRequestedEvent &evt) {
    glm::vec3 direction = evt.direction;

    if (glm::length(direction) < 0.001f) {
      direction = glm::vec3(0.0f, 0.0f, 1.0f);
    } else {
      direction = glm::normalize(direction);
    }

    float thickness = glm::max(0.12f, evt.thickness);

    glm::vec3 basePosition = evt.position - (direction * 0.1f);

    float coreSize = glm::max(0.06f, thickness * 0.5f);
    float emberSize = glm::max(0.02f, thickness * 0.15f);

    for (int i = 0; i < 65; ++i) {
      bool isCoreParticle = (i % 4) != 0;

      float travel = Random::randFloat(0.0f, 1.0f) * evt.length;
      float spreadFactor = 0.05f + (travel * 0.25f);

      glm::vec3 lateral = Random::randVec3(-spreadFactor, spreadFactor);
      lateral -= direction * glm::dot(lateral, direction);

      glm::vec3 spawnPos = basePosition + lateral + direction * travel;

      float thrust = isCoreParticle ? Random::randFloat(2.0f, 4.5f)
                                    : Random::randFloat(4.5f, 8.0f);
      float buoyancy = isCoreParticle ? Random::randFloat(0.2f, 0.8f)
                                      : Random::randFloat(1.5f, 3.5f);

      glm::vec3 particleVelocity = direction * thrust +
                                   lateral * Random::randFloat(1.0f, 3.0f) +
                                   glm::vec3(0.0f, buoyancy, 0.0f);

      glm::vec4 colorBegin =
          isCoreParticle
              ? glm::vec4(1.0f, Random::randFloat(0.25f, 0.45f), 0.0f, 1.0f)
              : glm::vec4(1.0f, Random::randFloat(0.1f, 0.2f), 0.0f, 0.9f);

      glm::vec4 colorEnd = isCoreParticle ? glm::vec4(0.6f, 0.05f, 0.0f, 0.0f)
                                          : glm::vec4(0.3f, 0.02f, 0.0f, 0.0f);

      float pSizeBegin = isCoreParticle ? coreSize : emberSize;
      float pSizeEnd =
          isCoreParticle ? (coreSize * Random::randFloat(1.5f, 2.5f)) : 0.0f;

      m_particleSystem.emit(
          {.position = spawnPos,
           .velocity = particleVelocity,
           .velocityVariation = glm::vec3(0.4f, 0.4f, 0.4f),
           .direction = direction,
           .colorBegin = colorBegin,
           .colorEnd = colorEnd,
           .sizeBegin = pSizeBegin,
           .sizeEnd = pSizeEnd,
           .sizeVariation = pSizeBegin * 0.3f,
           .stretch = 1.5f,
           .stretchVariation = 0.2f,
           .lifeTime = isCoreParticle ? Random::randFloat(0.15f, 0.3f)
                                      : Random::randFloat(0.25f, 0.45f)});
    }
  }

  void _emitFumeCloud(const GameEvents::ParticleSpawnRequestedEvent &evt) {
    glm::vec3 direction = evt.direction;
    if (glm::length(direction) < 0.001f) {
      direction = glm::vec3(0.0f, 1.0f, 0.0f);
    } else {
      direction = glm::normalize(direction);
    }

    float radius = glm::max(0.5f, evt.length);
    float thickness = glm::max(0.2f, evt.thickness);

    for (int i = 0; i < 18; ++i) {
      float angle = Random::randFloat(0.0f, glm::two_pi<float>());
      float radialScale = Random::randFloat(0.65f, 1.0f) * radius;
      glm::vec3 ringOffset(std::cos(angle) * radialScale,
                           Random::randFloat(-0.2f, 0.2f),
                           std::sin(angle) * radialScale);
      glm::vec3 drift = ringOffset * 0.2f + glm::vec3(0.0f, 0.9f, 0.0f);

      m_particleSystem.emit({.position = evt.position + ringOffset,
                             .velocity = drift * 1.2f + direction * 0.15f,
                             .velocityVariation = glm::vec3(0.5f, 0.7f, 0.5f),
                             .colorBegin = glm::vec4(0.45f, 0.72f, 0.20f, 0.6f),
                             .colorEnd = glm::vec4(0.10f, 0.18f, 0.05f, 0.0f),
                             .sizeBegin = thickness,
                             .sizeEnd = thickness * 0.35f,
                             .sizeVariation = 0.3f,
                             .stretch = 1.8f,
                             .lifeTime = 0.45f});
    }
  }
};
