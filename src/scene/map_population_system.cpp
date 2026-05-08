#include "scene/map_population_system.hpp"
#include "resource/model_manager.hpp"
#include "scene/static_prop.hpp"
#include "utility/random.hpp"

#include <glm/glm.hpp>

using Random = Utility::Random;

void MapPopulator::populateMap(GameObjectManager &objects,
                               MapManager &map_manager) {
  auto props = _getMapProps();

  for (const auto &prop : props) {
    _spawnProp(objects, map_manager, prop);
  }
}

std::vector<MapPopulator::PropInstance> MapPopulator::_getMapProps() {
  std::vector<PropInstance> props;

  // Define prop scale factors
  constexpr float tree_1_scale = 0.006f * 4;
  constexpr float tree_2_scale = 0.4f * 4;
  constexpr float bush_1_scale = 0.002f * 3;
  constexpr float bush_2_scale = 0.0025f * 3;
  constexpr float rock_1_scale = 0.005f * 3;

  auto randomize_scale = [](float base_scale, float min_mul = 0.9f,
                            float max_mul = 1.15f) {
    return glm::vec3(base_scale * Random::randFloat(min_mul, max_mul));
  };

  // Helper function to heavily favor spawning trees (60% trees, 20% bushes, 20%
  // rocks)
  auto get_random_prop_type = []() {
    int r = Random::randInt(0, 9);
    if (r < 3)
      return 0; // 30% chance TREE_1
    if (r < 6)
      return 1; // 30% chance TREE_2
    if (r < 7)
      return 2; // 10% chance BUSH_1
    if (r < 8)
      return 3; // 10% chance BUSH_2
    return 4;   // 20% chance ROCK_1
  };

  const int props_per_grove_min = 15; // Increased from 8
  const int props_per_grove_max = 45; // Increased from 26
  const float grove_radius = 25.0f; // Slightly wider spread for larger clusters
  const float spawn_area_extent = 460.0f;

  // Baseline scatter: tighter grid, more props
  const float grid_step = 35.0f; // Decreased from 55.0f (more grid cells)
  const int grid_props_min = 3;  // Increased from 1
  const int grid_props_max = 7;  // Increased from 4

  for (float gx = -spawn_area_extent; gx <= spawn_area_extent;
       gx += grid_step) {
    for (float gz = -spawn_area_extent; gz <= spawn_area_extent;
         gz += grid_step) {
      int props_in_cell = Random::randInt(grid_props_min, grid_props_max);
      for (int i = 0; i < props_in_cell; ++i) {
        float x = gx + Random::randFloat(-grid_step * 0.45f, grid_step * 0.45f);
        float z = gz + Random::randFloat(-grid_step * 0.45f, grid_step * 0.45f);

        switch (get_random_prop_type()) { // Using weighted random
        case 0:
          props.push_back({ModelName::TREE_1, glm::vec3(x, 0.0f, z),
                           glm::vec3(0.0f, Random::randFloat() * 360.0f, 0.0f),
                           randomize_scale(tree_1_scale)});
          break;
        case 1:
          props.push_back({ModelName::TREE_2, glm::vec3(x, 0.0f, z),
                           glm::vec3(0.0f, Random::randFloat() * 360.0f, 0.0f),
                           randomize_scale(tree_2_scale)});
          break;
        case 2:
          props.push_back({ModelName::BUSH_1, glm::vec3(x, 0.0f, z),
                           glm::vec3(0.0f, Random::randFloat() * 360.0f, 0.0f),
                           randomize_scale(bush_1_scale)});
          break;
        case 3:
          props.push_back({ModelName::BUSH_2, glm::vec3(x, 0.0f, z),
                           glm::vec3(0.0f, Random::randFloat() * 360.0f, 0.0f),
                           randomize_scale(bush_2_scale)});
          break;
        case 4:
          props.push_back({ModelName::ROCK_1, glm::vec3(x, 0.0f, z),
                           glm::vec3(0.0f, Random::randFloat() * 360.0f, 0.0f),
                           randomize_scale(rock_1_scale, 0.85f, 1.25f)});
          break;
        }
      }
    }
  }

  // Add groves: massively increased count for a forest feel
  const int grove_count = 350; // Increased from 140

  for (int g = 0; g < grove_count; ++g) {
    float center_x = Random::randFloat(-spawn_area_extent, spawn_area_extent);
    float center_z = Random::randFloat(-spawn_area_extent, spawn_area_extent);

    int props_in_grove =
        Random::randInt(props_per_grove_min, props_per_grove_max);

    for (int i = 0; i < props_in_grove; ++i) {
      float r = Random::randFloat(0.0f, 1.0f);
      r = r * r * grove_radius;

      float angle = Random::randFloat(0.0f, 2.0f * glm::pi<float>());

      float x = center_x + r * glm::cos(angle);
      float z = center_z + r * glm::sin(angle);

      switch (get_random_prop_type()) { // Using weighted random
      case 0:
        props.push_back({ModelName::TREE_1, glm::vec3(x, 0.0f, z),
                         glm::vec3(0.0f, Random::randFloat() * 360.0f, 0.0f),
                         randomize_scale(tree_1_scale)});
        break;
      case 1:
        props.push_back({ModelName::TREE_2, glm::vec3(x, 0.0f, z),
                         glm::vec3(0.0f, Random::randFloat() * 360.0f, 0.0f),
                         randomize_scale(tree_2_scale)});
        break;
      case 2:
        props.push_back({ModelName::BUSH_1, glm::vec3(x, 0.0f, z),
                         glm::vec3(0.0f, Random::randFloat() * 360.0f, 0.0f),
                         randomize_scale(bush_1_scale)});
        break;
      case 3:
        props.push_back({ModelName::BUSH_2, glm::vec3(x, 0.0f, z),
                         glm::vec3(0.0f, Random::randFloat() * 360.0f, 0.0f),
                         randomize_scale(bush_2_scale)});
        break;
      case 4:
        props.push_back({ModelName::ROCK_1, glm::vec3(x, 0.0f, z),
                         glm::vec3(0.0f, Random::randFloat() * 360.0f, 0.0f),
                         randomize_scale(rock_1_scale, 0.85f, 1.25f)});
        break;
      }
    }
  }

  return props;
}

void MapPopulator::_spawnProp(GameObjectManager &objects,
                              MapManager &map_manager,
                              const PropInstance &prop_instance) {
  auto model = ModelManager::copy(prop_instance.modelName);
  if (!model) {
    return;
  }

  auto [prop, handle] = objects.emplaceWithHandle<StaticProp>(
      model, prop_instance.position, prop_instance.scale,
      prop_instance.rotation);

  const glm::vec3 snapped_position = map_manager.snapToGroundNoCache(
      prop.getPosition(), prop.getPosition().y - prop.getWorldAABB().min.y);
  prop.setPosition(snapped_position);

  map_manager.registerObject(handle, prop.getPosition(), true);
}
