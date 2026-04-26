#pragma once

#include "utility/utility.hpp"
#include <assimp/scene.h>
#include <cstdint>
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <cassert>
#include <string>
#include <vector>

struct KeyPosition {
  glm::vec3 position;
  float timeStamp;
};

struct KeyRotation {
  glm::quat orientation;
  float timeStamp;
};

struct KeyScale {
  glm::vec3 scale;
  float timeStamp;
};

class Bone {
public:
  struct BoneNameHash {
    uint32_t hash;

    constexpr BoneNameHash() = default;
    BoneNameHash(BoneNameHash &&) = default;
    BoneNameHash &operator=(BoneNameHash &&) = default;
    constexpr BoneNameHash(const BoneNameHash &) = default;
    constexpr BoneNameHash(std::string_view name) : hash(fnv1a(name)) {}
    constexpr explicit operator uint32_t() const { return hash; }
    constexpr explicit operator int() const { return static_cast<int>(hash); }
    auto operator<=>(const BoneNameHash &) const = default;
  };

private:
  std::vector<KeyPosition> m_positions;
  std::vector<KeyRotation> m_rotations;
  std::vector<KeyScale> m_scales;

  uint32_t m_numPositions;
  uint32_t m_numRotations;
  uint32_t m_numScalings;

  glm::mat4 m_localTransform;
  BoneNameHash m_name;
  uint32_t m_id;

public:
  [[deprecated("Use more efficient BoneNameHash constructor instead")]] Bone(
      const std::string &name, uint32_t id, const aiNodeAnim *channel);
  Bone(BoneNameHash name, uint32_t id, const aiNodeAnim *channel);

  void update(float animation_time);

  [[nodiscard]] glm::mat4 getLocalTransform() const { return m_localTransform; }
  [[nodiscard]] BoneNameHash getBoneName() const { return m_name; }
  [[nodiscard]] uint32_t getBoneID() const { return m_id; }

  [[nodiscard]] uint32_t getPositionIndex(float animation_time) const;
  [[nodiscard]] uint32_t getRotationIndex(float animation_time) const;
  [[nodiscard]] uint32_t getScaleIndex(float animation_time) const;

private:
  [[nodiscard]] float _getScaleFactor(float last_timestamp,
                                      float next_timestamp,
                                      float animation_time) const;
  [[nodiscard]] glm::mat4 _interpolatePosition(float animation_time) const;
  [[nodiscard]] glm::mat4 _interpolateRotation(float animation_time) const;
  [[nodiscard]] glm::mat4 _interpolateScaling(float animation_time) const;
};

namespace std {
template <> struct hash<Bone::BoneNameHash> {
  std::size_t operator()(const Bone::BoneNameHash &h) const { return h.hash; }
};
} // namespace std
