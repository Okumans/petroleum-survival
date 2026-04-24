#pragma once

#include "scene/game_object.hpp"
#include "utility/enum_map.hpp"

#include <algorithm>
#include <memory>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

class GameObjectManager {
private:
  EnumMap<GameObjectType, std::vector<std::unique_ptr<GameObject>>> m_objects;

public:
  GameObjectManager() = default;

  template <typename ObjectType, typename... Args>
    requires std::is_base_of_v<GameObject, ObjectType>
  ObjectType &emplace(Args &&...args) {
    auto object = std::make_unique<ObjectType>(std::forward<Args>(args)...);

    ObjectType &object_ref = *object;
    GameObjectType object_type = object_ref.getObjectType();

    m_objects[object_type].emplace_back(std::move(object));

    return object_ref;
  }

  void update(double delta_time) {
    for (std::vector<std::unique_ptr<GameObject>> &objects : m_objects) {
      for (std::unique_ptr<GameObject> &object : objects) {
        if (object) {
          object->update(delta_time);
        }
      }
    }
  }

  void updateWithType(GameObjectType type, double delta_time) {
    for (std::unique_ptr<GameObject> &object : m_objects[type]) {
      if (object) {
        object->update(delta_time);
      }
    }
  }

  void draw(const RenderContext &ctx) {
    for (std::vector<std::unique_ptr<GameObject>> &objects : m_objects) {
      for (std::unique_ptr<GameObject> &object : objects) {
        if (object) {
          object->draw(ctx);
        }
      }
    }
  }

  void drawWithType(GameObjectType type, const RenderContext &ctx) {
    for (std::unique_ptr<GameObject> &object : m_objects[type]) {
      if (object) {
        object->draw(ctx);
      }
    }
  }

  void collectGarbage() {
    for (GameObjectType key : m_objects.keys()) {
      std::erase_if(m_objects[key],
                    [](const std::unique_ptr<GameObject> &object) {
                      return !object || object->isRemovalRequested();
                    });
    }
  }

  [[nodiscard]] auto getObjects() {
    return m_objects | std::ranges::views::join |
           std::ranges::views::transform(
               [](std::unique_ptr<GameObject> &object) {
                 return object.get();
               });
  }

  [[nodiscard]] auto getObjectsWithType(GameObjectType type) {
    return m_objects[type] | std::ranges::views::transform(
                                 [](std::unique_ptr<GameObject> &object) {
                                   return object.get();
                                 });
  }

  void clear() {
    for (std::vector<std::unique_ptr<GameObject>> &objects : m_objects) {
      objects.clear();
    }
  }

  void clearWithType(GameObjectType type) { m_objects[type].clear(); }
};
