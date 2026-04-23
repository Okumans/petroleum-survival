#pragma once

#include "scene/game_object.hpp"

#include <algorithm>
#include <memory>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

class GameObjectManager {
private:
  std::vector<std::unique_ptr<GameObject>> m_objects;

public:
  GameObjectManager() = default;

  template <typename ObjectType, typename... Args>
    requires std::is_base_of_v<GameObject, ObjectType>
  ObjectType &emplace(Args &&...args) {
    auto object = std::make_unique<ObjectType>(std::forward<Args>(args)...);
    ObjectType &object_ref = *object;
    m_objects.emplace_back(std::move(object));
    return object_ref;
  }

  void update(double delta_time) {
    for (auto &object : m_objects) {
      if (object) {
        object->update(delta_time);
      }
    }
  }

  void draw(const RenderContext &ctx) {
    for (auto &object : m_objects) {
      if (object) {
        object->draw(ctx);
      }
    }
  }

  void collectGarbage() {
    std::erase_if(m_objects, [](const std::unique_ptr<GameObject> &object) {
      return !object || object->isRemovalRequested();
    });
  }

  [[nodiscard]] auto getObjects() {
    return m_objects | std::ranges::views::transform(
                           [](std::unique_ptr<GameObject> &object) {
                             return object.get();
                           });
  }

  void clear() { m_objects.clear(); }
};
