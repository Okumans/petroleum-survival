#include "model.hpp"
#include "assimp/mesh.h"
#include "graphics/idrawable.hpp"
#include "graphics/material.hpp"
#include "graphics/texture.hpp"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <glm/fwd.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <format>
#include <memory>
#include <optional>
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
  const aiScene *scene = import.ReadFile(
      path, aiProcess_Triangulate | aiProcess_FlipUVs |
                aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices |
                aiProcess_GlobalScale);

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

void Model::_processNode(aiNode *node, const aiScene *scene, bool flip_vertical,
                         glm::mat4 transform) {
  glm::mat4 nodeTransform;
  memcpy(glm::value_ptr(nodeTransform), &node->mTransformation,
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

Mesh Model::_processMesh(aiMesh *mesh, const aiScene *scene, bool flip_vertical,
                         glm::mat4 transform) {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  vertices.reserve(mesh->mNumVertices);
  indices.reserve(mesh->mNumFaces);

  // Always pre-bake the node transform into vertices (keeps AABB correct
  // and non-animated appearance consistent). For skinned meshes, the bone
  // OffsetMatrix is adjusted with inverse(transform) to compensate.
  glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(transform)));

  for (size_t i = 0; i < mesh->mNumVertices; ++i) {
    glm::vec4 pos4 =
        transform * glm::vec4(mesh->mVertices[i].x, mesh->mVertices[i].y,
                              mesh->mVertices[i].z, 1.0f);
    glm::vec3 position(pos4.x, pos4.y, pos4.z);

    glm::vec3 normal =
        (mesh->mNormals)
            ? normalMat * glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y,
                                    mesh->mNormals[i].z)
            : glm::vec3(0.0f);

    glm::vec2 texCoords = (mesh->mTextureCoords[0])
                              ? glm::vec2(mesh->mTextureCoords[0][i].x,
                                          mesh->mTextureCoords[0][i].y)
                              : glm::vec2(0.0f);

    glm::vec3 tangent =
        (mesh->mTangents)
            ? normalMat * glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y,
                                    mesh->mTangents[i].z)
            : glm::vec3(1.0f, 0.0f, 0.0f);

    glm::vec3 bitangent = (mesh->mBitangents)
                              ? normalMat * glm::vec3(mesh->mBitangents[i].x,
                                                      mesh->mBitangents[i].y,
                                                      mesh->mBitangents[i].z)
                              : glm::vec3(0.0f, 1.0f, 0.0f);

    Vertex vertex;
    vertex.position = position;
    vertex.normal = normal;
    vertex.texCoords = texCoords;
    vertex.tangent = tangent;
    vertex.bitangent = bitangent;

    _setVertexBoneDataToDefault(vertex);

    vertices.push_back(vertex);
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

  aiMaterial *assimp_material = scene->mMaterials[mesh->mMaterialIndex];

  // Fetch Factors
  aiGetMaterialFloat(assimp_material, AI_MATKEY_METALLIC_FACTOR,
                     &metallicFactor);
  aiGetMaterialFloat(assimp_material, AI_MATKEY_ROUGHNESS_FACTOR,
                     &roughnessFactor);
  aiGetMaterialFloat(assimp_material, AI_MATKEY_OPACITY, &opacity);
  aiGetMaterialColor(assimp_material, AI_MATKEY_COLOR_DIFFUSE, &diffuseColor);

  mat_builder.setMetallicFactor(metallicFactor);
  mat_builder.setRoughnessFactor(roughnessFactor);
  mat_builder.setAOFactor(aoFactor);

  // 1. Diffuse / Base Color
  if (assimp_material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
    if (auto &&tex =
            _loadMaterialTexture(assimp_material, scene, aiTextureType_DIFFUSE,
                                 TextureType::DIFFUSE, flip_vertical);
        tex)
      mat_builder.setDiffuse(tex);
  }

  else if (assimp_material->GetTextureCount(aiTextureType_BASE_COLOR) > 1) {
    if (auto &&tex = _loadMaterialTexture(assimp_material, scene,
                                          aiTextureType_BASE_COLOR,
                                          TextureType::DIFFUSE, flip_vertical);
        tex)
      mat_builder.setDiffuse(tex);
  }

  // 2. Normal / Height
  if (assimp_material->GetTextureCount(aiTextureType_NORMALS) > 0) {
    if (auto &&tex =
            _loadMaterialTexture(assimp_material, scene, aiTextureType_NORMALS,
                                 TextureType::NORMAL, flip_vertical);
        tex)
      mat_builder.setNormal(tex);

  }

  else if (assimp_material->GetTextureCount(aiTextureType_HEIGHT) > 0) {
    if (auto &&tex =
            _loadMaterialTexture(assimp_material, scene, aiTextureType_HEIGHT,
                                 TextureType::NORMAL, flip_vertical);
        tex)
      mat_builder.setNormal(tex);
  }

  // 3. Metallic-Roughness (GLTF Packed)
  if (assimp_material->GetTextureCount(aiTextureType_UNKNOWN) > 0) {
    if (auto &&tex =
            _loadMaterialTexture(assimp_material, scene, aiTextureType_UNKNOWN,
                                 TextureType::METALLIC, flip_vertical);
        tex) {
      mat_builder.setMetallic(tex);
      mat_builder.setRoughness(tex);
    }
  }

  else if (assimp_material->GetTextureCount(aiTextureType_METALNESS) > 0) {
    if (auto &&tex = _loadMaterialTexture(assimp_material, scene,
                                          aiTextureType_METALNESS,
                                          TextureType::METALLIC, flip_vertical);
        tex) {
      mat_builder.setMetallic(tex);
      mat_builder.setRoughness(tex);
    }
  }

  else if (assimp_material->GetTextureCount(aiTextureType_SHININESS) > 0) {
    if (auto &&tex = _loadMaterialTexture(
            assimp_material, scene, aiTextureType_SHININESS,
            TextureType::ROUGHNESS, flip_vertical);
        tex)
      mat_builder.setRoughness(tex);
  }

  // 4. Ambient Occlusion
  if (assimp_material->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION) > 0) {
    if (auto &&tex = _loadMaterialTexture(assimp_material, scene,
                                          aiTextureType_AMBIENT_OCCLUSION,
                                          TextureType::AO, flip_vertical);
        tex)
      mat_builder.setAO(tex);

  }

  else if (assimp_material->GetTextureCount(aiTextureType_AMBIENT) > 0) {
    if (auto &&tex =
            _loadMaterialTexture(assimp_material, scene, aiTextureType_AMBIENT,
                                 TextureType::AO, flip_vertical);
        tex)
      mat_builder.setAO(tex);
  }

  Material material = mat_builder.create();
  glm::vec3 final_color(diffuseColor.r, diffuseColor.g, diffuseColor.b);

  _extractBoneWeightForVertices(vertices, mesh, transform);

  return Mesh(std::move(vertices), std::move(indices), std::move(material),
              final_color, opacity);
}

