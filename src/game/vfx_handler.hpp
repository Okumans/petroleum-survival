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
  using Random = Utility::Random;

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
    case ParticleEffectType::PHYSICAL_HIT:
      _emitPhysicalHit(evt.position);
      break;
    case ParticleEffectType::FLAME:
      _emitFlameBurst(evt);
      break;
    case ParticleEffectType::GAS_E20:
      _emitGasSpray(evt, false);
      break;
    case ParticleEffectType::GAS_E95:
      _emitGasSpray(evt, true);
      break;
    case ParticleEffectType::WOOD_SWEEP:
      _emitWoodSweep(evt);
      break;
    case ParticleEffectType::FUME_IDLE:
      _emitFumeCloud(evt, false);
      break;
    case ParticleEffectType::FUME_ATTACK:
      _emitFumeCloud(evt, true);
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
    glm::vec3 spawn_pos = enemy->getHitboxAABB().getCenter();
    spawn_pos += Random::randVec3(-0.5f, 0.5f);

    auto [exp, exp_handle] =
        m_objects.createWithHandle<Exp>(GameFactories::getExp(), [&](Exp &e) {
          e.setPosition(spawn_pos);
          e.setAmount(enemy->getExpDropAmount());
        });

    m_mapManager.registerObject(exp_handle, exp.getPosition(), true);
  }

