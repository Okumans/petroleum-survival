#pragma once

#include "graphics/bone.hpp"
#include "graphics/idrawable.hpp"
#include "mesh.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "graphics/animation_data.hpp"
#include <cstdint>
#include <map>
#include <string>

class Model : public IDrawable {
private:
  // model data
  std::vector<Mesh> m_meshes;
  std::string m_directory;
  std::string m_path;

  std::map<Bone::BoneNameHash, BoneInfo> m_boneInfoMap;
  uint32_t m_boneCount = 0;

public:
  Model(const char *path, bool flip_vertical = false);

  Model(const Model &) = delete;
  Model &operator=(Model &) const = delete;

  Model(Model &&) noexcept;

  void draw(const RenderContext &ctx) override;
  void drawInstanced(const RenderContext &ctx, uint32_t count);
  [[nodiscard]] const std::vector<Mesh> &getMeshes() const { return m_meshes; }

  [[nodiscard]] std::map<Bone::BoneNameHash, BoneInfo> &getBoneInfoMap() {
    return m_boneInfoMap;
  }
  [[nodiscard]] const std::map<Bone::BoneNameHash, BoneInfo> &
  getBoneInfoMap() const {
    return m_boneInfoMap;
  }
  [[nodiscard]] uint32_t &getBoneCount() { return m_boneCount; }
  [[nodiscard]] uint32_t getBoneCount() const { return m_boneCount; }

private:
  void _loadModel(const char *path, bool flip_vertical);
  void _processNode(aiNode *node, const aiScene *scene, bool flip_vertical,
                    glm::mat4 transform);
  Mesh _processMesh(aiMesh *mesh, const aiScene *scene, bool flip_vertical,
                    glm::mat4 transform);
  std::shared_ptr<Texture> _loadMaterialTexture(aiMaterial *mat,
                                                const aiScene *scene,
                                                aiTextureType type,
                                                TextureType typeName,
                                                bool flip_vertical);

  void _setVertexBoneDataToDefault(Vertex &vertex);
  void _setVertexBoneData(Vertex &vertex, uint32_t bone_id, float weight);
  void _extractBoneWeightForVertices(std::vector<Vertex> &vertices,
                                     aiMesh *mesh, glm::mat4 mesh_transform);
};
