# Crossing Road

A high-fidelity 3D "Crossy Road" clone built using a custom C++ OpenGL game engine. This project demonstrates modern graphics techniques including Physically Based Rendering (PBR), Image-Based Lighting (IBL), and dynamic shadow mapping.

___

https://github.com/user-attachments/assets/3da3dba8-40d0-49d2-b299-7242c22b9946

## Features

### Graphics & Rendering
- **Physically Based Rendering (PBR):** Realistic material interaction with light using a custom PBR shader.
- **Image-Based Lighting (IBL):** Environment lighting using irradiance maps generated from HDR skyboxes.
- **Dynamic Shadows:** High-resolution shadow mapping for realistic depth and grounding.
- **Custom Water Shader:** Animated water effects for river terrains.
- **Skybox:** Immersive environment backgrounds.
- **Post-Processing & Debugging:** Includes AABB visualization and debug drawers for development.

### Gameplay
- **Dynamic Map Generation:** Procedural terrain generation including grass, roads, and rivers.
- **Smooth Animations:** Parabolic jump arcs for the player character.
- **Variety of Obstacles:** Cars, trains, and river hazards (lilypads).
- **Scoring System:** Tracks progress based on the furthest row reached.

### Engine Architecture
- **Resource Management:** Centralized managers for textures, models, shaders, and materials.
- **Model Loading:** Supports complex 3D models via Assimp integration.
- **UI System:** Custom UI manager with bitmap font support for overlays and menus.

## Getting Started

### Prerequisites
- **C++ Compiler:** Supporting C++20 or later.
- **CMake:** Version 3.20 or higher.
- **OpenGL:** Version 4.5 or higher.
- **xxd:** Used for embedding assets or shaders into the binary.
- **Dependencies:** (Automatically managed via CPM)
  - GLFW (Window & Input)
  - GLAD (OpenGL Loading)
  - GLM (Mathematics)
  - Assimp (Model Loading)

### Building the Project
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

The executable will be located in the `bin` directory of your build folder.

## Controls
- **Space** Jump Forward
- **A** Move Left
- **D** Move Right
- **Esc:** Exit Game

## Project Structure
- `src/core/`: Application entry point and window management.
- `src/game/`: Core game logic, state management, and map generation.
- `src/graphics/`: Rendering engine, shaders, camera, and lighting systems.
- `src/scene/`: Game objects (Player, Car, Row objects) and terrain definitions.
- `src/resource/`: Resource loading and management systems.
- `src/ui/`: User interface and font rendering.
- `assets/`: 3D models (.glb), textures, and GLSL shaders.

## Development Scripts
The `scripts/` directory contains utility tools for asset processing:
- `hdr_to_cubemap.py`: Converts HDR environment maps to cubemaps.
- `unpack_arm.py`: Unpacks ARM (AO, Roughness, Metallic) textures from channel-packed images.

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