private:
  void _emitCollectBurst(const glm::vec3 &position) {
    for (int i = 0; i < 20; ++i) {
      glm::vec3 rand_color(Random::randFloat(0.0f, 1.0f),
                           Random::randFloat(0.0f, 1.0f),
                           Random::randFloat(0.0f, 1.0f));
      rand_color = glm::normalize(rand_color + glm::vec3(0.2f));
      m_particleSystem.emit({.position = position,
                             .velocity = glm::vec3(0.0f, 5.0f, 0.0f),
                             .velocityVariation = glm::vec3(5.0f, 5.0f, 5.0f),
                             .colorBegin = glm::vec4(rand_color, 1.0f),
                             .colorEnd = glm::vec4(rand_color * 0.5f, 0.0f),
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

  void _emitPhysicalHit(const glm::vec3 &position) {
    for (int i = 0; i < 26; ++i) {
      bool spark = (i % 5) == 0;

      glm::vec3 vel = glm::vec3(0.0f, 1.6f, 0.0f) +
                      Random::randVec3(-1.0f, 1.0f) * (spark ? 4.5f : 2.2f);

      glm::vec4 begin = spark ? glm::vec4(1.0f, 0.95f, 0.75f, 1.0f)
                              : glm::vec4(0.55f, 0.50f, 0.45f, 0.9f);
      glm::vec4 end = spark ? glm::vec4(0.8f, 0.55f, 0.25f, 0.0f)
                            : glm::vec4(0.25f, 0.22f, 0.20f, 0.0f);

      float size_begin = spark ? 0.10f : 0.22f;
      float size_end = spark ? 0.0f : 0.05f;
      float life = spark ? 0.15f : 0.35f;

      m_particleSystem.emit({.position = position + glm::vec3(0.0f, 0.9f, 0.0f),
                             .velocity = vel,
                             .velocityVariation = glm::vec3(1.2f, 1.0f, 1.2f),
                             .colorBegin = begin,
                             .colorEnd = end,
                             .sizeBegin = size_begin,
                             .sizeEnd = size_end,
                             .sizeVariation = size_begin * 0.35f,
                             .lifeTime = life});
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

    glm::vec3 base_pos = evt.position - (direction * 0.1f);

    float core_size = glm::max(0.06f, thickness * 0.5f);
    float ember_size = glm::max(0.02f, thickness * 0.15f);

    for (int i = 0; i < 65; ++i) {
      bool is_core_particle = (i % 4) != 0;

      float travel = Random::randFloat(0.0f, 1.0f) * evt.length;
      float spread_factor = 0.05f + (travel * 0.25f);

      glm::vec3 lateral = Random::randVec3(-spread_factor, spread_factor);
      lateral -= direction * glm::dot(lateral, direction);

      glm::vec3 spawn_pos = base_pos + lateral + direction * travel;

      float thrust = is_core_particle ? Random::randFloat(2.0f, 4.5f)
                                      : Random::randFloat(4.5f, 8.0f);
      float buoyancy = is_core_particle ? Random::randFloat(0.2f, 0.8f)
                                        : Random::randFloat(1.5f, 3.5f);

      glm::vec3 particle_velocity = direction * thrust +
                                    lateral * Random::randFloat(1.0f, 3.0f) +
                                    glm::vec3(0.0f, buoyancy, 0.0f);

      glm::vec4 color_begin =
          is_core_particle
              ? glm::vec4(1.0f, Random::randFloat(0.25f, 0.45f), 0.0f, 1.0f)
              : glm::vec4(1.0f, Random::randFloat(0.1f, 0.2f), 0.0f, 0.9f);

      glm::vec4 color_end = is_core_particle
                                ? glm::vec4(0.6f, 0.05f, 0.0f, 0.0f)
                                : glm::vec4(0.3f, 0.02f, 0.0f, 0.0f);

      float particle_size_begin = is_core_particle ? core_size : ember_size;
      float particle_size_end =
          is_core_particle ? (core_size * Random::randFloat(1.5f, 2.5f)) : 0.0f;

      m_particleSystem.emit(
          {.position = spawn_pos,
           .velocity = particle_velocity,
           .velocityVariation = glm::vec3(0.4f, 0.4f, 0.4f),
           .direction = direction,
           .colorBegin = color_begin,
           .colorEnd = color_end,
           .sizeBegin = particle_size_begin,
           .sizeEnd = particle_size_end,
           .sizeVariation = particle_size_begin * 0.3f,
           .stretch = 1.5f,
           .stretchVariation = 0.2f,
           .lifeTime = is_core_particle ? Random::randFloat(0.15f, 0.3f)
                                        : Random::randFloat(0.25f, 0.45f)});
    }
  }

  void _emitGasSpray(const GameEvents::ParticleSpawnRequestedEvent &evt,
                     bool darker) {
    glm::vec3 direction = evt.direction;

    if (glm::length(direction) < 0.001f) {
      direction = glm::vec3(0.0f, 0.0f, 1.0f);
    } else {
      direction = glm::normalize(direction);
    }

    float thickness = glm::max(0.12f, evt.thickness);

    glm::vec3 base_pos = evt.position - (direction * 0.1f);

    float core_size = glm::max(0.06f, thickness * 0.55f);
    float mist_size = glm::max(0.025f, thickness * 0.22f);

    glm::vec3 base = darker ? glm::vec3(0.18f, 0.10f, 0.04f)
                            : glm::vec3(0.28f, 0.16f, 0.06f);

    for (int i = 0; i < 70; ++i) {
      bool is_core_particle = (i % 5) != 0;

      float travel = Random::randFloat(0.0f, 1.0f) * evt.length;
      float spread_factor = 0.06f + (travel * 0.28f);

      glm::vec3 lateral = Random::randVec3(-spread_factor, spread_factor);
      lateral -= direction * glm::dot(lateral, direction);

      glm::vec3 spawn_pos = base_pos + lateral + direction * travel;

      float thrust = is_core_particle ? Random::randFloat(1.2f, 2.8f)
                                      : Random::randFloat(2.5f, 4.5f);
      float buoyancy = is_core_particle ? Random::randFloat(0.05f, 0.25f)
                                        : Random::randFloat(0.15f, 0.45f);

      glm::vec3 particle_velocity = direction * thrust +
                                    lateral * Random::randFloat(0.8f, 2.2f) +
                                    glm::vec3(0.0f, buoyancy, 0.0f);

      float tint = is_core_particle ? Random::randFloat(0.85f, 1.15f)
                                    : Random::randFloat(0.7f, 1.05f);

      glm::vec3 begin_rgb = glm::clamp(
          base * tint + glm::vec3(Random::randFloat(-0.03f, 0.03f),
                                  Random::randFloat(-0.02f, 0.02f),
                                  Random::randFloat(-0.015f, 0.015f)),
          0.0f, 1.0f);

      glm::vec4 color_begin =
          glm::vec4(begin_rgb, is_core_particle ? 0.85f : 0.65f);
      glm::vec4 color_end =
          glm::vec4(begin_rgb * (darker ? 0.35f : 0.45f), 0.0f);

      float particle_size_begin = is_core_particle ? core_size : mist_size;
      float particle_size_end =
          is_core_particle ? (core_size * Random::randFloat(2.0f, 3.2f)) : 0.0f;

      m_particleSystem.emit(
          {.position = spawn_pos,
           .velocity = particle_velocity,
           .velocityVariation = glm::vec3(0.55f, 0.35f, 0.55f),
           .direction = direction,
           .colorBegin = color_begin,
           .colorEnd = color_end,
           .sizeBegin = particle_size_begin,
           .sizeEnd = particle_size_end,
           .sizeVariation = particle_size_begin * 0.35f,
           .stretch = 1.0f,
           .stretchVariation = 0.25f,
           .lifeTime = is_core_particle ? Random::randFloat(0.18f, 0.35f)
                                        : Random::randFloat(0.28f, 0.55f)});
    }
  }

  void _emitWoodSweep(const GameEvents::ParticleSpawnRequestedEvent &evt) {
    glm::vec3 direction = evt.direction;
    if (glm::length(direction) < 0.001f) {
      direction = glm::vec3(0.0f, 0.0f, 1.0f);
    } else {
      direction = glm::normalize(direction);
    }

    float length = glm::max(0.8f, evt.length);
    float width = glm::max(0.35f, evt.thickness);

    // Build a simple "sweeping fan" in front of the player by spraying
    // particles in a forward wedge with some lateral variation.
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::cross(direction, up);
    float right_len = glm::length(right);
    if (right_len < 0.001f) {
      right = glm::vec3(1.0f, 0.0f, 0.0f);
    } else {
      right /= right_len;
    }

    glm::vec3 base_pos = evt.position + direction * 0.4f;

    // White sweep with subtle cool tint.
    glm::vec3 base_alpha(0.95f, 0.95f, 1.0f);
    glm::vec3 base_blue(0.85f, 0.88f, 0.98f);

    for (int i = 0; i < 85; ++i) {
      float t = Random::randFloat(0.05f, 1.0f);
      float forward_dist = t * length;

      float lateral_span = width * (0.55f + t * 1.15f);
      float lateral = Random::randFloat(-lateral_span, lateral_span);
      float lift = Random::randFloat(0.0f, 0.12f) + t * 0.18f;

      glm::vec3 spawn_pos = base_pos + direction * forward_dist +
                            right * lateral + glm::vec3(0.0f, lift, 0.0f);

      glm::vec3 velocity = direction * Random::randFloat(1.2f, 3.4f) +
                           right * (lateral * Random::randFloat(0.6f, 1.1f)) +
                           glm::vec3(0.0f, Random::randFloat(0.3f, 1.2f), 0.0f);

      bool bright = (i % 4) == 0;
      glm::vec3 begin_rgb = bright ? base_alpha : base_blue;
      begin_rgb += glm::vec3(Random::randFloat(-0.04f, 0.04f),
                             Random::randFloat(-0.04f, 0.04f),
                             Random::randFloat(-0.02f, 0.02f));
      begin_rgb = glm::clamp(begin_rgb, 0.0f, 1.0f);

      glm::vec4 color_begin = glm::vec4(begin_rgb, bright ? 0.95f : 0.82f);
      glm::vec4 color_end = glm::vec4(begin_rgb * 0.35f, 0.0f);

      float size_begin = bright ? Random::randFloat(0.06f, 0.10f)
                                : Random::randFloat(0.04f, 0.08f);
      float size_end = size_begin * Random::randFloat(1.2f, 2.4f);

      m_particleSystem.emit(
          {.position = spawn_pos,
           .velocity = velocity,
           .velocityVariation = glm::vec3(0.25f, 0.25f, 0.25f),
           .direction = direction,
           .colorBegin = color_begin,
           .colorEnd = color_end,
           .sizeBegin = size_begin,
           .sizeEnd = size_end,
           .sizeVariation = size_begin * 0.35f,
           .stretch = 1.6f,
           .stretchVariation = 0.25f,
           .lifeTime = Random::randFloat(0.18f, 0.32f)});
    }
  }

  void _emitFumeCloud(const GameEvents::ParticleSpawnRequestedEvent &evt,
                      bool is_attack) {
    float true_radius = glm::max(0.5f, evt.length);
    float thickness = glm::max(0.4f, evt.thickness);

    const int ring_particle_count = is_attack ? 128 : 96;
    const float shockwave_lifetime = 0.2f;
    const float base_particle_size =
        is_attack ? thickness * 1.8f : thickness * 1.2f;

    float spawn_radius =
        glm::max(0.1f, true_radius - (base_particle_size * 0.45f));

    // 1. Outer Ring
    for (int i = 0; i < ring_particle_count; ++i) {
      float angle = (glm::two_pi<float>() / ring_particle_count) * i;
      float jittered_radius = spawn_radius + Random::randFloat(-0.3f, 0.3f);

      glm::vec3 ring_offset(std::cos(angle) * jittered_radius,
                            Random::randFloat(0.0f, 0.15f),
                            std::sin(angle) * jittered_radius);

      // Attack: Saturated, opaque toxic green. Passive: Pale, translucent
      // yellow-green.
      glm::vec4 color_begin =
          is_attack
              ? glm::vec4(Random::randFloat(0.1f, 0.3f), // R: Low
                          Random::randFloat(0.8f, 1.0f), // G: Very High
                          Random::randFloat(0.1f, 0.2f), // B: Low
                          Random::randFloat(0.6f, 0.8f)) // Alpha: More opaque
              : glm::vec4(0.8f + Random::randFloat(0.0f, 0.2f),
                          0.9f + Random::randFloat(0.0f, 0.1f),
                          0.7f + Random::randFloat(0.0f, 0.2f),
                          Random::randFloat(0.4f, 0.6f));

      // Attack fades to deep green, passive fades to pale yellow
      glm::vec4 color_end = is_attack ? glm::vec4(0.1f, 0.8f, 0.1f, 0.0f)
                                      : glm::vec4(1.0f, 1.0f, 0.9f, 0.0f);

      // Attack particles burst upwards slightly faster
      glm::vec3 velocity =
          is_attack ? glm::vec3(0.0f, Random::randFloat(0.2f, 0.6f), 0.0f)
                    : glm::vec3(0.0f, Random::randFloat(0.0f, 0.2f), 0.0f);

      m_particleSystem.emit(
          {.position = evt.position + ring_offset,
           .velocity = velocity,
           .velocityVariation = glm::vec3(0.15f),
           .colorBegin = color_begin,
           .colorEnd = color_end,
           .sizeBegin = Random::randFloat(0.05f, 0.2f),
           .sizeEnd = base_particle_size * Random::randFloat(0.6f, 1.4f),
           .sizeVariation = Random::randFloat(0.0f, 0.2f),
           .stretch = 1.0f,
           .lifeTime = shockwave_lifetime * Random::randFloat(0.7f, 1.3f)});
    }

    // 2. Inner Flash / Filler
    int filler_count = is_attack ? 24 : 12; // Double the inner flash on attack
    for (int i = 0; i < filler_count; ++i) {
      float inner_radius = Random::randFloat(0.0f, spawn_radius * 0.7f);
      float inner_angle = Random::randFloat(0.0f, glm::two_pi<float>());

      glm::vec3 inner_offset(std::cos(inner_angle) * inner_radius, 0.1f,
                             std::sin(inner_angle) * inner_radius);

      // Bright neon green flash for attack, pale flash for passive
      glm::vec4 inner_color_begin = is_attack
                                        ? glm::vec4(0.4f, 1.0f, 0.3f, 0.5f)
                                        : glm::vec4(0.95f, 1.0f, 0.85f, 0.25f);

      glm::vec4 inner_color_end = is_attack ? glm::vec4(0.2f, 0.9f, 0.2f, 0.0f)
                                            : glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);

      m_particleSystem.emit(
          {.position = evt.position + inner_offset,
           .velocity = glm::vec3(0.0f, is_attack ? 0.3f : 0.1f, 0.0f),
           .velocityVariation = glm::vec3(0.05f),
           .colorBegin = inner_color_begin,
           .colorEnd = inner_color_end,
           .sizeBegin = thickness * (is_attack ? 1.5f : 1.0f),
           .sizeEnd = thickness * (is_attack ? 3.0f : 2.2f),
           .stretch = 1.0f,
           .lifeTime = shockwave_lifetime * Random::randFloat(1.2f, 2.0f)});
    }
  }
};