std::shared_ptr<Texture> Model::_loadMaterialTexture(aiMaterial *mat,
                                                     const aiScene *scene,
                                                     aiTextureType type,
                                                     TextureType typeName,
                                                     bool flip_vertical) {
  for (size_t i = 0; i < mat->GetTextureCount(type); ++i) {
    aiString path;
    mat->GetTexture(type, i, &path);

    // 1. Check for Embedded Textures (e.g. GLB files or fully embedded FBX)
    // GetEmbeddedTexture dynamically resolves indices (like "*0") or absolute
    // string bindings.
    const aiTexture *embedded_tex = scene->GetEmbeddedTexture(path.C_Str());

    std::string unique_name;

    if (embedded_tex) {
      // If embedded, namespace the texture name to the model path to prevent
      // collisions
      unique_name = std::format("{}:{}", m_path, path.C_Str());
    } else {
      // 2. Not Embedded, external asset lookup
      std::string_view path_str = path.C_Str();
      std::string standard_path = std::format("{}/{}", m_directory, path_str);

      if (std::filesystem::exists(standard_path)) {
        // Attempt valid relative paths properly dictated by formats like .mtl
        // (e.g. "textures/rock.png")
        unique_name = standard_path;
      } else {
        // 3. Fallback: For broken Assimp imports housing absolute rigid paths
        // (e.g. "/home/foo/bar...") We aggressively strip all broken
        // directories and assume the file rests alongside the 3D model in
        // m_directory.
        auto last_slash = path_str.find_last_of("/\\");
        std::string_view filename = (last_slash != std::string::npos)
                                        ? path_str.substr(last_slash + 1)
                                        : path_str;
        unique_name = std::format("{}/{}", m_directory, filename);
      }
    }

    TextureName texture_name(std::move(unique_name));

    if (!TextureManager::exists(texture_name)) {
      if (embedded_tex) {
        if (embedded_tex->mHeight == 0) {
          return TextureManager::load(texture_name, typeName,
                                      embedded_tex->pcData,
                                      embedded_tex->mWidth, flip_vertical);
        }

        return TextureManager::load(
            texture_name, typeName, embedded_tex->pcData,
            embedded_tex->mWidth * embedded_tex->mHeight * 4, flip_vertical);
      }

      return TextureManager::load(texture_name, typeName, unique_name.c_str(),
                                  flip_vertical);
    } else {
      return TextureManager::copy(texture_name);
    }
  }

  return nullptr;
}

