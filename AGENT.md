# Vampire Survivor Project - Naming Conventions & Practices

This file serves as a reference for AI agents to maintain consistent coding style, naming conventions, and architectural practices across the C++ codebase.

## Naming Conventions

### 1. Variables & Members
- **Local Variables & Parameters:** `snake_case` (e.g., `bone_transform`, `delta_time`).
- **Member Variables (Private/Protected):** Prefix with `m_` and use `camelCase` (e.g., `m_camera`, `m_testObject`).
- **Static Member Variables:** Prefix with `s_` (e.g., `s_models`, `s_minClipY`).

### 2. Classes, Structs & Enums
- **Classes/Structs:** `PascalCase` (e.g., `GameObject`, `RenderContext`, `CameraController`).
- **Enums:** `PascalCase` for the enum name (e.g., `ModelName`, `GameState`).
- **Enum Values:** `UPPER_SNAKE_CASE` (e.g., `KASANE_TETO`, `GAME_OVER`).

### 3. Methods & Functions
- **Public Methods:** `camelCase` (e.g., `updateAnimation()`, `setBaseColor()`).
- **Private/Protected Methods:** Prefix with an underscore `_` and use `camelCase` (e.g., `_setupMesh()`, `_updateTransform()`).
- **Global Functions/C-Style callbacks:** `camelCase` or descriptive prefixed names.

### 4. Constants & Macros
- **Constants (Const/Constexpr):** `UPPER_SNAKE_CASE` (e.g., `MAX_BONES`, `MAX_BONE_INFLUENCE`).
- **Compiler Macros:** `UPPER_SNAKE_CASE` (e.g., `ASSETS_PATH`).

## Architectural Practices

### Headers vs Sources
- Always use `#pragma once` at the top of headers.
- Include heavy external headers (like `<assimp/...>` or `<glm/gtc/...>`) in the `.cpp` file rather than `.hpp` where possible.
- Use forward declarations (e.g., `#include <glm/fwd.hpp>`) in `.hpp` whenever full definitions are not strictly necessary.

### Memory & Pointers
- Utilize smart pointers over raw `new`/`delete` for object management (e.g., `std::unique_ptr<GameObject>`, `std::shared_ptr<Model>`).
- Pass complex objects to functions by `const &` (e.g., `const RenderContext &ctx`).

### Rendering Conventions
- Use robust standard 3D positional systems using `glm::vec3` (`m_position`, `m_scale`, `m_rotation`).
- Mathematical AABB boundaries should be cached natively (`m_worldAABB`) using the standalone robust `AABB` struct. Lazy-cache transformations heavily where applicable to improve C++ runtime performance over iterations over hundreds of models.

### Assets Loading Workflow
- Follow the loading tasks queue paradigm established inside `App::_setupResources()` to safely load Models, Textures, and Shaders.
- `ModelManager`, `TextureManager`, and `ShaderManager` should be utilized for fetching instantiated assets across the active GL Context block.

## Adding New Assets (Required Workflow)

When adding any new asset to the game, follow this checklist in order:

1. Place files in the correct asset folder first.
- Models and animation files: `assets/objects/<asset_name>/...`
- Textures: `assets/textures/<category>/<asset_name>/...`

2. Confirm the intended runtime path.
- Runtime paths should use `ASSETS_PATH` and match the on-disk folder structure.

3. Register model identifiers in the manager enum.
- Add the new model key to `ModelName` in `model_manager.hpp`.
- Load it through `ModelManager::load(...)` during resource setup.

4. Register animation identifiers when needed.
- Add the new animation key to `AnimationName` in `animation_manager.hpp`.
- Load it through `AnimationManager::load(...)` during resource setup.

5. Use the resource setup helper lambdas in `App::_setupResources()`.
- Extend helper lambdas (like model/animation loader helpers) rather than inlining duplicated loading code.
- Add a loading task entry so the asset is loaded via the queue.

6. Consume assets only through managers.
- Do not instantiate runtime model/animation objects ad-hoc in gameplay code.
- Retrieve with manager APIs (`get`/`copy`) after setup is complete.

7. Ensure manager initialization guarantees remain valid.
- If using enum-based managers with `ensureInit()`, all enum entries must be loaded by setup tasks before gameplay starts.
