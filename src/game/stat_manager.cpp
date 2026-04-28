#include "game/stat_manager.hpp"

StatManager::StatManager() {
  // Initialize default multipliers
  m_stats[StatType::MIGHT] = 1.0f;
  m_stats[StatType::AREA] = 1.0f;
  m_stats[StatType::COOLDOWN] = 1.0f;
  m_stats[StatType::SPEED] = 1.0f;
  m_stats[StatType::AMOUNT] =
      0.0f; // Amount is usually additive, not a multiplier (e.g. +1 projectile)
  m_stats[StatType::MAGNET] = 1.0f;
  m_stats[StatType::HEALTH_REGEN] = 0.0f;
}

float StatManager::getMultiplier(StatType type) const { return m_stats[type]; }

void StatManager::addMultiplier(StatType type, float value) {
  m_stats[type] += value;
}

void StatManager::setMultiplier(StatType type, float value) {
  m_stats[type] = value;
}
