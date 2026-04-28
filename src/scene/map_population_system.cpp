#include "scene/map_population_system.hpp"
#include "resource/model_manager.hpp"
#include "scene/static_prop.hpp"
#include "utility/random.hpp"

#include <glm/glm.hpp>

void MapPopulator::populateMap(GameObjectManager &objects,
                               MapManager &mapManager) {
  auto props = _getMapProps();

  for (const auto &prop : props) {
    _spawnProp(objects, mapManager, prop);
  }
}

std::vector<MapPopulator::PropInstance> MapPopulator::_getMapProps() {
  std::vector<PropInstance> props;

  // Define prop scale factors
  constexpr float TREE_1_SCALE = 0.006f * 3;
  constexpr float TREE_2_SCALE = 0.4f * 3;
  constexpr float BUSH_1_SCALE = 0.002f * 3;
  constexpr float BUSH_2_SCALE = 0.0025f * 3;
  constexpr float ROCK_1_SCALE = 0.005f * 3;

  auto randomizeScale = [](float base_scale, float min_mul = 0.9f,
                           float max_mul = 1.15f) {
    return glm::vec3(base_scale * Random::randFloat(min_mul, max_mul));
  };

  const int NUM_GROVES = 12; // How many clusters of props
  const int PROPS_PER_GROVE_MIN = 4;
  const int PROPS_PER_GROVE_MAX = 15;
  const float GROVE_RADIUS = 18.0f; // Max spread of a single cluster
  const float SPAWN_AREA_EXTENT =
      60.0f; // Your world boundaries to spawn within

  for (int g = 0; g < NUM_GROVES; ++g) {
    float center_x = Random::randFloat(-SPAWN_AREA_EXTENT, SPAWN_AREA_EXTENT);
    float center_z = Random::randFloat(-SPAWN_AREA_EXTENT, SPAWN_AREA_EXTENT);

    int props_in_grove =
        Random::randInt(PROPS_PER_GROVE_MIN, PROPS_PER_GROVE_MAX);

    for (int i = 0; i < props_in_grove; ++i) {
      float r = Random::randFloat(0.0f, 1.0f);
      r = r * r * GROVE_RADIUS;

      float angle = Random::randFloat(0.0f, 2.0f * glm::pi<float>());

      float x = center_x + r * glm::cos(angle);
      float z = center_z + r * glm::sin(angle);

      switch (Random::randInt(0, 4)) {
      case 0:
        props.push_back({ModelName::TREE_1, glm::vec3(x, 0.0f, z),
                         glm::vec3(0.0f, Random::randFloat() * 360.0f, 0.0f),
                         randomizeScale(TREE_1_SCALE)});
        break;
      case 1:
        props.push_back({ModelName::TREE_2, glm::vec3(x, 0.0f, z),
                         glm::vec3(0.0f, Random::randFloat() * 360.0f, 0.0f),
                         randomizeScale(TREE_2_SCALE)});
        break;
      case 2:
        props.push_back({ModelName::BUSH_1, glm::vec3(x, 0.0f, z),
                         glm::vec3(0.0f, Random::randFloat() * 360.0f, 0.0f),
                         randomizeScale(BUSH_1_SCALE)});
        break;
      case 3:
        props.push_back({ModelName::BUSH_2, glm::vec3(x, 0.0f, z),
                         glm::vec3(0.0f, Random::randFloat() * 360.0f, 0.0f),
                         randomizeScale(BUSH_2_SCALE)});
        break;
      case 4:
        props.push_back({ModelName::ROCK_1, glm::vec3(x, 0.0f, z),
                         glm::vec3(0.0f, Random::randFloat() * 360.0f, 0.0f),
                         randomizeScale(ROCK_1_SCALE, 0.85f, 1.25f)});
        break;
      }
    }
  }

  return props;
}

void MapPopulator::_spawnProp(GameObjectManager &objects,
                              MapManager &mapManager,
                              const PropInstance &propInstance) {
  auto model = ModelManager::copy(propInstance.modelName);
  if (!model) {
    return;
  }

  auto [prop, handle] = objects.emplaceWithHandle<StaticProp>(
      model, propInstance.position, propInstance.scale, propInstance.rotation);

  const glm::vec3 snapped_position = mapManager.snapToGroundNoCache(
      prop.getPosition(), prop.getPosition().y - prop.getWorldAABB().min.y);
  prop.setPosition(snapped_position);

  mapManager.registerObject(handle, prop.getPosition(), true);
}
