# Game Design Document: Gas Station Survivor

## 1. Core Theme & Lore

You are an **evil gas station manager** tasked with protecting your most precious resource: **Diesel Fuel** (which acts as your health). The general public—ranging from angry pedestrians to reckless vehicles—is swarming your station to steal your oil. You must fend them off using whatever everyday gas station items you can get your hands on.

_You are not the good guy._

---

## 2. Progression & Economy

Unlike traditional survivors where you collect mystical gems, your progression revolves around raw capitalism:

- **Currency (EXP)**: Enemies drop **Money**.
- **Scaling**: Drop values scale based on the difficulty of the enemy defeated ($20, $100, $500, up to $1000 bills).
- **Bundling**: Similar to the "red gem" mechanic, if too much money is left on the ground, off-screen bills will consolidate into massive, high-value stacks (e.g., a $10k bundle) to prevent performance issues and provide a massive payout when collected.

---

## 3. The Arsenal (Weapons)

Your weapons are mundane items found around the gas station, repurposed for violence.

### Projectile Weapons

| Weapon                | Mechanic                | Description                                                                                                                                  |
| :-------------------- | :---------------------- | :------------------------------------------------------------------------------------------------------------------------------------------- |
| **Water Bottles**     | Straight Projectile     | A basic, reliable straight-firing projectile. The classic freebie given when customers hit a payment threshold.                              |
| **M150 Energy Drink** | Erratic Projectile      | Fast-moving, hyperactive projectile that bounces wildly, mimicking a caffeine rush.                                                          |
| **Cup of Coffee**     | Arcing Projectile / AoE | Thrown in an arc, exploding into a hot splash Area-of-Effect upon hitting the ground.                                                        |
| **Gun**               | Piercing Projectile     | High velocity, high damage, infinite pierce. Kept behind the counter for emergencies.                                                        |
| **Lighter**           | Bouncing / Synergy      | Acts as a bouncing projectile on its own, but gains massive synergy effects when paired with the Gas Nozzle (igniting fuel for massive AoE). |

### Aura & Melee Weapons

| Weapon                     | Mechanic         | Description                                                                                                                                    |
| :------------------------- | :--------------- | :--------------------------------------------------------------------------------------------------------------------------------------------- |
| **Gas Nozzle (E20 / E95)** | Directional Cone | A constant cone of damage projecting in the direction you face (like a flamethrower). Uses particle systems and a cone hitbox.                 |
| **Solid Wood Block**       | Melee Sweep      | A heavy block of wood swung directly in front of the player. Short range, instant sweep, infinite pierce.                                      |
| **Toxic Diesel Fumes**     | Static Aura      | (Garlic Equivalent) The player reeks of concentrated chemicals, emitting a permanent aura that deals rapid tick damage and light knockback.    |
| **Orbiting Traffic Cones** | Rotating Aura    | (King Bible Equivalent) Heavy traffic cones tied to ropes that rapidly orbit the player, smashing into enemies and providing strong knockback. |

### Tactical Structures

| Weapon      | Mechanic         | Description                                                                                      |
| :---------- | :--------------- | :----------------------------------------------------------------------------------------------- |
| **Oil Rig** | Placeable Turret | A heavy structure deployed on the map. Ticks high damage every $N$ seconds within an $X$ radius. |

---

## 4. The Opposition (Enemies)

The game utilizes a flexible entity architecture to support both animated humans and static rigid bodies.

### Humanoid Enemies

- **Angry Customers**: Basic mob enemies. Standard speed and health.
- **Protesters / Thieves**: Fast-moving variants that try to swarm the fuel reserves quickly.

### Non-Humanoid Enemies (Vehicles)

- **Sedans & Cars**: High health, high momentum enemies that act as battering rams. Resistant to knockback.
- **Motorcycles**: Extremely fast, erratic pathing, but low health.
- **Trucks (Bosses)**: Massive, slow-moving damage sponges that require heavy localized damage (like the Oil Rig or Gun) to take down.

---

## 5. Skills & Upgrades

When leveling up with your accumulated money, you can invest in intrinsic skills to boost your manager stats:

- **Might**: Increases global damage multiplier.
- **Area**: Increases the hitbox size of weapons (larger sweeps, bigger coffee splashes).
- **Cooldown**: Reduces the time between weapon attacks.
- **Amount**: Adds additional projectiles (throw 3 water bottles instead of 1).
- **Speed**: Increases the velocity of projectiles.
- **Duration**: Increases how long effects last (e.g., Oil Rig lifespan, Coffee puddle duration).
- **Greed**: Increases the value of money drops.
