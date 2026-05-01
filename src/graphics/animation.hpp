#pragma once

// Assuming these will match your project structure when implemented
// #include "graphics/animdata.hpp"
#include "graphics/bone.hpp"
#include "graphics/model.hpp"
#include "utility/name_hash.hpp"

#include <assimp/scene.h>
#include <cstdint>
#include <glm/glm.hpp>

#include <map>
#include <string>
#include <vector>

class Bone;
struct BoneInfo;

struct AssimpNodeData {
  glm::mat4 transformation;
  Utility::NameHash name;
  uint32_t childrenCount;
  std::vector<AssimpNodeData> children;

  AssimpNodeData() = default;

  mutable Bone *m_cachedBone = nullptr;
  mutable const BoneInfo *m_cachedBoneInfo = nullptr;
};

class Animation {
private:
  using NameHash = Utility::NameHash;

  glm::mat4 m_globalTransformation{1.0f};
  float m_duration;
  int m_ticksPerSecond;
  std::vector<Bone> m_bones;
  AssimpNodeData m_rootNode;
  std::map<NameHash, BoneInfo> m_boneInfoMap;

public:
  Animation() = default;
  Animation(const std::string &animation_path, Model *model);
  ~Animation();

  Bone *findBone(NameHash name);

  [[nodiscard]] float getTicksPerSecond() const { return m_ticksPerSecond; }
  [[nodiscard]] float getDuration() const { return m_duration; }
  [[nodiscard]] const AssimpNodeData &getRootNode() const { return m_rootNode; }
  [[nodiscard]] const glm::mat4 &getGlobalTransformation() const {
    return m_globalTransformation;
  }
  [[nodiscard]] const std::map<NameHash, BoneInfo> &getBoneIDMap() const {
    return m_boneInfoMap;
  }

private:
  void _readMissingBones(const aiAnimation *animation, Model &model);
  void _readHierarchyData(AssimpNodeData &dest, const aiNode *src);
  void _resolveBones(const AssimpNodeData &node);
};
