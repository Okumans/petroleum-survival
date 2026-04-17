#pragma once

// Assuming these will match your project structure when implemented
// #include "graphics/animdata.hpp"
#include "graphics/bone.hpp"
#include "graphics/model.hpp"

#include <assimp/scene.h>
#include <cstdint>
#include <glm/glm.hpp>

#include <map>
#include <string>
#include <vector>

// Forward declarations
class Bone;
struct BoneInfo;

struct AssimpNodeData {
  glm::mat4 transformation;
  std::string name;
  uint32_t childrenCount;
  std::vector<AssimpNodeData> children;
};

class Animation {
private:
  float m_duration;
  int m_ticksPerSecond;
  std::vector<Bone> m_bones;
  AssimpNodeData m_rootNode;
  std::map<std::string, BoneInfo> m_boneInfoMap;

public:
  Animation() = default;
  Animation(const std::string &animation_path, Model *model);
  ~Animation();

  Bone *findBone(const std::string &name);

  float getTicksPerSecond() const { return m_ticksPerSecond; }
  float getDuration() const { return m_duration; }
  const AssimpNodeData &getRootNode() const { return m_rootNode; }
  const std::map<std::string, BoneInfo> &getBoneIDMap() const {
    return m_boneInfoMap;
  }

private:
  void _readMissingBones(const aiAnimation *animation, Model &model);
  void _readHierarchyData(AssimpNodeData &dest, const aiNode *src);
};
