#include "game/upgrade.hpp"
#include "game/game.hpp"
#include "utility/random.hpp"
#include <algorithm>

std::vector<Upgrade> UpgradeGenerator::generateUpgrades(int count, int level) {
  std::vector<Upgrade> upgrades;

  // Create pool of possible upgrades and randomly select count unique ones
  std::vector<int> pool;
  for (int i = 0; i < 6; ++i) {
    pool.push_back(i);
  }

  // Simple shuffle using random swaps
  for (size_t i = 0; i < pool.size(); ++i) {
    int j = Random::randInt(0, static_cast<int>(pool.size()) - 1);
    std::swap(pool[i], pool[j]);
  }

  for (int i = 0; i < std::min(count, (int)pool.size()); ++i) {
    int type = pool[i];

    // Scale upgrade values with player level
    float levelScale = 1.0f + (level * 0.1f);

    switch (type) {
    case 0: {
      // PLAYER_HEALTH upgrade
      upgrades.emplace_back(
          UpgradeType::PLAYER_HEALTH, "Vitality+", "Increase max health by 25",
          "icons/heart.png", 25.0f * levelScale, [](Game &game) {
            auto player = game.getPlayer();
            if (player) {
              player->setMaxHealth(player->getMaxHealth() * 1.25f);
              player->setHealth(player->getHealth() * 1.25f);
            }
          });
      break;
    }

    case 1: {
      // PLAYER_SPEED upgrade
      upgrades.emplace_back(UpgradeType::PLAYER_SPEED, "Haste+",
                            "Increase movement speed by 20%", "icons/speed.png",
                            0.2f, [](Game &game) {
                              // Speed is applied through animation/locomotion
                              // This would require exposing player speed
                              // multiplier For now, we just mark it as applied
                              (void)game;
                            });
      break;
    }
    case 2: {
      // WEAPON_DAMAGE upgrade (apply to all weapons)
      upgrades.emplace_back(
          UpgradeType::WEAPON_DAMAGE, "Might+", "Increase weapon damage by 30%",
          "icons/sword.png", 0.3f, [](Game &game) {
            auto player = game.getPlayer();
            if (player) {
              for (auto &weapon : player->getWeapons()) {
                weapon->setBaseDamage(weapon->getBaseDamage() * 1.3f);
              }
            }
          });
      break;
    }
    case 3: {
      // WEAPON_COOLDOWN upgrade (faster firing)
      upgrades.emplace_back(UpgradeType::WEAPON_COOLDOWN, "Trigger+",
                            "Reduce weapon cooldown by 25%",
                            "icons/cooldown.png", 0.25f, [](Game &game) {
                              auto player = game.getPlayer();
                              if (player) {
                                for (auto &weapon : player->getWeapons()) {
                                  weapon->setBaseCooldown(
                                      weapon->getBaseCooldown() * 0.75f);
                                }
                              }
                            });
      break;
    }
    case 4: {
      // MIGHT_MULTIPLIER from stat manager
      upgrades.emplace_back(
          UpgradeType::MIGHT_MULTIPLIER, "Power+",
          "Increase damage multiplier by 15%", "icons/power.png", 0.15f,
          [](Game &game) {
            StatManager &stats = game.getStats();
            float current = stats.getMultiplier(StatType::MIGHT);
            stats.setMultiplier(StatType::MIGHT, current * 1.15f);
          });
      break;
    }
    case 5: {
      // COOLDOWN_MULTIPLIER from stat manager
      upgrades.emplace_back(
          UpgradeType::COOLDOWN_MULTIPLIER, "Efficiency+",
          "Reduce cooldown by 10%", "icons/efficiency.png", 0.1f,
          [](Game &game) {
            StatManager &stats = game.getStats();
            float current = stats.getMultiplier(StatType::COOLDOWN);
            stats.setMultiplier(StatType::COOLDOWN, current * 0.9f);
          });
      break;
    }
    }
  }

  return upgrades;
}

Upgrade UpgradeGenerator::generateUpgrade(int level) {
  auto upgrades = generateUpgrades(1, level);
  return upgrades.empty() ? Upgrade() : upgrades[0];
}
