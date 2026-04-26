#include "bone.hpp"
#include <cstdint>
#include <ctime>

namespace {

static glm::vec3 vec3FromAssimp(const aiVector3D &vec) {
  return glm::vec3(vec.x, vec.y, vec.z);
}

static glm::quat quatFromAssimp(const aiQuaternion &quat) {
  return glm::quat(quat.w, quat.x, quat.y, quat.z);
}
} // namespace

Bone::Bone(const std::string &name, uint32_t id, const aiNodeAnim *channel)
    : m_localTransform(1.0f), m_name(name), m_id(id) {
  m_numPositions = channel->mNumPositionKeys;
  for (size_t position_index = 0; position_index < m_numPositions;
       ++position_index) {
    aiVector3D ai_position = channel->mPositionKeys[position_index].mValue;
    float timestamp = channel->mPositionKeys[position_index].mTime;

    KeyPosition data;
    data.position = vec3FromAssimp(ai_position);
    data.timeStamp = timestamp;
    m_positions.push_back(data);
  }

  m_numRotations = channel->mNumRotationKeys;
  for (size_t rotation_index = 0; rotation_index < m_numRotations;
       ++rotation_index) {
    aiQuaternion ai_orientation = channel->mRotationKeys[rotation_index].mValue;
    float timestamp = channel->mRotationKeys[rotation_index].mTime;

    KeyRotation data;
    data.orientation = quatFromAssimp(ai_orientation);
    data.timeStamp = timestamp;
    m_rotations.push_back(data);
  }

  m_numScalings = channel->mNumScalingKeys;
  for (size_t key_index = 0; key_index < m_numScalings; ++key_index) {
    aiVector3D scale = channel->mScalingKeys[key_index].mValue;
    float timestamp = channel->mScalingKeys[key_index].mTime;

    KeyScale data;
    data.scale = vec3FromAssimp(scale);
    data.timeStamp = timestamp;
    m_scales.push_back(data);
  }
}

Bone::Bone(BoneNameHash name, uint32_t id, const aiNodeAnim *channel)
    : m_localTransform(1.0f), m_name(name), m_id(id) {

  m_numPositions = channel->mNumPositionKeys;
  for (size_t position_index = 0; position_index < m_numPositions;
       ++position_index) {

    aiVector3D ai_position = channel->mPositionKeys[position_index].mValue;
    float timestamp = channel->mPositionKeys[position_index].mTime;

    m_positions.push_back(KeyPosition{
        .position = vec3FromAssimp(ai_position),
        .timeStamp = timestamp,
    });
  }

  m_numRotations = channel->mNumRotationKeys;
  for (size_t rotation_index = 0; rotation_index < m_numRotations;
       ++rotation_index) {
    aiQuaternion ai_orientation = channel->mRotationKeys[rotation_index].mValue;
    float timestamp = channel->mRotationKeys[rotation_index].mTime;

    m_rotations.push_back(KeyRotation{
        .orientation = quatFromAssimp(ai_orientation),
        .timeStamp = timestamp,
    });
  }

  m_numScalings = channel->mNumScalingKeys;
  for (size_t key_index = 0; key_index < m_numScalings; ++key_index) {
    aiVector3D scale = channel->mScalingKeys[key_index].mValue;
    float timestamp = channel->mScalingKeys[key_index].mTime;

    m_scales.push_back(KeyScale{
        .scale = vec3FromAssimp(scale),
        .timeStamp = timestamp,
    });
  }
}

void Bone::update(float animation_time) {
  glm::mat4 translation = _interpolatePosition(animation_time);
  glm::mat4 rotation = _interpolateRotation(animation_time);
  glm::mat4 scale = _interpolateScaling(animation_time);
  m_localTransform = translation * rotation * scale;
}

uint32_t Bone::getPositionIndex(float animation_time) const {
  for (size_t index = 0; index < m_numPositions - 1; ++index) {
    if (animation_time < m_positions[index + 1].timeStamp) {
      return index;
    }
  }
  assert(0);
  return 0; // Fallback
}

uint32_t Bone::getRotationIndex(float animation_time) const {
  for (size_t index = 0; index < m_numRotations - 1; ++index) {
    if (animation_time < m_rotations[index + 1].timeStamp) {
      return index;
    }
  }
  assert(0);
  return 0; // Fallback
}

uint32_t Bone::getScaleIndex(float animation_time) const {
  for (size_t index = 0; index < m_numScalings - 1; ++index) {
    if (animation_time < m_scales[index + 1].timeStamp) {
      return index;
    }
  }
  assert(0);
  return 0; // Fallback
}

float Bone::_getScaleFactor(float last_timestamp, float next_timestamp,
                            float animation_time) const {
  float midway_length = animation_time - last_timestamp;
  float frames_diff = next_timestamp - last_timestamp;
  return midway_length / frames_diff;
}

glm::mat4 Bone::_interpolatePosition(float animation_time) const {
  if (1 == m_numPositions) {
    return glm::translate(glm::mat4(1.0f), m_positions[0].position);
  }

  int p0_index = getPositionIndex(animation_time);
  int p1_index = p0_index + 1;
  float scale_factor =
      _getScaleFactor(m_positions[p0_index].timeStamp,
                      m_positions[p1_index].timeStamp, animation_time);
  glm::vec3 final_position =
      glm::mix(m_positions[p0_index].position, m_positions[p1_index].position,
               scale_factor);
  return glm::translate(glm::mat4(1.0f), final_position);
}

glm::mat4 Bone::_interpolateRotation(float animation_time) const {
  if (1 == m_numRotations) {
    auto rotation = glm::normalize(m_rotations[0].orientation);
    return glm::toMat4(rotation);
  }

  int p0_index = getRotationIndex(animation_time);
  int p1_index = p0_index + 1;
  float scale_factor =
      _getScaleFactor(m_rotations[p0_index].timeStamp,
                      m_rotations[p1_index].timeStamp, animation_time);
  glm::quat final_rotation =
      glm::slerp(m_rotations[p0_index].orientation,
                 m_rotations[p1_index].orientation, scale_factor);
  final_rotation = glm::normalize(final_rotation);
  return glm::toMat4(final_rotation);
}

glm::mat4 Bone::_interpolateScaling(float animation_time) const {
  if (1 == m_numScalings) {
    return glm::scale(glm::mat4(1.0f), m_scales[0].scale);
  }

  int p0_index = getScaleIndex(animation_time);
  int p1_index = p0_index + 1;
  float scale_factor =
      _getScaleFactor(m_scales[p0_index].timeStamp,
                      m_scales[p1_index].timeStamp, animation_time);
  glm::vec3 final_scale = glm::mix(m_scales[p0_index].scale,
                                   m_scales[p1_index].scale, scale_factor);
  return glm::scale(glm::mat4(1.0f), final_scale);
}
