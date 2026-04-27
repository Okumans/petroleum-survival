#pragma once

#include <glm/glm.hpp>

#include <vector>

class Animation;
struct AssimpNodeData;

class Animator {
private:
  std::vector<glm::mat4> m_finalBoneMatrices;
  std::vector<glm::mat4> m_fromBoneMatrices;
  std::vector<glm::mat4> m_toBoneMatrices;

  Animation *m_currentAnimation;
  Animation *m_previousAnimation;

  float m_currentTime;
  float m_previousTime;
  float m_deltaTime;

  float m_blendDuration;
  float m_blendTime;
  bool m_isBlending;

  float m_speed = 1.0f;

public:
  Animator(Animation *animation);
  void updateAnimation(float delta_time);
  void playAnimation(Animation *p_animation, float blend_duration = 0.15f);
  void setSpeed(float speed) { m_speed = speed; }
  [[nodiscard]] const std::vector<glm::mat4> &getFinalBoneMatrices() const {
    return m_finalBoneMatrices;
  }

private:
  static float _advanceTime(Animation *animation, float current_time,
                            float delta_time);
  void _calculatePose(Animation *animation, float animation_time,
                      std::vector<glm::mat4> &out_matrices);
  void _calculateBoneTransform(const AssimpNodeData *node,
                               glm::mat4 parent_transform, Animation *animation,
                               float animation_time,
                               std::vector<glm::mat4> &out_matrices);
};
