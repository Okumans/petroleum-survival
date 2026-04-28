#pragma once

#include "scene/weapon/weapon.hpp"

class GasNozzleE95 : public Weapon {
public:
  GasNozzleE95() : Weapon(1.0f, 10.0f) {
    m_id = "gas_nozzle_e95";
    m_name = "Gas Nozzle (E95)";
    m_description = "Sprays premium E95 gas. (Placeholder)";
    m_iconName = "icon_e95_gas_nozzle";
    m_maxLevel = 8;
  }

  std::string getLevelDescription(uint32_t level) const override {
    switch (level) {
    case 1:
      return "Sprays premium E95 gas. (Placeholder)";
    case 2:
      return "Cooldown -10%.";
    case 3:
      return "Damage +5.";
    case 4:
      return "Cooldown -10%.";
    case 5:
      return "Damage +5.";
    case 6:
      return "Cooldown -10%.";
    case 7:
      return "Damage +5.";
    case 8:
      return "Damage +10.";
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
      m_baseDamage += 5.0f;
      break;
    case 4:
      setBaseCooldown(getBaseCooldown() * 0.9f);
      break;
    case 5:
      m_baseDamage += 5.0f;
      break;
    case 6:
      setBaseCooldown(getBaseCooldown() * 0.9f);
      break;
    case 7:
      m_baseDamage += 5.0f;
      break;
    case 8:
      m_baseDamage += 10.0f;
      break;
    default:
      m_baseDamage *= 1.1f;
      break;
    }
  }

  bool fire() override {
    // Placeholder implementation
    return false;
  }
};
