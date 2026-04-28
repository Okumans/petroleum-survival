#pragma once

#include <functional>
#include <string>
#include <memory>

class Game;

enum class UpgradeType {
  PLAYER_HEALTH,
  PLAYER_SPEED,
  WEAPON_DAMAGE,
  WEAPON_COOLDOWN,
  MIGHT_MULTIPLIER,
  COOLDOWN_MULTIPLIER,
};

struct Upgrade {
  UpgradeType type;
  std::string title;
  std::string description;
  std::string icon; // Path to icon texture (placeholder path)
  float value;      // Multiplier or additive value depending on type
  std::function<void(Game &)> apply; // Callback to apply the upgrade

  Upgrade() = default;
  Upgrade(UpgradeType t, const std::string &ti, const std::string &desc,
          const std::string &ic, float v, std::function<void(Game &)> cb)
      : type(t), title(ti), description(desc), icon(ic), value(v),
        apply(cb) {}
};

class UpgradeGenerator {
public:
  static Upgrade generateUpgrade(int level);
  static std::vector<Upgrade> generateUpgrades(int count, int level);
};
