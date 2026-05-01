

https://github.com/user-attachments/assets/25c51314-7769-4213-8a64-4087c72bcea4


# Petroleum Survival

A 3D top-down wave-survival game built from scratch in C++23 and OpenGL 4.6. You play **The Witch**, an evil gas station manager defending your fuel reserves from a rising tide of pedestrians and vehicles. Survive the waves, level up, and turn the gas station into a kill zone.

> *You are not the good guy.*

Inspired by the *Vampire Survivors* genre, with custom-built rendering, animation, terrain chunking, particle, and gameplay systems — no game engine.

---

## Story

The world has run dry. **Petroleum** is the only thing that matters anymore, and the angry public has decided that what's left in your pumps belongs to them. You — a witch with a chip on her shoulder and a habit of repurposing gas-station inventory into improvised weaponry — would prefer to keep the diesel.

Buddhists are walking off the highway. Militia have spotted your lights from miles away. Cars come barreling through the lot one after another, and the bigger ones don't stop for anybody. You don't owe these people anything. The pumps are yours.

Stand your ground. Throw water bottles. Get rich.

---

## Gameplay

Top-down auto-attack survival. You move; weapons fire on their own cooldowns. Defeat enemies to drop **Money** (the EXP currency) — pick it up to fill the level meter. On level-up, choose between weapon offers (new weapons, or upgrades to ones you already own) and passive items.

You can carry up to 6 active weapons at once, and each weapon levels up to a max level for cooldown and damage scaling. The wave director ramps difficulty over time: humanoids first, then weak cars, standard cars, armored cars, and finally a boss car forced in alongside maintained heavy spawns.

### Controls

| Action | Keys |
|---|---|
| Move | `W` `A` `S` `D` or arrow keys |
| Sprint | `Shift` (held) |
| Navigate menu / level-up offers | `←` `→` or `A` `D` |
| Confirm choice | `Space` or `Enter` |
| Start game | `Space` (from main menu) |
| Quit | `Esc` |

Mouse is supported for clicking menu buttons.

### Starter Loadout

You begin every run with a single weapon: a **Water Bottle**. Everything else is unlocked through level-up draws.

### Weapons

All eight weapons below are reachable through the level-up offer pool. Each scales over up to eight levels, with cooldown reductions and damage bumps at each step.

| Weapon | Type | Description |
|---|---|---|
| **Water Bottle** | Straight projectile | Reliable forward throw. The starter weapon. |
| **Solid Wood Block** | Melee sweep | Heavy short-range swing in the direction you're facing. Pierces. |
| **Toxic Diesel Fumes** | Static aura | Permanent damage aura around you with light knockback. Cannot crit. |
| **Gas Nozzle E20** | Directional cone | Constant cone of damage projecting forward, flamethrower-style, with tick damage at close range. |
| **Gas Nozzle E95** | Directional cone | Higher-octane variant of the E20. |
| **Lighter** | Bouncing projectile | Bounces freely on its own. Synergizes with Gas Nozzles at max level for extra ignition damage. |
| **Magic Wand** | Homing projectile | Auto-targets the nearest enemy. |
| **Orbiting Traffic Cones** | Rotating aura | Heavy cones that orbit you at speed, smashing into anything they touch. |

### Passive Items

| Item | Effect |
|---|---|
| **Heart** | `+20` max health, instant heal `+20`. |
| **Golden Heart** | `+10` max health, instant heal `+10`, `+0.2/sec` health regen. |
| **Running Shoe** | `+10%` movement speed. |

### Enemies

| Enemy | Notes |
|---|---|
| **Buddhist Humanoid** | Animated chasing humanoid mob. |
| **Militia Humanoid** | Larger and more aggressive humanoid variant. |
| **Weak Car** | Low-HP rammers. Authored as a sedan, taxi, pickup, etc. |
| **Standard Car** | Mid-tier cars. |
| **Armored Car** | High-HP, high-knockback-resistance, around `300` HP. |
| **Boss Car** | Heavy buses and monster trucks. Arrives late and tanks everything. |

