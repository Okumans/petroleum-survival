#pragma once

#include "game/game_events.hpp"
#include "game/map_manager.hpp"
#include "graphics/particle_system.hpp"
#include "resource/model_manager.hpp"
#include "scene/enemy/enemy.hpp"
#include "scene/exp.hpp"
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

    Exp exp_clone(ModelManager::copy(ModelName::EXP_GEM_2),
                  enemy->getExpDropAmount(), spawnPos);
    exp_clone.setScale(50.0f);
    exp_clone.setEmissionColor(glm::vec3({0.0, 0.2, 0.3f}));

    auto [exp, exp_handle] = m_objects.emplaceWithHandle<Exp>(exp_clone);
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
};
