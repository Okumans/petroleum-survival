#include "model.hpp"
#include "graphics/idrawable.hpp"
#include "graphics/material.hpp"
#include "graphics/texture.hpp"
#include "resource/texture_manager.hpp"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cassert>
#include <glm/fwd.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <format>
#include <memory>
#include <print>
#include <string_view>

Model::Model(const char *path, bool flip_vertical) : m_path(path) {
  _loadModel(path, flip_vertical);
}

Model::Model(Model &&other) noexcept
    : m_meshes(std::move(other.m_meshes)),
      m_directory(std::move(other.m_directory)),
      m_path(std::move(other.m_path)) {
  other.m_meshes.clear();
}

void Model::draw(const RenderContext &ctx) {
  for (Mesh &mesh : m_meshes) {
    mesh.draw(ctx);
  }
}

void Model::_loadModel(const char *path, bool flip_vertical) {
  Assimp::Importer import;
  const aiScene *scene =
      import.ReadFile(path,
                      aiProcess_Triangulate | aiProcess_FlipUVs |
                          aiProcess_CalcTangentSpace |
                          aiProcess_JoinIdenticalVertices);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    std::println("ERROR::ASSIMP::{}", import.GetErrorString());

    return;
  }

  std::string_view path_sv = std::string_view(path);
  auto last_slash = path_sv.find_last_of('/');
  if (last_slash != std::string_view::npos) {
    m_directory = path_sv.substr(0, last_slash);
  } else {
    m_directory = ".";
  }

  _processNode(scene->mRootNode, scene, flip_vertical, glm::mat4(1.0f));
}

void Model::_processNode(aiNode *node,
                         const aiScene *scene,
                         bool flip_vertical,
                         glm::mat4 transform) {
  glm::mat4 nodeTransform;
  memcpy(glm::value_ptr(nodeTransform),
         &node->mTransformation,
         sizeof(float) * 16);
  nodeTransform = glm::transpose(nodeTransform);
  glm::mat4 globalTransform = transform * nodeTransform;

  for (size_t i = 0; i < node->mNumMeshes; ++i) {
    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    m_meshes.push_back(
        _processMesh(mesh, scene, flip_vertical, globalTransform));
  }

  for (size_t i = 0; i < node->mNumChildren; ++i) {
    _processNode(node->mChildren[i], scene, flip_vertical, globalTransform);
  }
}

