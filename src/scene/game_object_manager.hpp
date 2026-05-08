#pragma once

#include "scene/game_object.hpp"
#include "scene/game_object_factory.hpp"
#include "utility/enum_map.hpp"

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <memory>
#include <optional>
#include <ranges>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

class GameObjectManager {
private:
  EnumMap<GameObjectType, std::vector<std::unique_ptr<GameObject>>> m_objects;

  uint32_t m_nextId = 1;
  std::vector<uint32_t> m_freeIds;
  std::unordered_map<uint32_t, uint32_t> m_idGenerations;
  std::unordered_map<uint32_t, GameObject *> m_idToObject;
  std::unordered_map<const GameObject *, ObjectHandle> m_objectToHandle;

  ObjectHandle _acquireHandle() {
    if (!m_freeIds.empty()) {
      uint32_t id = m_freeIds.back();
      m_freeIds.pop_back();

      uint32_t next_generation = m_idGenerations[id] + 1;
      m_idGenerations[id] = next_generation;
      return ObjectHandle{.id = id, .generation = next_generation};
    }

    uint32_t id = m_nextId++;
    m_idGenerations[id] = 1;
    return ObjectHandle{.id = id, .generation = 1};
  }

  void _releaseHandle(const ObjectHandle &handle) {
    if (!handle.isValid()) {
      return;
    }

    m_idToObject.erase(handle.id);
    m_freeIds.push_back(handle.id);
  }

  void _registerObjectHandle(GameObject &object, const ObjectHandle &handle) {
    m_idToObject[handle.id] = &object;
    m_objectToHandle[&object] = handle;
  }

  void _unregisterObjectHandle(GameObject &object) {
    auto handle_it = m_objectToHandle.find(&object);
    if (handle_it == m_objectToHandle.end()) {
      return;
    }

    ObjectHandle handle = handle_it->second;
    m_objectToHandle.erase(handle_it);
    _releaseHandle(handle);
  }

public:
  GameObjectManager() = default;

  template <typename ObjectType, typename... Args>
    requires std::is_base_of_v<GameObject, ObjectType>
  std::pair<ObjectType &, ObjectHandle> emplaceWithHandle(Args &&...args) {
    auto object = std::make_unique<ObjectType>(std::forward<Args>(args)...);

    ObjectType &object_ref = *object;
    GameObjectType object_type = object_ref.getObjectType();
    ObjectHandle handle = _acquireHandle();

    _registerObjectHandle(object_ref, handle);
    m_objects[object_type].emplace_back(std::move(object));

    return {object_ref, handle};
  }

  template <typename ObjectType, typename... Args>
    requires std::is_base_of_v<GameObject, ObjectType>
  ObjectType &emplace(Args &&...args) {
    auto [object_ref, handle] =
        emplaceWithHandle<ObjectType>(std::forward<Args>(args)...);
    (void)handle;
    return object_ref;
  }

  template <typename ObjectType>
    requires std::is_base_of_v<GameObject, ObjectType>
  std::pair<ObjectType &, ObjectHandle>
  createWithHandle(const GameObjectFactory<ObjectType> &factory) {
    return emplaceWithHandle<ObjectType>(factory.create());
  }

  template <typename ObjectType>
    requires std::is_base_of_v<GameObject, ObjectType>
  std::pair<ObjectType &, ObjectHandle>
  createWithHandle(const GameObjectFactory<ObjectType> &factory,
                   std::invocable<ObjectType &> auto &&modifier) {
    return emplaceWithHandle<ObjectType>(factory.create(modifier));
  }

  template <typename ObjectType>
    requires std::is_base_of_v<GameObject, ObjectType>
  ObjectType &create(const GameObjectFactory<ObjectType> &factory) {
    auto [object_ref, handle] = createWithHandle<ObjectType>(factory);
    (void)handle;
    return object_ref;
  }

  template <typename ObjectType>
    requires std::is_base_of_v<GameObject, ObjectType>
  ObjectType &create(const GameObjectFactory<ObjectType> &factory,
                     std::invocable<ObjectType &> auto &&modifier) {
    auto [object_ref, handle] = createWithHandle<ObjectType>(factory, modifier);
    (void)handle;
    return object_ref;
  }

  [[nodiscard]] GameObject *get(const ObjectHandle &handle) {
    if (!handle.isValid()) {
      return nullptr;
    }

    auto id_it = m_idToObject.find(handle.id);
    if (id_it == m_idToObject.end()) {
      return nullptr;
    }

    auto generation_it = m_idGenerations.find(handle.id);
    if (generation_it == m_idGenerations.end()) {
      return nullptr;
    }

    if (generation_it->second != handle.generation) {
      return nullptr;
    }

    return id_it->second;
  }

  [[nodiscard]] const GameObject *get(const ObjectHandle &handle) const {
    return const_cast<GameObjectManager *>(this)->get(handle);
  }

  [[nodiscard]] bool isAlive(const ObjectHandle &handle) const {
    return get(handle) != nullptr;
  }

  [[nodiscard]] std::optional<ObjectHandle>
  getHandle(const GameObject &object) const {
    auto handle_it = m_objectToHandle.find(&object);
    if (handle_it == m_objectToHandle.end()) {
      return std::nullopt;
    }

    return handle_it->second;
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

  void updateWhere(double delta_time,
                   std::predicate<GameObject &> auto &&predicate) {
    for (std::vector<std::unique_ptr<GameObject>> &objects : m_objects) {
      for (std::unique_ptr<GameObject> &object : objects) {
        if (!object) {
          continue;
        }

        if (predicate(*object)) {
          object->update(delta_time);
        }
      }
    }
  }

  void updateWithTypeWhere(double delta_time, GameObjectType type,
                           std::predicate<GameObject &> auto &&predicate) {
    for (std::unique_ptr<GameObject> &object : m_objects[type]) {
      if (!object) {
        continue;
      }

      if (predicate(*object)) {
        object->update(delta_time);
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

  void drawWhere(const RenderContext &ctx,
                 std::predicate<GameObject &> auto &&predicate) {
    for (std::vector<std::unique_ptr<GameObject>> &objects : m_objects) {
      for (std::unique_ptr<GameObject> &object : objects) {
        if (!object) {
          continue;
        }

        if (predicate(*object)) {
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
                    [this](const std::unique_ptr<GameObject> &object) {
                      if (!object) {
                        return true;
                      }

                      if (!object->isRemovalRequested()) {
                        return false;
                      }

                      _unregisterObjectHandle(*object);
                      return true;
                    });
    }
  }

  [[nodiscard]] auto getObjects() {
    return m_objects | std::views::join |
           std::views::filter([](std::unique_ptr<GameObject> &object) {
             return object != nullptr;
           }) |
           std::views::transform([](std::unique_ptr<GameObject> &object) {
             return object.get();
           });
  }

  [[nodiscard]] auto getObjectsWithType(GameObjectType type) {
    return m_objects[type] |
           std::views::filter([](std::unique_ptr<GameObject> &object) {
             return object != nullptr;
           }) |
           std::views::transform([](std::unique_ptr<GameObject> &object) {
             return object.get();
           });
  }

  void clear() {
    for (std::vector<std::unique_ptr<GameObject>> &objects : m_objects) {
      for (std::unique_ptr<GameObject> &object : objects) {
        if (object) {
          _unregisterObjectHandle(*object);
        }
      }

      objects.clear();
    }
  }

  void clearWithType(GameObjectType type) {
    for (std::unique_ptr<GameObject> &object : m_objects[type]) {
      if (object) {
        _unregisterObjectHandle(*object);
      }
    }

    m_objects[type].clear();
  }
};
