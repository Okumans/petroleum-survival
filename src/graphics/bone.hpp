#pragma once

#include "glm/fwd.hpp"
#include "utility/name_hash.hpp"
#include <assimp/scene.h>
#include <cstdint>
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <cassert>
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
private:
  std::vector<KeyPosition> m_positions;
  std::vector<KeyRotation> m_rotations;
  std::vector<KeyScale> m_scales;

  uint32_t m_numPositions;
  uint32_t m_numRotations;
  uint32_t m_numScalings;

  glm::mat4 m_localTransform;
  NameHash m_name;
  uint32_t m_id;

  mutable uint32_t m_lastPositionIndex = 0;
  mutable uint32_t m_lastRotationIndex = 0;
  mutable uint32_t m_lastScaleIndex = 0;

public:
  Bone(NameHash name, uint32_t id, const aiNodeAnim *channel);

  void update(float animation_time);

  [[nodiscard]] glm::mat4 getLocalTransform() const { return m_localTransform; }
  [[nodiscard]] NameHash getBoneName() const { return m_name; }
  [[nodiscard]] uint32_t getBoneID() const { return m_id; }

  [[nodiscard]] uint32_t getPositionIndex(float animation_time) const;
  [[nodiscard]] uint32_t getRotationIndex(float animation_time) const;
  [[nodiscard]] uint32_t getScaleIndex(float animation_time) const;

private:
  [[nodiscard]] float _getScaleFactor(float last_timestamp,
                                      float next_timestamp,
                                      float animation_time) const;
  [[nodiscard]] glm::vec3 _interpolatePosition(float animation_time) const;
  [[nodiscard]] glm::quat _interpolateRotation(float animation_time) const;
  [[nodiscard]] glm::vec3 _interpolateScaling(float animation_time) const;
};
