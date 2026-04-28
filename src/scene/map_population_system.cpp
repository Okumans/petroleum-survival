#include "scene/map_population_system.hpp"
#include "resource/model_manager.hpp"
#include "scene/static_prop.hpp"
#include "utility/random.hpp"

#include <glm/glm.hpp>

void MapPopulationSystem::populateMap(GameObjectManager &objects,
                                      MapManager &mapManager) {
  auto props = _getMapProps();

  for (const auto &prop : props) {
    _spawnProp(objects, mapManager, prop);
  }
}

std::vector<MapPopulationSystem::PropInstance>
MapPopulationSystem::_getMapProps() {
  std::vector<PropInstance> props;

  // Define prop scale factors as per requirements
  constexpr float TREE_1_SCALE = 0.006f * 3;
  constexpr float TREE_2_SCALE = 0.4f * 3;
  constexpr float BUSH_1_SCALE = 0.002f * 3;
  constexpr float BUSH_2_SCALE = 0.0025f * 3;
  constexpr float ROCK_1_SCALE = 0.005f * 3;

  // Create a circular pattern of props around the map
  // Center at (0, 0), with props placed in concentric rings
  const int RING_COUNT = 4;
  const float BASE_RADIUS = 20.0f;
  const float RING_SPACING = 15.0f;

  for (int ring = 1; ring < RING_COUNT; ++ring) {
    float radius = BASE_RADIUS + (ring - 1) * RING_SPACING;
    int props_in_ring = 6 + ring * 2; // More props in outer rings

    for (int i = 0; i < props_in_ring; ++i) {
      float angle = (2.0f * glm::pi<float>() * i) / props_in_ring +
                    Random::randFloat() * 0.3f;
      float x = radius * glm::cos(angle);
      float z = radius * glm::sin(angle);

      // Randomly select which prop to place
      int prop_type = Random::randInt(0, 4); // 5 types: tree1, tree2, bush1,
                                             // bush2, rock1

      switch (prop_type) {
      case 0:
        props.push_back({
            .modelName = ModelName::TREE_1,
            .position = glm::vec3(x, 0.0f, z),
            .rotation = glm::vec3(0.0f, Random::randFloat() * 360.0f, 0.0f),
            .scale = glm::vec3(TREE_1_SCALE),
        });
        break;
      case 1:
        props.push_back({
            .modelName = ModelName::TREE_2,
            .position = glm::vec3(x, 0.0f, z),
            .rotation = glm::vec3(0.0f, Random::randFloat() * 360.0f, 0.0f),
            .scale = glm::vec3(TREE_2_SCALE),
        });
        break;
      case 2:
        props.push_back({
            .modelName = ModelName::BUSH_1,
            .position = glm::vec3(x, 0.0f, z),
            .rotation = glm::vec3(0.0f, Random::randFloat() * 360.0f, 0.0f),
            .scale = glm::vec3(BUSH_1_SCALE),
        });
        break;
      case 3:
        props.push_back({
            .modelName = ModelName::BUSH_2,
            .position = glm::vec3(x, 0.0f, z),
            .rotation = glm::vec3(0.0f, Random::randFloat() * 360.0f, 0.0f),
            .scale = glm::vec3(BUSH_2_SCALE),
        });
        break;
      case 4:
        props.push_back({
            .modelName = ModelName::ROCK_1,
            .position = glm::vec3(x, 0.0f, z),
            .rotation = glm::vec3(0.0f, Random::randFloat() * 360.0f, 0.0f),
            .scale = glm::vec3(ROCK_1_SCALE),
        });
        break;
      }
    }
  }

  // Add some manually placed iconic props near the center
  props.push_back({
      .modelName = ModelName::TREE_2,
      .position = glm::vec3(10.0f, 0.0f, -8.0f),
      .rotation = glm::vec3(0.0f, 45.0f, 0.0f),
      .scale = glm::vec3(TREE_2_SCALE),
  });

  props.push_back({
      .modelName = ModelName::ROCK_1,
      .position = glm::vec3(-12.0f, 0.0f, 5.0f),
      .rotation = glm::vec3(0.0f, 120.0f, 0.0f),
      .scale = glm::vec3(ROCK_1_SCALE),
  });

  props.push_back({
      .modelName = ModelName::BUSH_2,
      .position = glm::vec3(6.0f, 0.0f, 8.0f),
      .rotation = glm::vec3(0.0f, 270.0f, 0.0f),
      .scale = glm::vec3(BUSH_2_SCALE),
  });

  return props;
}

void MapPopulationSystem::_spawnProp(GameObjectManager &objects,
                                     MapManager &mapManager,
                                     const PropInstance &propInstance) {
  // Get the model from the model manager (already loaded during app init)
  auto model = ModelManager::copy(propInstance.modelName);
  if (!model) {
    return; // Model not loaded, skip this prop
  }

  // Create the prop with all specified transforms
  auto [prop, handle] = objects.emplaceWithHandle<StaticProp>(
      model, propInstance.position, propInstance.scale, propInstance.rotation);

  // Register the prop with the map manager for chunk-based culling
  mapManager.registerObject(handle, prop.getPosition(), false);
}
