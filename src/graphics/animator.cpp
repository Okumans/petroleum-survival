#include "animator.hpp"

#include "glm/fwd.hpp"
#include "graphics/animation.hpp"
#include "graphics/animation_data.hpp"
#include "graphics/bone.hpp"

#include <cmath>
#include <cstdint>
#include <format>
#include <ranges>

Animator::Animator(Animation *animation)
    : m_finalBoneMatrices(100, glm::mat4(1.0f)), m_currentAnimation(animation),
      m_currentTime(0.0f) {}

void Animator::updateAnimation(float delta_time) {
  m_deltaTime = delta_time;

  if (m_currentAnimation) {
    m_currentTime += m_currentAnimation->getTicksPerSecond() * delta_time;
    m_currentTime = std::fmod(m_currentTime, m_currentAnimation->getDuration());

    _calculateBoneTransform(&m_currentAnimation->getRootNode(),
                            glm::mat4(1.0f));
  }
}

void Animator::playAnimation(Animation *p_animation) {
  m_currentAnimation = p_animation;
  m_currentTime = 0.0f;
}

void Animator::apply(Shader &shader) {
  for (const auto &[idx, transform] :
       std::ranges::views::enumerate(getFinalBoneMatrices())) {
    shader.setMat4(std::format("finalBonesMatrices[{}]", idx), transform);
  }
}

void Animator::_calculateBoneTransform(const AssimpNodeData *node,
                                       glm::mat4 parent_transform) {
  std::string node_name = node->name;
  glm::mat4 node_transform = node->transformation;

  Bone *bone = m_currentAnimation->findBone(node_name);

  if (bone) {
    bone->update(m_currentTime);
    node_transform = bone->getLocalTransform();
  }

  glm::mat4 global_transformation = parent_transform * node_transform;

  const std::map<std::string, BoneInfo> &bone_info_map =
      m_currentAnimation->getBoneIDMap();
  auto bone_iterator = bone_info_map.find(node_name);

  if (bone_iterator != bone_info_map.end()) {
    uint32_t index = bone_iterator->second.id;
    glm::mat4 offset = bone_iterator->second.offset;
    if (index < 200) {
      m_finalBoneMatrices[index] = global_transformation * offset;
    }
  }

  for (size_t i = 0; i < node->childrenCount; i++) {
    _calculateBoneTransform(&node->children[i], global_transformation);
  }
}
