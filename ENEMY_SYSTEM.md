# Enemy Spawner and Enemy Types Documentation

This document provides a detailed overview of the enemy spawning system and the different types of enemies implemented in the game engine, including their C++ definitions and customizable parameters.

## 1. Enemy Spawner (`EnemySpawner`)

The `EnemySpawner` is responsible for managing the lifecycle and spawning logic of enemies based on game time and predefined wave configurations.

### Wave Configuration
Waves are defined using the `WaveConfig` struct:

```cpp
struct WaveConfig {
  float timeStart;
  float timeEnd;
  std::function<void(Game&, float, float)> spawnLogic; // (game, currentTime, delta_time)
};
```

### Spawning Logic
- **`update(float currentTime, float delta_time)`**:
    - Iterates through all registered waves. If the current game time falls within a wave's window, its `spawnLogic` is executed.
    - **Default Spawning**: If no wave is active, the spawner uses a default logic that spawns 3 enemies every 1 second in a circle around the player.
- **Scaling**:
    - **Health Scaling**: The system features linear health scaling. Wave logic can apply additional multipliers to spawned enemies.

---

## 2. Enemy Base Class (`Enemy`)

All enemies inherit from the `Enemy` class, which itself inherits from `Entity`.

```cpp
class Enemy : public Entity {
protected:
  float m_baseDamage = 5.0f;
  float m_baseSpeed = 0.8f;
  
public:
  // ... constructors and virtual methods ...
};
```

### Customizable Parameters (Inherited from `Entity`)

Developers can adjust the following parameters to balance enemies:

| Parameter | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `m_health` | `float` | `100.0f` | Current health of the enemy. |
| `m_maxHealth` | `float` | `100.0f` | Maximum health capacity. |
| `m_knockbackResist` | `float` | `0.0f` | Resistance to knockback (0.0 = full knockback, 1.0 = immune). |
| `m_baseDamage` | `float` | `10.0f` (Entity) / `5.0f` (Enemy) | Damage dealt to the player on collision. |
| `m_baseSpeed` | `float` | `0.8f` | Movement speed multiplier for the AI. |
| `m_expDropAmount` | `float` | `10.0f` | Amount of experience (Exp) dropped upon death. |
| `m_critMultiplier` | `float` | `1.5f` | Multiplier applied when a critical hit is landed *by* the entity (if applicable). |
| `m_critProbability`| `float` | `0.05f` | Probability (0.0 to 1.0) of a critical hit occurring. |
| `m_iFrameState` | `AnimationState`| `0.0s` | Duration of invincibility frames after taking damage. |

### Item Drop System
Enemies can drop items based on a probability table:
```cpp
struct ItemDrop {
  int itemId;
  float probability; // 0.0 to 1.0
};

// Inside Entity/Enemy:
std::vector<ItemDrop> m_itemDrops;
```

---

## 3. Enemy Types and Variants

### A. Humanoid Enemy (`HumanoidEnemy`)
The basic swarm enemy.
- **Base Stats**: 25 HP, 5 Damage, 0.8 Speed.
- **Knockback Resistance**: 0.0 (easily pushed).
- **Visuals**: Scaled at `60.0f` with `HATSUNE_MIKU` model.

### B. Car Enemy Variants
Cars use momentum-based movement and have varying tiers:

| Tier | HP | Damage | Speed | Knockback Resist | Models |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Weak** | 100 | 10 | 0.1 | 0.2 | Sedan, Taxi |
| **Standard** | 250 | 15 | 0.15 | 0.4 | Muscle, Pickup |
| **Armored** | 500 | 25 | 0.08 | 0.8 | Police, Bus |
| **Boss** | 1500 | 40 | 0.12 | 1.0 (Immune) | Monster Truck |

**Boss Special Properties**: 
- Immune to knockback.
- Visual scale set to 1.5x (`BossCarEnemy` constructor).

---

## 4. Spawning Logic Variants

### Circle Spawning
`spawnInCircle(int count, float radius, float healthMultiplier)`
- Spawns a cluster of enemies around the player.
- Useful for "Swarms" or "Ambush" events.

### Specific Spawning
`spawnSpecificEnemy(GameObjectType type, glm::vec3 pos, float health_multiplier)`
- Allows wave logic to instantiate precise tiers (e.g., `BOSS_CAR_ENEMY`) at specific locations.

---

## 5. Summary of Gameplay Constants
- **Aggro Range (Cars)**: `25.0f` units.
- **Wave Transition intervals**: 60 seconds per tier.
- **Exp Spawning**: Exp gems are spawned slightly randomized around the death position via `VFXHandler`.
