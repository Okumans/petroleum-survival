# Vampire Survivor - Balancing & System Documentation

## Enemy Spawn System

### Mixed On-Screen/Off-Screen Spawning

The `EnemySpawner` now supports configurable mixed spawning:

- **Default Settings:**
  - On-screen spawn fraction: 30% (0.3)
  - On-screen radius: 12 units (close to camera)
  - Off-screen radius: 25 units (outer spawn area)

- **Configuration:**
  - Use `setOnScreenSpawnFraction(float)` to adjust the ratio of on-screen vs off-screen spawns
  - Use `setOnScreenRadius(float)` to define the "visible" area
  - Use `setOffScreenRadius(float)` to define the maximum spawn distance

- **Behavior:**
  - Enemies spawning within on-screen radius appear close to the player (harder)
  - Enemies spawning in the outer ring appear further away (gives player reaction time)
  - The `spawnMixed()` method automatically distributes spawns based on the fraction

## Enemy Tier System & Experience Drops

### Tier Mapping

Enemies are classified into 4 tiers with corresponding experience drops:

| Tier          | Type             | Base XP | Multiplier         | Final XP |
| ------------- | ---------------- | ------- | ------------------ | -------- |
| 0 (Common)    | HumanoidEnemy    | 20      | 1.0 + (tier × 0.5) | 20       |
| 1 (Elite)     | WeakCarEnemy     | 100     | 1.5                | 150      |
| 2 (Boss)      | StandardCarEnemy | 500     | 2.0                | 1000     |
| 3 (Legendary) | BossCarEnemy     | 1000    | 2.5                | 2500     |

### Factory Integration

All enemy types now use the factory system:

- **HumanoidEnemy**: `GameFactories::getHumanoidEnemy()`
- **CarEnemy variants**: `GameFactories::getCar(ModelName)`
- Factories copy prototypes and apply modifiers via lambdas
- Each spawned enemy has exp and stat values configured via `setExpDropAmount()` and health multiplier

## Player Weapon System

### Getters & Setters

**New Player Methods:**

```cpp
const std::vector<std::shared_ptr<Weapon>>& getWeapons() const;
std::vector<std::shared_ptr<Weapon>>& getWeapons();
```

**New Weapon Methods:**

```cpp
void setBaseDamage(float damage);
void setBaseCooldown(float cooldown);
float getBaseDamage() const;
float getBaseCooldown() const;
```

**Usage in Upgrades:**

- Upgrades iterate through `player->getWeapons()` and call setters
- Damage and cooldown changes apply multiplicatively:
  - Damage: `weapon->setBaseDamage(weapon->getBaseDamage() * 1.3f)` (30% increase)
  - Cooldown: `weapon->setBaseCooldown(weapon->getBaseCooldown() * 0.75f)` (25% faster)

## Level-Up & Upgrade System

### Upgrade Generation

The `UpgradeGenerator` creates 3 random upgrades when the player levels up:

```cpp
std::vector<Upgrade> UpgradeGenerator::generateUpgrades(int count, int level);
```

**Upgrade Types:**

| Type | Effect | Scaling |
|------|--------|---------|
| PLAYER_HEALTH | +25% max health | 1.25× multiplier |
| PLAYER_SPEED | +20% movement speed | (Placeholder) |
| WEAPON_DAMAGE | +30% weapon damage | 1.3× to all weapons |
| WEAPON_COOLDOWN | -25% weapon cooldown | 0.75× to all weapons |
| MIGHT_MULTIPLIER | +15% stat multiplier | StatManager multiplier |
| COOLDOWN_MULTIPLIER | -10% cooldown multiplier | StatManager multiplier |

### Level Scaling

Upgrade values scale with player level:

```cpp
float levelScale = 1.0f + (level * 0.1f);
// At level 1: 1.1× base value
// At level 5: 1.5× base value
// At level 10: 2.0× base value
```

### XP Progression

- **Starting requirement**: 10 XP to reach level 2
- **Scaling formula**: `nextRequirement = currentRequirement × 1.5`
  - Level 2: 10 XP
  - Level 3: 15 XP
  - Level 4: 23 XP
  - Level 5: 34 XP
  - etc.

## Game State Machine

### LEVEL_UP State

When player levels up:

1. Game state transitions to `GameState::LEVEL_UP`
2. `UpgradeGenerator::generateUpgrades(3, m_currentLevel)` creates 3 candidates
3. Candidates stored in `m_levelUpCandidates`
4. Player can select an upgrade or skip

### Selection & Application

- Call `game.selectLevelUpOption(index)` to mark selected upgrade
- Call `game.confirmLevelUpSelection()` to apply and return to playing
- Call `game.skipLevelUp()` to skip upgrades and return to playing
- Use `game.getLevelUpCandidates()` to access candidates for UI rendering
- Use `game.getLevelUpSelection()` to check current selection

## Tuning Values

### Enemy Health Scaling

Default behavior: `health *= health_multiplier`

- Wave 1 (0-60s): 1.0×
- Wave 2 (60-120s): 1.0 + (time - 60) / 60 (scales from 1.0 to 2.0)
- Default (no wave): 1.0 + (gameTime × 0.05)

### Factory Initialization Examples

**HumanoidEnemy:**

```cpp
HumanoidEnemy enemy(ModelManager::copy(ModelName::HATSUNE_MIKU));
enemy.setScale(60.0f);
enemy.setup();
```

**CarEnemy (Generic):**

```cpp
CarEnemy car(ModelManager::copy(model));
car.setScale(0.8f);
```

### Recommended Balance Tweaks

1. **For harder difficulty**: Increase `m_onScreenSpawnFraction` (e.g., 0.6) to spawn more enemies near player
2. **For easier difficulty**: Decrease it (e.g., 0.1)
3. **For scaling**: Adjust `exp * levelScale` multiplier in `generateUpgrades()`
4. **For weapon power**: Adjust multipliers (1.3f for damage, 0.75f for cooldown) in upgrade definitions

## Known Limitations & TODOs

- **LevelUI**: UI rendering for XP bar and level up screen.
- **Speed upgrade**: PLAYER_SPEED upgrade is a placeholder; actual player speed multiplier needs implementation in locomotion system
- **Icon paths**: Upgrade icons use placeholder paths (e.g., `"icons/heart.png"`); update with actual asset paths
- **Mobile/Console controls**: Level-up selection currently assumes keyboard/mouse input via UIManager callbacks

## File Structure

```
src/game/
├── upgrade.hpp        # Upgrade struct and UpgradeGenerator
├── upgrade.cpp        # Upgrade generation logic
├── enemy_spawner.hpp  # Enhanced spawner with mixed spawn support
└── enemy_spawner.cpp  # Tier-based exp and factory usage

src/ui/
├── ui_level_up.hpp       # LevelUI class (HUD + Level Up)
└── ui_level_up.cpp       # UI rendering

src/scene/
├── player.hpp         # getWeapons() getters added
├── weapon/weapon.hpp  # setBaseDamage/Cooldown methods added
└── game_factories.*   # Already supports all enemy types via factory pattern
```
