#include "game/upgrade.hpp"

#include "game/game.hpp"
#include "scene/player.hpp"
#include "scene/weapon/weapon.hpp"
#include "scene/weapons/gas_nozzle_e20.hpp"
#include "scene/weapons/gas_nozzle_e95.hpp"
#include "scene/weapons/lighter.hpp"
#include "scene/weapons/magic_wand.hpp"
#include "scene/weapons/orbiting_cones.hpp"
#include "scene/weapons/toxic_fumes.hpp"
#include "scene/weapons/water_bottle.hpp"
#include "scene/weapons/wood_block.hpp"
#include "utility/random.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace {
struct WeaponOffer {
  std::function<std::shared_ptr<Weapon>()> create;
};

struct ItemOffer {
  std::string id;
  std::string title;
  std::string description;
  std::string iconName;
  std::function<void(Game &)> apply;
};

const std::vector<WeaponOffer> &weaponPool() {
  static const std::vector<WeaponOffer> pool = {
      {[]() { return std::make_shared<ToxicFumes>(); }},
      {[]() { return std::make_shared<GasNozzleE20>(); }},
      {[]() { return std::make_shared<GasNozzleE95>(); }},
      {[]() { return std::make_shared<Lighter>(); }},
      {[]() { return std::make_shared<MagicWand>(); }},
      {[]() { return std::make_shared<OrbitingCones>(); }},
      {[]() { return std::make_shared<WaterBottle>(); }},
      {[]() { return std::make_shared<SolidWoodBlock>(); }},
  };
  return pool;
}

const std::vector<ItemOffer> &itemPool() {
  static const std::vector<ItemOffer> pool = {
      {.id = "heart",
       .title = "Heart",
       .description = "Increase max health by 20.",
       .iconName = "icon_heart",
       .apply =
           [](Game &g) {
             Player *p = g.getPlayer();
             if (!p) {
               return;
             }
             p->setMaxHealth(p->getMaxHealth() + 20.0f);
             p->heal(20.0f);
           }},
      {.id = "golden_heart",
       .title = "Golden Heart",
       .description = "Increase max health by 10 and health regen by 0.2/sec.",
       .iconName = "icon_golden_heart",
       .apply =
           [](Game &g) {
             Player *p = g.getPlayer();
             if (!p) {
               return;
             }
             p->setMaxHealth(p->getMaxHealth() + 10.0f);
             p->heal(10.0f);
             g.getStats().addMultiplier(StatType::HEALTH_REGEN, 0.2f);
           }},
      {.id = "running_shoe",
       .title = "Running Shoe",
       .description = "Increase movement speed by 10%.",
       .iconName = "icon_running_shoe",
       .apply =
           [](Game &g) {
             StatManager &stats = g.getStats();
             stats.setMultiplier(StatType::SPEED,
                                 stats.getMultiplier(StatType::SPEED) * 1.10f);
           }},
  };
  return pool;
}

std::shared_ptr<Weapon> findWeaponById(Player &player, const std::string &id) {
  for (auto &w : player.getWeapons()) {
    if (w && w->getId() == id) {
      return w;
    }
  }
  return nullptr;
}

bool canLevelUpWeapon(const Weapon &weapon) {
  return weapon.getLevel() < weapon.getMaxLevel();
}
} // namespace

std::vector<Upgrade> UpgradeGenerator::generateUpgrades(Game &game, int count) {
  std::vector<Upgrade> upgrades;
  upgrades.reserve(static_cast<size_t>(count));

  Player *player = game.getPlayer();
  if (!player) {
    return upgrades;
  }

  // Build a pool of upgradeable candidates:
  // - weapons not equipped yet (if slots remain)
  // - equipped weapons that are not max level
  struct Candidate {
    std::string id;
    std::string title;
    std::string iconName;
    std::string description;
    std::function<void(Game &)> apply;
  };

  std::vector<Candidate> candidate_pool;
  candidate_pool.reserve(64);

  const bool has_free_slot = player->getWeapons().size() < Player::MAX_WEAPONS;

  for (const auto &offer : weaponPool()) {
    std::shared_ptr<Weapon> proto = offer.create();
    if (!proto) {
      continue;
    }

    const std::string weapon_id = proto->getId();
    std::shared_ptr<Weapon> existing = findWeaponById(*player, weapon_id);

    if (existing) {
      if (!canLevelUpWeapon(*existing)) {
        continue;
      }

      int next_level = existing->getLevel() + 1;
      Candidate c;
      c.id = weapon_id;
      c.title =
          existing->getName() + " (Lv " + std::to_string(next_level) + ")";
      c.iconName = existing->getIconName();
      c.description = existing->getLevelDescription(next_level);
      c.apply = [weapon_id](Game &g) {
        Player *p = g.getPlayer();
        if (!p) {
          return;
        }
        std::shared_ptr<Weapon> w = findWeaponById(*p, weapon_id);
        if (w) {
          w->upgrade();
        }
      };
      candidate_pool.push_back(std::move(c));
      continue;
    }

    if (!has_free_slot) {
      continue;
    }

    Candidate c;
    c.id = weapon_id;
    c.title = proto->getName() + " (New)";
    c.iconName = proto->getIconName();
    c.description = proto->getLevelDescription(1);
    c.apply = [offer](Game &g) {
      Player *p = g.getPlayer();
      if (!p) {
        return;
      }
      if (p->getWeapons().size() >= Player::MAX_WEAPONS) {
        return;
      }
      std::shared_ptr<Weapon> w = offer.create();
      if (!w) {
        return;
      }
      w->setContext(&g);
      p->addWeapon(std::move(w));
    };
    candidate_pool.push_back(std::move(c));
  }

  for (const auto &item : itemPool()) {
    Candidate c;
    c.id = item.id;
    c.title = item.title;
    c.iconName = item.iconName;
    c.description = item.description;
    c.apply = item.apply;
    candidate_pool.push_back(std::move(c));
  }

  if (candidate_pool.empty()) {
    return upgrades;
  }

  // Pick unique candidates.
  std::vector<int> indices(candidate_pool.size());
  for (int i = 0; i < static_cast<int>(indices.size()); ++i) {
    indices[i] = i;
  }
  for (int i = 0; i < static_cast<int>(indices.size()); ++i) {
    int j = Utility::Random::randInt(0, static_cast<int>(indices.size()) - 1);
    std::swap(indices[i], indices[j]);
  }

  const int out_count =
      std::min(count, static_cast<int>(candidate_pool.size()));
  for (int i = 0; i < out_count; ++i) {
    const Candidate &c = candidate_pool[indices[i]];
    upgrades.emplace_back(c.title, c.description, c.iconName, c.apply);
  }

  return upgrades;
}
