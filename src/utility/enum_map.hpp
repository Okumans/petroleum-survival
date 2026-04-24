#pragma once

#include "external/magic_enum.hpp"
#include <algorithm>
#include <array>
#include <format>
#include <initializer_list>
#include <ranges>
#include <stdexcept>
#include <utility>

/**
 * @brief A statically-sized map that uses an enumeration as its key.
 * * Backed by a `std::array` for $O(1)$ lookup time and cache locality. By
 * default, the size is determined automatically using
 * `magic_enum::enum_count<Key>()`. The enumeration values must be castable to
 * `size_t` and fall within the range
 * $[0, Size - 1]$.
 * * @tparam Key The enumeration type used as the map's key.
 * @tparam Value The type of the elements stored in the map.
 * @tparam Size The number of elements in the map (defaults to the number of
 * enum values).
 */
template <typename Key, typename Value,
          size_t Size = magic_enum::enum_count<Key>()>
class EnumMap {
private:
  std::array<Value, Size> _data;

  /**
   * @brief Converts an enumeration key to its corresponding underlying size_t
   * index.
   */
  static constexpr size_t to_idx(Key key) { return static_cast<size_t>(key); }

  /**
   * @brief Helper constructor to unpack a C-style array into the internal
   * std::array.
   */
  template <size_t... Is>
  constexpr EnumMap(Value (&&values)[Size], std::index_sequence<Is...>)
      : _data{std::move(values[Is])...} {}

public:
  /**
   * @brief Default constructor. Elements are default-initialized.
   */
  constexpr EnumMap() = default;

  /**
   * @brief Constructs an EnumMap from an array of rvalue values.
   * @param values A C-style array of size `Size` containing the values.
   */
  constexpr EnumMap(Value (&&values)[Size])
      : EnumMap(std::move(values), std::make_index_sequence<Size>{}) {}

  /**
   * @brief Constructs an EnumMap from an initializer list of key-value pairs.
   * * Elements not explicitly provided in the initializer list will be
   * default-initialized.
   * * @param list A brace-enclosed list of std::pair<Key, Value>.
   */
  constexpr EnumMap(std::initializer_list<std::pair<Key, Value>> list)
      : _data{} {
    for (auto &item :
         const_cast<std::initializer_list<std::pair<Key, Value>> &>(list)) {
      _data[to_idx(item.first)] = std::move(item.second);
    }
  }

  /**
   * @brief Returns a reference to the mapped value of the specified key.
   * Does not perform bounds checking.
   * @param key The enum key.
   * @return Value& Reference to the underlying value.
   */
  constexpr Value &at(Key key) { return _data[to_idx(key)]; }

  /**
   * @brief Returns a const reference to the mapped value of the specified key.
   * Does not perform bounds checking.
   * @param key The enum key.
   * @return const Value& Const reference to the underlying value.
   */
  constexpr const Value &at(Key key) const { return _data[to_idx(key)]; }

  /**
   * @brief Accesses the mapped value of the specified key via the subscript
   * operator.
   * @param key The enum key.
   * @return Value& Reference to the underlying value.
   */
  constexpr Value &operator[](Key key) { return _data[to_idx(key)]; }

  /**
   * @brief Accesses the mapped value of the specified key via the subscript
   * operator.
   * @param key The enum key.
   * @return const Value& Const reference to the underlying value.
   */
  constexpr const Value &operator[](Key key) const {
    return _data[to_idx(key)];
  }

  /**
   * @brief Returns a reference to the mapped value, with strict bounds
   * checking.
   * * @param key The enum key.
   * @throws std::runtime_error if the evaluated index is strictly $\ge$ Size.
   * @return Value& Reference to the underlying value.
   */
  constexpr Value &get_checked(Key key) {
    if (to_idx(key) >= Size) {
      throw std::runtime_error(
          std::format("Key out of bounds: {}", static_cast<int>(key)));
    }
    return _data[to_idx(key)];
  }

  /**
   * @brief Returns the total capacity/size of the EnumMap.
   */
  constexpr size_t size() const { return Size; }

  // Iterators for traversing the stored values
  constexpr auto begin() noexcept { return _data.begin(); }
  constexpr auto end() noexcept { return _data.end(); }
  constexpr auto begin() const noexcept { return _data.begin(); }
  constexpr auto end() const noexcept { return _data.end(); }

  /**
   * @brief Returns a lazily evaluated view of keys.
   * * @return A std::ranges view yielding keys.
   */
  constexpr auto keys() const {
    return std::views::iota(size_t(0), Size) |
           std::views::transform([](size_t i) { return static_cast<Key>(i); });
  }

  /**
   * @brief Returns a lazily evaluated view of key-value pairs.
   * * Allows iterating over the map similarly to `std::map`, yielding
   * `std::pair<Key, const Value &>` for each element.
   * * @return A std::ranges view yielding key-value pairs.
   */
  constexpr auto pairs() const {
    return std::views::iota(size_t(0), Size) |
           std::views::transform([this](size_t i) {
             return std::pair<Key, const Value &>(static_cast<Key>(i),
                                                  _data[i]);
           });
  }
};

/**
 * @brief A validator struct to ensure all elements within an EnumMap are
 * "truthy".
 * * Used for asserting that every value in the map evaluates to `true` (e.g.,
 * ensuring no pointers are null, or no std::optional instances are empty). The
 * element type must be explicitly convertible to `bool`.
 * * @tparam T The type of the EnumMap or similar container being validated.
 */
template <typename T> struct EnumMapValidator {
  /**
   * @brief Evaluates whether all items in the container are true.
   * @param ref The container reference to evaluate.
   * @return true if all elements are truthy, false otherwise.
   */
  constexpr bool operator()(const T &ref) const {
    return std::ranges::all_of(ref, [](const auto &value) {
      static_assert(std::is_constructible_v<bool, decltype(value)>,
                    "EnumMap elements must be convertible to bool!");

      return static_cast<bool>(value);
    });
  }
};
