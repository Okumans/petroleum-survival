#pragma once

#include "comptime_string.hpp"
#include "type_of.hpp"

#include <format>
#include <optional>
#include <stdexcept>

namespace Utility {

/**
 * @brief A wrapper class that mandates explicit initialization before access.
 * * Provides safe access to an underlying object of type T. It prevents
 * undefined behavior by throwing a descriptive exception if the object
 * is accessed before `init()` is called.
 * * @tparam T The type of the object being stored.
 * @tparam Name A compile-time string representing the object's name for
 * debugging.
 */
template <typename T, CompTimeStr Name = "Unknown"> class NotInitialized {
private:
  std::optional<T> storage;

public:
  /**
   * @brief Retrieves a reference to the underlying object.
   * @throws std::runtime_error if the object has not been initialized.
   * @return T& Reference to the initialized object.
   */
  T &ensureInitialized() {
    if (!storage.has_value()) {
      throw std::runtime_error(std::format(
          "Access Violation: The object \"{}\" of type \"{}\" is "
          "uninitialized. "
          "Please ensure init() is called before attempting to access it.",
          Name.data, type_of<T>()));
    }
    return *storage;
  }

  /**
   * @brief Retrieves a const reference to the underlying object.
   * @throws std::runtime_error if the object has not been initialized.
   * @return const T& Const reference to the initialized object.
   */
  const T &ensureInitialized() const {
    if (!storage.has_value()) {
      throw std::runtime_error(std::format(
          "Access Violation: The object \"{}\" of type \"{}\" is "
          "uninitialized. "
          "Please ensure init() is called before attempting to access it.",
          Name.data, type_of<T>()));
    }
    return *storage;
  }

  /**
   * @brief Checks if the object has been initialized.
   * @return true if initialized, false otherwise.
   */
  bool isInitialized() const { return storage.has_value(); }

  /**
   * @brief Constructs the underlying object in-place.
   * @throws std::runtime_error if the object is already initialized.
   * @tparam Args Types of the arguments to pass to T's constructor.
   * @param args Arguments to forward to T's constructor.
   */
  template <typename... Args> void init(Args &&...args) {
    // if (storage.has_value()) {
    //   throw std::runtime_error(std::format(
    //       "Initialization Error: The object \"{}\" is already initialized. "
    //       "Multiple calls to init() are not allowed.",
    //       Name.data));
    // }
    storage.emplace(std::forward<Args>(args)...);
  }
};

/**
 * @brief Default validator that always returns true.
 */
template <typename T> struct DefaultValidator {
  constexpr bool operator()(const T &) const { return true; }
};

/**
 * @brief A lazily-allocated, indexable storage class with optional validation.
 * * Automatically allocates storage upon the first call to `set()`. The object
 * is considered fully initialized only if storage exists AND the custom
 * Validator passes.
 * * @tparam T The underlying indexable container type (e.g., std::map,
 * std::unordered_map).
 * @tparam Name A compile-time string representing the object's name for
 * debugging.
 * @tparam Validator A callable type that verifies if the object state is
 * valid/complete.
 */
template <typename T, CompTimeStr Name = "Unknown",
          typename Validator = DefaultValidator<T>>
class SettableNotInitialized {
private:
  std::optional<T> storage;

public:
  /**
   * @brief Assigns a value to a specific key, allocating storage if necessary.
   * @param key The key/index to set.
   * @param value The value to assign.
   * @return true if the entire object now passes validation, false otherwise.
   */
  template <typename Key, typename Value>
    requires requires(T t, Key k, Value v) { t[k] = std::forward<Value>(v); }
  constexpr bool set(Key &&key, Value &&value) {
    if (!storage.has_value()) {
      storage.emplace();
    }

    (*storage)[std::forward<Key>(key)] = std::forward<Value>(value);

    return Validator{}(*storage);
  }

  /**
   * @brief Bypasses validation to retrieve a value by key.
   * @throws std::runtime_error if storage hasn't been allocated yet.
   * @param key The key to look up.
   * @return decltype(auto) Reference to the stored value.
   */
  template <typename Key>
    requires requires(T t, Key k) { t[k]; }
  constexpr decltype(auto) getUnvalidated(Key &&key) {
    if (storage.has_value()) {
      return (*storage)[std::forward<Key>(key)];
    }

    throw std::runtime_error(std::format(
        "Read Error: Attempted to read from \"{}\" before it was allocated. "
        "Please call set() at least once to initialize the underlying storage.",
        Name.data));
  }

  /**
   * @brief Bypasses validation to retrieve a const value by key.
   * @throws std::runtime_error if storage hasn't been allocated yet.
   * @param key The key to look up.
   * @return decltype(auto) Const reference to the stored value.
   */
  template <typename Key>
    requires requires(const T t, Key k) { t[k]; }
  constexpr decltype(auto) getUnvalidated(Key &&key) const {
    if (storage.has_value()) {
      return (*storage)[std::forward<Key>(key)];
    }

    throw std::runtime_error(std::format(
        "Read Error: Attempted to read from \"{}\" before it was allocated. "
        "Please call set() at least once to initialize the underlying storage.",
        Name.data));
  }

  /**
   * @brief Retrieves a reference to the fully validated object.
   * @throws std::runtime_error if unallocated or validation fails.
   * @return T& Reference to the validated object.
   */
  constexpr T &ensureInitialized() {
    if (!isInitialized()) {
      throw std::runtime_error(
          std::format("Validation Error: Object \"{}\" of type \"{}\" cannot "
                      "be accessed yet. "
                      "It is either currently unallocated or lacks the "
                      "required data to pass validation.",
                      Name.data, type_of<T>()));
    }
    return *storage;
  }

  /**
   * @brief Retrieves a const reference to the fully validated object.
   * @throws std::runtime_error if unallocated or validation fails.
   * @return const T& Const reference to the validated object.
   */
  constexpr const T &ensureInitialized() const {
    if (!isInitialized()) {
      throw std::runtime_error(
          std::format("Validation Error: Object \"{}\" of type \"{}\" cannot "
                      "be accessed yet. "
                      "It is either currently unallocated or lacks the "
                      "required data to pass validation.",
                      Name.data, type_of<T>()));
    }
    return *storage;
  }

  /**
   * @brief Checks if storage is allocated AND passes the custom validator.
   * @return true if fully initialized and valid, false otherwise.
   */
  constexpr bool isInitialized() const {
    return storage.has_value() && Validator{}(*storage);
  }

  /**
   * @brief Clears the underlying storage, returning it to an unallocated state.
   */
  constexpr void clear() { storage.reset(); }
};

} // namespace Utility