Cars draw from one of seven authored vehicle models (sedan, muscle, pickup, taxi, police, bus, monster truck) chosen at random per spawn.

---

## Technical Architecture

The codebase is hand-rolled — no game engine, no scene-graph library. Subsystems live under `src/`:

| Module | Responsibility |
|---|---|
| `core/` | App lifecycle, GLFW window, render loop, input |
| `game/` | Game-state machine, wave director, map manager, particle/VFX dispatch, upgrade system |
| `scene/` | Entities (Player, enemies, weapons, projectiles, exp), `GameObjectFactory` registrations |
| `graphics/` | Renderer, Mesh, Model, Camera, Animator, ParticleSystem, lighting, IBL, shadow mapping |
| `resource/` | Enum-based managers for Models, Animations, Textures, Shaders |
| `ui/` | Main/credits menu, HUD, level-up overlay, damage-text animations |
| `utility/` | Random, math helpers, enum maps, type utilities |
| `assets/shaders/` | GLSL sources, embedded into the binary as headers via `xxd` at build time |

### Technical Highlights

- **OpenGL 4.6 core profile** with persistent-mapped SSBOs (`GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT`) for zero-overhead per-frame uploads. Buffer pointers are mapped once at startup; the CPU writes new instance data into them every frame without re-binding or re-mapping.

- **SSBO-driven instanced rendering.** A single `Renderer` exposes a per-frame queue and submits up to `MAX_INSTANCES = 50000` instances per draw via `glDrawElementsInstanced`. Per-instance model matrices and bone palettes for skinned meshes live in two SSBOs (`m_instanceSSBO`, `m_boneSSBO`), so the same mesh can render hundreds of skinned, animated enemies in a single draw call.

- **GPU-batched particle system.** `ParticleSystem` keeps a fixed-size particle pool indexed by `m_poolIndex`, packs each live particle into a tight `ParticleGPUData` struct (positionSize / directionStretch / color) inside a persistent SSBO, and renders the entire system with one `glDrawArraysInstanced(GL_TRIANGLES, 0, 6, activeParticleCount)` call. Toxic fumes, gas nozzle spray, lighter ignition, and damage flashes all flow through this pipeline.

- **Procedural chunked terrain.** `MapManager` streams a 5×5 grid of terrain `Chunk`s (visible radius `2`) around the player. Each chunk owns a procedural Perlin-style heightmap, a triangulated `Mesh`, and tracked dynamic-object handles. `_ensureChunk()` and `updateCurrentChunkObjects()` load and unload chunks as the player crosses chunk boundaries, and cached heightmap lookups give O(1) ground-snap queries for entities (used to keep the player and EXP drops glued to the terrain).

- **Object pooling** for particles to keep per-frame allocations near zero. Enemies and projectiles flow through a typed `GameObjectFactory<T>` so spawn and despawn don't churn through `new` / `delete`.

- **PBR shading** with image-based lighting. An irradiance cubemap is generated at startup from the skybox via `IBLGenerator::generateIrradianceMap`. Materials use albedo / normal / metal-rough / AO / height maps; a height map drives terrain tinting parametrically.

- **Shadow mapping sized to the loaded chunk radius** so shadows cover everything that's currently streamed in and don't pop at chunk seams.

- **Embedded shaders.** All `.glsl` files in `assets/shaders/` are converted to C arrays at build time via `xxd -i` and compiled directly into the binary; no runtime shader file IO.

- **C++23 throughout**, with `glm/fwd.hpp` forward-declared in headers to keep compile times reasonable. Smart pointers everywhere; `const RenderContext &` passed by reference to avoid copies.

- **CPM-fetched dependencies** (GLFW, GLAD, GLM, Assimp) — no system-wide installs of those four libraries are required.