Mesh Model::_processMesh(aiMesh *mesh,
                         const aiScene *scene,
                         bool flip_vertical,
                         glm::mat4 transform) {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  vertices.reserve(mesh->mNumVertices);
  indices.reserve(mesh->mNumFaces);

  glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(transform)));

  for (size_t i = 0; i < mesh->mNumVertices; ++i) {
    glm::vec4 pos4 = transform * glm::vec4(mesh->mVertices[i].x,
                                           mesh->mVertices[i].y,
                                           mesh->mVertices[i].z,
                                           1.0f);
    glm::vec3 position(pos4.x, pos4.y, pos4.z);

    glm::vec3 normal = (mesh->mNormals)
                           ? normalMat * glm::vec3(mesh->mNormals[i].x,
                                                   mesh->mNormals[i].y,
                                                   mesh->mNormals[i].z)
                           : glm::vec3(0.0f);

    glm::vec2 texCoords = (mesh->mTextureCoords[0])
                              ? glm::vec2(mesh->mTextureCoords[0][i].x,
                                          mesh->mTextureCoords[0][i].y)
                              : glm::vec2(0.0f);

    glm::vec3 tangent = (mesh->mTangents)
                            ? normalMat * glm::vec3(mesh->mTangents[i].x,
                                                    mesh->mTangents[i].y,
                                                    mesh->mTangents[i].z)
                            : glm::vec3(1.0f, 0.0f, 0.0f);

    glm::vec3 bitangent = (mesh->mBitangents)
                              ? normalMat * glm::vec3(mesh->mBitangents[i].x,
                                                      mesh->mBitangents[i].y,
                                                      mesh->mBitangents[i].z)
                              : glm::vec3(0.0f, 1.0f, 0.0f);

    vertices.emplace_back(position, normal, texCoords, tangent, bitangent);
  }

  for (size_t i = 0; i < mesh->mNumFaces; ++i) {
    aiFace face = mesh->mFaces[i];

    for (size_t j = 0; j < face.mNumIndices; ++j) {
      indices.push_back(face.mIndices[j]);
    }
  }

  // PBR Factors fallback
  float metallicFactor = 0.0f;
  float roughnessFactor = 1.0f;
  float opacity = 1.0f;
  float aoFactor = 1.0f;
  aiColor4D diffuseColor(1.0f, 1.0f, 1.0f, 1.0f);

  MaterialBuilder mat_builder = Material::builder();

  if (mesh->mMaterialIndex >= 0) {
    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

    // Fetch Factors
    aiGetMaterialFloat(material, AI_MATKEY_METALLIC_FACTOR, &metallicFactor);
    aiGetMaterialFloat(material, AI_MATKEY_ROUGHNESS_FACTOR, &roughnessFactor);
    aiGetMaterialFloat(material, AI_MATKEY_OPACITY, &opacity);
    aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuseColor);

    mat_builder.setMetallicFactor(metallicFactor);
    mat_builder.setRoughnessFactor(roughnessFactor);
    mat_builder.setAOFactor(aoFactor);

    // 1. Diffuse / Base Color
    if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
      if (auto &&tex = _loadMaterialTexture(material,
                                            scene,
                                            aiTextureType_DIFFUSE,
                                            TextureType::DIFFUSE,
                                            flip_vertical);
          tex)
        mat_builder.setDiffuse(tex);
    }

    else if (material->GetTextureCount(aiTextureType_BASE_COLOR) > 1) {
      if (auto &&tex = _loadMaterialTexture(material,
                                            scene,
                                            aiTextureType_BASE_COLOR,
                                            TextureType::DIFFUSE,
                                            flip_vertical);
          tex)
        mat_builder.setDiffuse(tex);
    }

    // 2. Normal / Height
    if (material->GetTextureCount(aiTextureType_NORMALS) > 0) {
      if (auto &&tex = _loadMaterialTexture(material,
                                            scene,
                                            aiTextureType_NORMALS,
                                            TextureType::NORMAL,
                                            flip_vertical);
          tex)
        mat_builder.setNormal(tex);

    }

    else if (material->GetTextureCount(aiTextureType_HEIGHT) > 0) {
      if (auto &&tex = _loadMaterialTexture(material,
                                            scene,
                                            aiTextureType_HEIGHT,
                                            TextureType::NORMAL,
                                            flip_vertical);
          tex)
        mat_builder.setNormal(tex);
    }

    // 3. Metallic-Roughness (GLTF Packed)
    if (material->GetTextureCount(aiTextureType_UNKNOWN) > 0) {
      if (auto &&tex = _loadMaterialTexture(material,
                                            scene,
                                            aiTextureType_UNKNOWN,
                                            TextureType::METALLIC,
                                            flip_vertical);
          tex) {
        mat_builder.setMetallic(tex);
        mat_builder.setRoughness(tex);
      }
    }

    else if (material->GetTextureCount(aiTextureType_METALNESS) > 0) {
      if (auto &&tex = _loadMaterialTexture(material,
                                            scene,
                                            aiTextureType_METALNESS,
                                            TextureType::METALLIC,
                                            flip_vertical);
          tex) {
        mat_builder.setMetallic(tex);
        mat_builder.setRoughness(tex);
      }
    }

    else if (material->GetTextureCount(aiTextureType_SHININESS) > 0) {
      if (auto &&tex = _loadMaterialTexture(material,
                                            scene,
                                            aiTextureType_SHININESS,
                                            TextureType::ROUGHNESS,
                                            flip_vertical);
          tex)
        mat_builder.setRoughness(tex);
    }

    // 4. Ambient Occlusion
    if (material->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION) > 0) {
      if (auto &&tex = _loadMaterialTexture(material,
                                            scene,
                                            aiTextureType_AMBIENT_OCCLUSION,
                                            TextureType::AO,
                                            flip_vertical);
          tex)
        mat_builder.setAO(tex);

    }

    else if (material->GetTextureCount(aiTextureType_AMBIENT) > 0) {
      if (auto &&tex = _loadMaterialTexture(material,
                                            scene,
                                            aiTextureType_AMBIENT,
                                            TextureType::AO,
                                            flip_vertical);
          tex)
        mat_builder.setAO(tex);
    }
  }

  Material material = mat_builder.create();
  glm::vec3 finalColor(diffuseColor.r, diffuseColor.g, diffuseColor.b);

  return Mesh(std::move(vertices),
              std::move(indices),
              std::move(material),
              finalColor,
              opacity);
}

std::shared_ptr<Texture> Model::_loadMaterialTexture(aiMaterial *mat,
                                                     const aiScene *scene,
                                                     aiTextureType type,
                                                     TextureType typeName,
                                                     bool flip_vertical) {
  for (size_t i = 0; i < mat->GetTextureCount(type); ++i) {
    aiString path;
    mat->GetTexture(type, i, &path);

    std::string unique_name;

    if (path.C_Str()[0] == '*') {
      unique_name = std::format("{}:{}", m_path, path.C_Str());
    } else {
      unique_name = std::format("{}/{}", m_directory, path.C_Str());
    }

    TextureName texture_name(std::move(unique_name));

    if (!TextureManager::exists(texture_name)) {
      if (path.C_Str()[0] == '*') {
        int index = std::stoi(path.C_Str() + 1);
        aiTexture *aiTex = scene->mTextures[index];

        if (aiTex->mHeight == 0) {
          return TextureManager::loadTexture(texture_name,
                                             typeName,
                                             aiTex->pcData,
                                             aiTex->mWidth,
                                             flip_vertical);
        }

        return TextureManager::loadTexture(texture_name,
                                           typeName,
                                           aiTex->pcData,
                                           aiTex->mWidth * aiTex->mHeight * 4,
                                           flip_vertical);
      }

      return TextureManager::loadTexture(texture_name,
                                         typeName,
                                         unique_name.c_str(),
                                         flip_vertical);
    } else {
      return TextureManager::getTexture(texture_name);
    }
  }

  return nullptr;
}