void Model::_setVertexBoneDataToDefault(Vertex &vertex) {
  for (size_t i = 0; i < MAX_BONE_INFLUENCE; i++) {
    vertex.m_boneIDs[i] = -1;
    vertex.m_weights[i] = 0.0f;
  }
}

void Model::_setVertexBoneData(Vertex &vertex, uint32_t bone_id, float weight) {
  for (size_t i = 0; i < MAX_BONE_INFLUENCE; ++i) {
    if (vertex.m_boneIDs[i] < 0) {
      vertex.m_weights[i] = weight;
      vertex.m_boneIDs[i] = bone_id;
      break;
    }
  }
}

void Model::_extractBoneWeightForVertices(std::vector<Vertex> &vertices,
                                          aiMesh *mesh,
                                          glm::mat4 mesh_transform) {
  // Pre-compute the inverse of the mesh's accumulated node transform.
  // Since vertices are pre-baked with mesh_transform, the offset must
  // undo that bake so the bone math sees mesh-local coordinates:
  //   finalMatrix * v_stored = boneGlobal * rawOffset * v_local
  //   finalMatrix * (T * v_local) = boneGlobal * rawOffset * v_local
  //   => finalMatrix = boneGlobal * rawOffset * inverse(T)
  //   => adjustedOffset = rawOffset * inverse(T)
  glm::mat4 inv_mesh_transform = glm::inverse(mesh_transform);

  for (size_t bone_index = 0; bone_index < mesh->mNumBones; ++bone_index) {
    std::optional<uint32_t> bone_id;
    std::string bone_name = mesh->mBones[bone_index]->mName.C_Str();

    if (m_boneInfoMap.find(bone_name) == m_boneInfoMap.end()) {
      BoneInfo new_bone_info;
      new_bone_info.id = m_boneCount;

      glm::mat4 target_mat;
      std::memcpy(glm::value_ptr(target_mat),
                  &mesh->mBones[bone_index]->mOffsetMatrix, sizeof(float) * 16);
      glm::mat4 raw_offset = glm::transpose(target_mat);
      new_bone_info.offset = raw_offset * inv_mesh_transform;

      m_boneInfoMap[bone_name] = new_bone_info;
      bone_id = m_boneCount++;
    } else {
      bone_id = m_boneInfoMap[bone_name].id;
    }

    assert(bone_id.has_value());

    aiVertexWeight *weights = mesh->mBones[bone_index]->mWeights;
    size_t num_weights = mesh->mBones[bone_index]->mNumWeights;

    for (size_t weight_index = 0; weight_index < num_weights; ++weight_index) {
      size_t vertex_id = weights[weight_index].mVertexId;
      float weight = weights[weight_index].mWeight;

      assert(vertex_id <= vertices.size());
      _setVertexBoneData(vertices[vertex_id], bone_id.value(), weight);
    }
  }

  // Weight normalization pass to prevent spaghetti deformations when total
  // weight deviates from 1.0. This physically ensures the vertices don't
  // collapse or explode outward.
  for (auto &vertex : vertices) {
    float total_weight = 0.0f;
    for (size_t i = 0; i < MAX_BONE_INFLUENCE; ++i) {
      total_weight += vertex.m_weights[i];
    }

    if (total_weight > 0.0f) {
      for (size_t i = 0; i < MAX_BONE_INFLUENCE; ++i) {
        vertex.m_weights[i] /= total_weight;
      }
    }
  }
}
