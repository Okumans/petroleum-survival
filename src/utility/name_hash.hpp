#pragma once

#include <cstdint>
#include <string_view>

namespace {
inline static constexpr uint32_t _fnv1a(std::string_view str) {
  uint32_t hash = 0x811c9dc5;
  for (char c : str) {
    hash ^= static_cast<uint8_t>(c);
    hash *= 0x01000193;
  }
  return hash;
}
} // namespace

struct NameHash {
  uint32_t hash;

  constexpr NameHash() = default;
  constexpr NameHash(NameHash &&) = default;
  constexpr NameHash &operator=(NameHash &&) = default;
  constexpr NameHash(const NameHash &) = default;
  constexpr NameHash(std::string_view name) : hash(_fnv1a(name)) {}
  constexpr NameHash(const char *name) : hash(_fnv1a(name)) {}
  constexpr explicit operator uint32_t() const { return hash; }
  constexpr explicit operator int() const { return static_cast<int>(hash); }
  constexpr auto operator<=>(const NameHash &) const = default;
};

namespace std {
template <> struct hash<NameHash> {
  constexpr std::size_t operator()(const NameHash &h) const { return h.hash; }
};
} // namespace std
