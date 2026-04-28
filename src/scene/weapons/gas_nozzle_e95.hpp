#pragma once

#include "scene/weapons/gas_nozzle_e20.hpp"

// Same mechanics as GasNozzle (E20) but sprays behind the player and uses a
// darker gas palette.
class GasNozzleE95 : public GasNozzleE20 {
protected:
  [[nodiscard]] GameEvents::ParticleEffectType getSprayEffect() const override {
    return GameEvents::ParticleEffectType::GAS_E95;
  }

  [[nodiscard]] glm::vec3 getSprayForward() const override {
    return -m_context.ensureInitialized()->getPlayerForward();
  }

public:
  GasNozzleE95() : GasNozzleE20() {
    m_id = "gas_nozzle_e95";
    m_name = "Gas Nozzle (E95)";
    m_description = "Sprays premium E95 gas behind you.";
    m_iconName = "icon_e95_gas_nozzle";
    m_baseDamage = 13.0f;
  }
};
