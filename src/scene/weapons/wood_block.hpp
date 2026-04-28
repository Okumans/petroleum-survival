#pragma once

#include "game/game_events.hpp"
#include "scene/game_factories.hpp"
#include "scene/projectile.hpp"
#include "scene/weapon/weapon.hpp"
#include "scene/weapons/melee_projectile.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <string>

class SolidWoodBlock : public Weapon {
private:
  float m_meleeRange = 2.2f;
  float m_lifetime = 0.18f;

public:
  SolidWoodBlock() : Weapon(1.0f, 20.0f) {
    m_id = "solid_wood_block";
    m_name = "Solid Wood Block";
    m_description = "Swings a heavy block in front of you.";
    m_iconName = "icon_solid_wood_block";
    m_maxLevel = 8;
  }

  std::string getLevelDescription(uint32_t level) const override {
    switch (level) {
    case 1:
      return "Swings a heavy block in front of you.";
    case 2:
      return "Cooldown -10%.";
    case 3:
      return "Damage +10.";
    case 4:
      return "Area +15%.";
    case 5:
      return "Cooldown -10%.";
    case 6:
      return "Damage +10.";
    case 7:
      return "Area +15%.";
    case 8:
      return "Damage +15.";
    default:
      return "Upgrade " + m_name + " to level " + std::to_string(level) + ".";
    }
  }

  void onLevelUp(uint32_t newLevel) override {
    switch (newLevel) {
    case 2:
      setBaseCooldown(getBaseCooldown() * 0.9f);
      break;
    case 3:
      m_baseDamage += 10.0f;
      break;
    case 4:
      m_meleeRange *= 1.15f;
      break;
    case 5:
      setBaseCooldown(getBaseCooldown() * 0.9f);
      break;
    case 6:
      m_baseDamage += 10.0f;
      break;
    case 7:
      m_meleeRange *= 1.15f;
      break;
    case 8:
      m_baseDamage += 15.0f;
      break;
    default:
      m_baseDamage *= 1.1f;
      break;
    }
  }

  bool fire() override {
    glm::vec3 player_pos = m_context.ensureInitialized()->getPlayerPosition();
    glm::vec3 player_forward =
        m_context.ensureInitialized()->getPlayerForward();

    glm::vec3 spawn_pos = player_pos + (player_forward * (m_meleeRange * 0.5f));

    std::shared_ptr<Projectile> proj = std::make_shared<MeleeProjectile>(
        GameFactories::getMeleeProjectile().create([&](MeleeProjectile &p) {
          p.setPosition(spawn_pos);
          p.setVelocity(player_forward * 0.0f);
          p.setDamage(getDamage());
          p.setMaxLifetime(m_lifetime);
          p.setScale(glm::vec3(m_meleeRange, 1.0f, m_meleeRange * 0.5f));
        }));

    emitProjectile(
        GameEvents::ProjectileSpawnRequestedEvent{.projectile = proj});

    return true;
  }
};
