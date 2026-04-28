#pragma once

#include "game/map_manager.hpp"
#include "resource/model_manager.hpp"
#include "scene/game_object_manager.hpp"

#include <glm/glm.hpp>
#include <vector>

/**
 * @class MapPopulationSystem
 * @brief Manages the procedural and manual placement of static environmental
 * props
 *
 * This system handles:
 * - Defining which props exist and their properties (model, scale)
 * - Manually placed prop instances at specific locations
 * - Procedurally spawning props across the map
 */
class MapPopulationSystem {
public:
  /**
   * @struct PropInstance
   * @brief A single prop placement with position and rotation
   */
  struct PropInstance {
    ModelName modelName;
    glm::vec3 position;
    glm::vec3 rotation = glm::vec3(0.0f); // pitch, yaw, roll in degrees
    glm::vec3 scale;
  };

  MapPopulationSystem() = default;
  ~MapPopulationSystem() = default;

  /**
   * @brief Populate the map with static props
   * @param objects The GameObjectManager to add props to
   * @param mapManager The MapManager for handling chunk registration
   */
  void populateMap(GameObjectManager &objects, MapManager &mapManager);

private:
  /**
   * @brief Define all the prop instances for the map
   * @return Vector of prop instances to spawn
   */
  static std::vector<PropInstance> _getMapProps();

  /**
   * @brief Spawn a single prop instance
   * @param objects The object manager
   * @param mapManager The map manager
   * @param propInstance The prop to spawn
   */
  static void _spawnProp(GameObjectManager &objects, MapManager &mapManager,
                         const PropInstance &propInstance);
};
