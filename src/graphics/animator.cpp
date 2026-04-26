#include "animator.hpp"

#include "glm/fwd.hpp"
#include "graphics/animation.hpp"
#include "graphics/animation_data.hpp"
#include "graphics/bone.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/common.hpp>
#include <string>

Animator::Animator(Animation *animation)
    : m_finalBoneMatrices(200, glm::mat4(1.0f)),
      m_fromBoneMatrices(200, glm::mat4(1.0f)),
      m_toBoneMatrices(200, glm::mat4(1.0f)), m_currentAnimation(animation),
      m_previousAnimation(nullptr), m_currentTime(0.0f), m_previousTime(0.0f),
      m_deltaTime(0.0f), m_blendDuration(0.15f), m_blendTime(0.0f),
      m_isBlending(false) {}

float Animator::_advanceTime(Animation *animation, float current_time,
                             float delta_time) {
  if (animation == nullptr) {
    return current_time;
  }

  float duration = animation->getDuration();
  if (duration <= 0.0f) {
    return 0.0f;
  }

  current_time += animation->getTicksPerSecond() * delta_time;
  return std::fmod(current_time, duration);
}

void Animator::_calculatePose(Animation *animation, float animation_time,
                              std::vector<glm::mat4> &out_matrices) {
  std::fill(out_matrices.begin(), out_matrices.end(), glm::mat4(1.0f));

  if (animation == nullptr) {
    return;
  }

  _calculateBoneTransform(&animation->getRootNode(), glm::mat4(1.0f), animation,
                          animation_time, out_matrices);
}

void Animator::updateAnimation(float delta_time) {
  m_deltaTime = delta_time;

  if (m_currentAnimation == nullptr) {
    return;
  }

  if (m_isBlending && m_previousAnimation != nullptr) {
    m_blendTime += delta_time;

    m_previousTime =
        _advanceTime(m_previousAnimation, m_previousTime, delta_time);
    m_currentTime = _advanceTime(m_currentAnimation, m_currentTime, delta_time);

    _calculatePose(m_previousAnimation, m_previousTime, m_fromBoneMatrices);
    _calculatePose(m_currentAnimation, m_currentTime, m_toBoneMatrices);

    float alpha =
        (m_blendDuration <= 0.0f) ? 1.0f : (m_blendTime / m_blendDuration);
    alpha = std::clamp(alpha, 0.0f, 1.0f);

    for (size_t i = 0; i < m_finalBoneMatrices.size(); ++i) {
      m_finalBoneMatrices[i] =
          m_fromBoneMatrices[i] * (1.0f - alpha) + m_toBoneMatrices[i] * alpha;
    }

    if (alpha >= 1.0f) {
      m_isBlending = false;
      m_previousAnimation = nullptr;
      m_previousTime = 0.0f;
    }

    return;
  }

  m_currentTime = _advanceTime(m_currentAnimation, m_currentTime, delta_time);
  _calculatePose(m_currentAnimation, m_currentTime, m_finalBoneMatrices);
}

void Animator::playAnimation(Animation *p_animation, float blend_duration) {
  if (p_animation == nullptr) {
    return;
  }

  if (m_currentAnimation == nullptr) {
    m_currentAnimation = p_animation;
    m_currentTime = 0.0f;
    m_isBlending = false;
    _calculatePose(m_currentAnimation, m_currentTime, m_finalBoneMatrices);
    return;
  }

  if (p_animation == m_currentAnimation) {
    return;
  }

  m_previousAnimation = m_currentAnimation;
  m_previousTime = m_currentTime;

  m_currentAnimation = p_animation;
  m_currentTime = 0.0f;

  m_blendDuration = blend_duration;
  m_blendTime = 0.0f;
  m_isBlending = true;
}

void Animator::apply(Shader &shader) {
  const auto &matrices = getFinalBoneMatrices();
  for (size_t idx = 0; idx < matrices.size(); ++idx) {
    shader.setMat4("finalBonesMatrices[" + std::to_string(idx) + "]",
                   matrices[idx]);
  }
}

void Animator::_calculateBoneTransform(const AssimpNodeData *node,
                                       glm::mat4 parent_transform,
                                       Animation *animation,
                                       float animation_time,
                                       std::vector<glm::mat4> &out_matrices) {
  Bone::BoneNameHash node_name(node->name);
  glm::mat4 node_transform = node->transformation;

  Bone *bone = animation->findBone(node_name);

  if (bone) {
    bone->update(animation_time);
    node_transform = bone->getLocalTransform();
  }

  glm::mat4 global_transformation = parent_transform * node_transform;

  const std::map<Bone::BoneNameHash, BoneInfo> &bone_info_map =
      animation->getBoneIDMap();

  auto bone_iterator = bone_info_map.find(node_name);

  if (bone_iterator != bone_info_map.end()) {
    uint32_t index = bone_iterator->second.id;
    glm::mat4 offset = bone_iterator->second.offset;

    if (index < out_matrices.size()) {
      out_matrices[index] = global_transformation * offset;
    }
  }

  for (size_t i = 0; i < node->childrenCount; i++) {
    _calculateBoneTransform(&node->children[i], global_transformation,
                            animation, animation_time, out_matrices);
  }
}
