#pragma once

#include "graphics/idrawable.hpp"
#include "mesh.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

class Model : public IDrawable {
private:
  // model data
  std::vector<Mesh> m_meshes;
  std::string m_directory;
  std::string m_path;

public:
  Model(const char *path, bool flip_vertical = false);

  Model(const Model &) = delete;
  Model &operator=(Model &) const = delete;

  Model(Model &&) noexcept;

  void draw(const RenderContext &ctx) override;
  const std::vector<Mesh> &getMeshes() const { return m_meshes; }

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
};
