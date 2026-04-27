#pragma once

#include "scene/game_object.hpp"

#include <concepts>
#include <functional>
#include <utility>

template <typename T>
  requires std::derived_from<T, GameObject> && std::copyable<T>
class GameObjectFactory {
private:
  T m_prototype;

public:
  GameObjectFactory(const GameObjectFactory &) = delete;
  GameObjectFactory &operator=(const GameObjectFactory &) = delete;
  GameObjectFactory(GameObjectFactory &&) = default;
  GameObjectFactory &operator=(GameObjectFactory &&) = default;

  explicit GameObjectFactory(T &&object_template)
      : m_prototype(std::move(object_template)) {}

  template <typename... Args>
    requires(sizeof...(Args) != 1 ||
             !(std::invocable<Args> && ...)) [[nodiscard]] static auto
  create_factory(Args &&...args) {
    return GameObjectFactory(T(std::forward<Args>(args)...));
  }

  template <typename F>
    requires std::invocable<F> &&
             std::convertible_to<std::invoke_result_t<F>, T>
  [[nodiscard]] static auto create_factory(F &&inv) {
    return GameObjectFactory(std::invoke(std::forward<F>(inv)));
  }

  [[nodiscard]] T create() const { return m_prototype; }

  [[nodiscard]] T create(std::invocable<T &> auto &&modifier) const {
    T instance = m_prototype;
    modifier(instance);
    return instance;
  }
};
