#pragma once

#include <functional>
#include <string>
#include <vector>

class Game;

struct Upgrade {
  std::string title;
  std::string description;
  std::string iconName; // TextureManager name (TextureType::UI)
  std::function<void(Game &)> apply; // Callback to apply the upgrade

  Upgrade() = default;
  Upgrade(const std::string &ti, const std::string &desc, const std::string &ic,
          std::function<void(Game &)> cb)
      : title(ti), description(desc), iconName(ic), apply(std::move(cb)) {}
};

class UpgradeGenerator {
public:
  static std::vector<Upgrade> generateUpgrades(Game &game, int count);
};
