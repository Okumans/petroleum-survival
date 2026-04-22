#pragma once

#include "graphics/shader.hpp"
#include <glm/glm.hpp>

#include <vector>

class Animation;
struct AssimpNodeData;

class Animator {
private:
  std::vector<glm::mat4> m_finalBoneMatrices;
  Animation *m_currentAnimation;
  float m_currentTime;
  float m_deltaTime;

public:
  Animator(Animation *animation);
  void updateAnimation(float delta_time);
  void playAnimation(Animation *p_animation);
  void apply(Shader &shader);
  [[nodiscard]] const std::vector<glm::mat4> &getFinalBoneMatrices() const {
    return m_finalBoneMatrices;
  }

private:
  void _calculateBoneTransform(const AssimpNodeData *node,
                               glm::mat4 parent_transform);
};
