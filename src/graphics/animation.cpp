#include "animation.hpp"
#include "assimp/anim.h"
#include "graphics/animation_data.hpp"

#include <assimp/Importer.hpp>
#include <cstdint>
#include <glm/gtc/type_ptr.hpp>

static glm::mat4 mat4FromAssimp(const aiMatrix4x4 &from) {
  glm::mat4 to;
  std::memcpy(glm::value_ptr(to), &from, sizeof(float) * 16);
  return glm::transpose(to);
}

Animation::Animation(const std::string &animation_path, Model *model) {
  Assimp::Importer importer;
  const aiScene *scene =
      importer.ReadFile(animation_path, aiProcess_Triangulate);

  assert(scene && scene->mRootNode);

  aiAnimation *animation = scene->mAnimations[0];
  m_duration = animation->mDuration;
  m_ticksPerSecond = animation->mTicksPerSecond;

  aiMatrix4x4 global_transformation = scene->mRootNode->mTransformation;
  m_globalTransformation = mat4FromAssimp(global_transformation.Inverse());

  _readHierarchyData(m_rootNode, scene->mRootNode);
  _readMissingBones(animation, *model);
}

Animation::~Animation() {}

Bone *Animation::findBone(const std::string &name) {
  auto iter = std::ranges::find_if(
      m_bones, [&](const Bone &bone) { return bone.getBoneName() == name; });

  if (iter == m_bones.end()) {
    return nullptr;
  }

  return &(*iter);
}

void Animation::_readMissingBones(const aiAnimation *animation, Model &model) {
  size_t size = animation->mNumChannels;

  std::map<std::string, BoneInfo> &bone_info_map = model.getBoneInfoMap();
  uint32_t &bone_count = model.getBoneCount();

  for (size_t i = 0; i < size; ++i) {
    auto channel = animation->mChannels[i];
    std::string bone_name = channel->mNodeName.data;

    if (!bone_info_map.contains(bone_name)) {
      bone_info_map[bone_name].id = bone_count;
      bone_count++;
    }

    m_bones.push_back(Bone(channel->mNodeName.data,
                           bone_info_map[channel->mNodeName.data].id, channel));
  }

  m_boneInfoMap = bone_info_map;
}

void Animation::_readHierarchyData(AssimpNodeData &dest, const aiNode *src) {
  assert(src);

  dest.name = src->mName.data;
  dest.transformation = mat4FromAssimp(src->mTransformation);
  dest.childrenCount = src->mNumChildren;

  for (size_t i = 0; i < src->mNumChildren; ++i) {
    AssimpNodeData newData;
    _readHierarchyData(newData, src->mChildren[i]);
    dest.children.push_back(newData);
  }
}