---

## Building from Source

### Build Dependencies

These need to be installed *before* you run CMake:

- **CMake ≥ 3.20** (project tested with 4.3).
- **A C++23 compiler.** MSVC 19.40+ (Visual Studio 2022 17.10+), Clang 17+, or GCC 13+.
- **Python 3 with `Jinja2`.** Required by the GLAD GL loader generator that runs as part of the build. Install with `pip install jinja2`.
- **`xxd`.** Used to embed shaders into the binary as headers. Bundled with Git for Windows; available via `xxd` / `vim-common` on Linux; ships with macOS by default (or via `brew install vim`).

The C++ libraries (GLFW, GLAD, GLM, Assimp) are fetched automatically by CMake via CPM. No manual install is needed for those.

### Windows (Visual Studio 2022)

```bat
git clone https://github.com/Okumans/petroleum-survival.git
cd petroleum-survival
mkdir build
cd build

cmake -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ..

cmake --build . --config Release
```

The `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` is needed because the bundled GLM 0.9.9.8 declares a CMake-compat baseline that's too old for CMake 4.x; setting this lets it configure cleanly.

Two convenience batch files are included in the repo root:

- `build_run.bat` — full configure + build, useful for first-time setup.
- `rebuild.bat` — incremental rebuild only.

The compiled executable lands in `build/bin/petroleum-survivor.exe` alongside its DLLs (`assimp-vc143-mt.dll`, `glad_gl_core_46.dll`, `glfw3.dll`). The `assets/` folder is copied next to the build directory automatically.

Make sure `xxd.exe` and a Python with `jinja2` are on `PATH` when you launch CMake. The easiest way is to start the build from a shell that already has Git and Python available — Git for Windows ships `xxd.exe` under `C:\Program Files\Git\usr\bin\`.

### Linux

```bash
# Debian / Ubuntu
sudo apt update
sudo apt install build-essential cmake git ninja-build python3 python3-pip xxd

pip3 install --user jinja2

git clone https://github.com/Okumans/petroleum-survival.git
cd petroleum-survival

cmake -S . -B build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build -j

./build/bin/petroleum-survivor
```

Assimp pulls in zlib and minizip internally; no extra system libraries are needed beyond a normal C++ build environment and the X11 / Wayland dev headers that come with `build-essential` on most distros.

### macOS

```bash
# Requires Xcode Command Line Tools and Homebrew
xcode-select --install        # if you don't have CLT yet
brew install cmake ninja python git
pip3 install jinja2

# xxd ships with macOS by default; if missing: brew install vim

git clone https://github.com/Okumans/petroleum-survival.git
cd petroleum-survival

cmake -S . -B build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build -j

./build/bin/petroleum-survivor
```

Apple Silicon and Intel Macs both work; CMake will pick the right architecture.

---

## Project Layout

```
petroleum-survival/
├── CMakeLists.txt          # Top-level project + dependency wiring
├── cmake/                  # CPM, FindTargets, package definitions
├── include/                # Vendored headers (e.g. stb_image)
├── src/                    # All gameplay + engine code
├── assets/
│   ├── shaders/            # GLSL, embedded into binary at build time
│   ├── objects/            # GLTF/FBX models + animations
│   ├── textures/           # PBR material sets, skybox, icons
│   └── icons/              # UI icons
├── scripts/                # Python helpers (HDR→cubemap, ARM map unpack)
├── build_run.bat           # Windows: full configure + build
├── rebuild.bat             # Windows: incremental rebuild
└── AGENT.md                # Coding conventions for AI / human contributors
```

---

## Credits

- **Lead Developer:** Jeerabhat Supapinit
- **Second-in-command Developer:** Jane
- Special thanks to **Antigravity**, **Gemini CLI**, and **Codex** for tuning gameplay balance.

Built with love for high-performance C++ gaming. Enjoy the chaos.

---

## License

See `LICENSE` (MIT).
