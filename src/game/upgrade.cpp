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

  std::vector<Candidate> candidatePool;
  candidatePool.reserve(64);

  const bool hasFreeSlot = player->getWeapons().size() < Player::MAX_WEAPONS;

  for (const auto &offer : weaponPool()) {
    std::shared_ptr<Weapon> proto = offer.create();
    if (!proto) {
      continue;
    }

    const std::string weaponId = proto->getId();
    std::shared_ptr<Weapon> existing = findWeaponById(*player, weaponId);

    if (existing) {
      if (!canLevelUpWeapon(*existing)) {
        continue;
      }

      int nextLevel = existing->getLevel() + 1;
      Candidate c;
      c.id = weaponId;
      c.title = existing->getName() + " (Lv " + std::to_string(nextLevel) + ")";
      c.iconName = existing->getIconName();
      c.description = existing->getLevelDescription(nextLevel);
      c.apply = [weaponId](Game &g) {
        Player *p = g.getPlayer();
        if (!p) {
          return;
        }
        std::shared_ptr<Weapon> w = findWeaponById(*p, weaponId);
        if (w) {
          w->upgrade();
        }
      };
      candidatePool.push_back(std::move(c));
      continue;
    }

    if (!hasFreeSlot) {
      continue;
    }

    Candidate c;
    c.id = weaponId;
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
    candidatePool.push_back(std::move(c));
  }

  if (candidatePool.empty()) {
    return upgrades;
  }

  // Pick unique candidates.
  std::vector<int> indices(candidatePool.size());
  for (int i = 0; i < static_cast<int>(indices.size()); ++i) {
    indices[i] = i;
  }
  for (int i = 0; i < static_cast<int>(indices.size()); ++i) {
    int j = Random::randInt(0, static_cast<int>(indices.size()) - 1);
    std::swap(indices[i], indices[j]);
  }

  const int outCount = std::min(count, static_cast<int>(candidatePool.size()));
  for (int i = 0; i < outCount; ++i) {
    const Candidate &c = candidatePool[indices[i]];
    upgrades.emplace_back(c.title, c.description, c.iconName, c.apply);
  }

  return upgrades;
}
