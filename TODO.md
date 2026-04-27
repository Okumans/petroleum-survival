# Project TODO List: Gas Station Survivor

## Completed Tasks

### Core Architecture

- [x] Basic game engine structure (App, Game, Renderer).
- [x] Entity-Component-like system for GameObjects.
- [x] PBR Rendering with Skybox and IBL.
- [x] Shadow mapping (directional light).
- [x] Collision detection (AABB-based) and resolution.
- [x] Map/Chunk management for scalability.
- [x] Particle system and VFX handler.
- [x] Event Bus for decoupled communication.
- [x] Stat management system (Might, Area, Cooldown, etc.).

### Gameplay Mechanics

- [x] Player movement and animation state.
- [x] Basic Enemy (Humanoid) with AI tracking and damage.
- [x] Experience (EXP) collection and leveling system logic.
- [x] Basic weapon system supporting projectiles and melee.
- [x] Melee weapon: **Solid Wood Block**.
- [x] Projectile weapon: **Magic Wand** (Placeholder).
- [x] Enemy spawner with wave support.
- [x] Damage text UI.

---

## Remaining Tasks (TODO)

### 1. Theming & Lore (Evil Gas Station Manager)

- [ ] Rename `Exp` to `Money` in code and UI.
- [ ] Replace placeholder models (`HATSUNE_MIKU`, `KASANE_TETO`) with gas station themed assets.
- [ ] Update UI to reflect "Diesel Fuel" as health/resource.

### 2. Economy & Progression

- [ ] Implement Money scaling: Drops should vary based on enemy type ($20, $100, $500, $1000).
- [ ] Implement **Money Bundling**: Consolidate off-screen bills into high-value stacks ($10k bundles).
- [ ] Implement Upgrade Selection UI: Generate 3 random upgrades on level up.
- [ ] Add `GREED` and `DURATION` stats to `StatManager`.

### 3. The Arsenal (New Weapons)

- [ ] **Water Bottles**: Standard straight projectile (Refactor Magic Wand).
- [ ] **M150 Energy Drink**: Erratic, bouncing projectile.
- [ ] **Cup of Coffee**: Arcing projectile with AoE splash.
- [ ] **Gun**: High-velocity piercing projectile.
- [ ] **Lighter**: Bouncing projectile with Gas Nozzle synergy.
- [ ] **Gas Nozzle (E20 / E95)**: Directional cone flamethrower.
- [ ] **Toxic Diesel Fumes**: Permanent aura (Garlic-style).
- [ ] **Orbiting Traffic Cones**: Rotating orbital aura.
- [ ] **Oil Rig**: Placeable turret structure.

### 4. The Opposition (Enemies)

- [x] Register specific car models (Sedan, Muscle, Pickup, Taxi, Police, Bus, Monster Truck).
- [x] Clean up unused car model assets.
- [ ] Implement **Vehicles** (Non-Humanoid):
  - [x] **Sedans/Cars**: High health, high momentum, knockback resistant. (Implemented `CarEnemy` class)
  - [ ] **Motorcycles**: Fast, low health, erratic pathing.
  - [ ] **Trucks (Bosses)**: Massive damage sponges.
- [ ] Add more Humanoid variants: Angry Customers, Protesters/Thieves.

### 5. Polish & Balancing

- [ ] Balance stat multipliers and enemy health scaling.
- [ ] Enhance VFX for weapon-specific effects (Coffee splash, Gas flame).
- [ ] Implement sound effects (Money clink, Engine roars, Sloshing liquids).
