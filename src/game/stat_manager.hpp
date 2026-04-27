#pragma once

#include "utility/enum_map.hpp"

enum class StatType {
  MIGHT,    // Damage multiplier (default 1.0)
  AREA,     // Size multiplier (default 1.0)
  COOLDOWN, // Cooldown multiplier (default 1.0 - lower is faster?) Wait,
            // cooldown reduction usually lowers it. Let's say default 1.0. 0.8
            // means 20% faster.
  SPEED,    // Projectile/Player speed multiplier
  AMOUNT,   // Extra projectiles
  MAGNET    // Pickup radius multiplier
};

class StatManager {
private:
  EnumMap<StatType, float> m_stats;

public:
  StatManager();

  // Get the current multiplier for a given stat.
  float getMultiplier(StatType type) const;

  // Add to the multiplier (e.g., adding 0.1 increases Might by 10%)
  void addMultiplier(StatType type, float value);

  // Set the multiplier explicitly
  void setMultiplier(StatType type, float value);
};
