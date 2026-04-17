#include "mesh.hpp"
#include "graphics/idrawable.hpp"
#include "graphics/material.hpp"

#include <glad/gl.h>

#include <cstdint>
#include <utility>

Mesh::Mesh(std::vector<Vertex> &&vertices,
           std::vector<uint32_t> &&indices,
           Material material,
           glm::vec3 color,
           float opacity)
    : m_vertices(std::move(vertices)), m_indices(std::move(indices)),
      m_material(material), m_baseColor(color), m_opacity(opacity), m_vao(0),
      m_vbo(0), m_ebo(0) {
  _setupMesh();
}

Mesh::~Mesh() {
  if (m_vao)
    glDeleteVertexArrays(1, &m_vao);

  if (m_vbo)
    glDeleteBuffers(1, &m_vbo);

  if (m_ebo)
    glDeleteBuffers(1, &m_ebo);
}

Mesh::Mesh(Mesh &&other) noexcept
    : m_vertices(std::move(other.m_vertices)),
      m_indices(std::move(other.m_indices)),
      m_material(std::move(other.m_material)), m_baseColor(other.m_baseColor),
      m_opacity(other.m_opacity), m_vao(other.m_vao), m_vbo(other.m_vbo),
      m_ebo(other.m_ebo) {
  other.m_vertices.clear();
  other.m_indices.clear();
  other.m_baseColor = glm::vec3(0.0f);
  other.m_opacity = 1.0f;
  other.m_vao = 0;
  other.m_vbo = 0;
  other.m_ebo = 0;
}

void Mesh::_setupMesh() {
  glCreateVertexArrays(1, &m_vao);
  glCreateBuffers(1, &m_vbo);
  glCreateBuffers(1, &m_ebo);

  glNamedBufferStorage(m_vbo,
                       m_vertices.size() * sizeof(Vertex),
                       m_vertices.data(),
                       0);
  glNamedBufferStorage(m_ebo,
                       m_indices.size() * sizeof(uint32_t),
                       m_indices.data(),
                       0);

  glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, sizeof(Vertex));
  glVertexArrayElementBuffer(m_vao, m_ebo);

  // Position vec3
  glEnableVertexArrayAttrib(m_vao, 0);
  glVertexArrayAttribFormat(m_vao,
                            0,
                            3,
                            GL_FLOAT,
                            GL_FALSE,
                            offsetof(Vertex, position));
  glVertexArrayAttribBinding(m_vao, 0, 0);

  // Normal vec3
  glEnableVertexArrayAttrib(m_vao, 1);
  glVertexArrayAttribFormat(m_vao,
                            1,
                            3,
                            GL_FLOAT,
                            GL_FALSE,
                            offsetof(Vertex, normal));
  glVertexArrayAttribBinding(m_vao, 1, 0);

  // TexCoords vec2
  glEnableVertexArrayAttrib(m_vao, 2);
  glVertexArrayAttribFormat(m_vao,
                            2,
                            2,
                            GL_FLOAT,
                            GL_FALSE,
                            offsetof(Vertex, texCoords));
  glVertexArrayAttribBinding(m_vao, 2, 0);

  // Tangent vec3
  glEnableVertexArrayAttrib(m_vao, 3);
  glVertexArrayAttribFormat(m_vao,
                            3,
                            3,
                            GL_FLOAT,
                            GL_FALSE,
                            offsetof(Vertex, tangent));
  glVertexArrayAttribBinding(m_vao, 3, 0);

  // Bitangent vec3
  glEnableVertexArrayAttrib(m_vao, 4);
  glVertexArrayAttribFormat(m_vao,
                            4,
                            3,
                            GL_FLOAT,
                            GL_FALSE,
                            offsetof(Vertex, bitangent));
  glVertexArrayAttribBinding(m_vao, 4, 0);
}

void Mesh::draw(const RenderContext &ctx) { draw(ctx, m_material); }

void Mesh::draw(const RenderContext &ctx, const Material &material) {
  int counter = 0;

  // 1. Diffuse/Albedo
  ctx.shader.setInt("u_DiffuseTex", counter);
  if (material.getDiffuse()) {
    glBindTextureUnit(counter, material.getDiffuse()->getTexID());
  } else {
    glBindTextureUnit(
        counter, TextureManager::getTexture(STATIC_WHITE_TEXTURE)->getTexID());
  }
  counter++;

  // 2. Normal
  ctx.shader.setInt("u_NormalTex", counter);
  if (material.getNormal()) {
    glBindTextureUnit(counter, material.getNormal()->getTexID());
  } else {
    glBindTextureUnit(
        counter, TextureManager::getTexture(STATIC_NORMAL_TEXTURE)->getTexID());
  }
  counter++;

  // 3. Height/Parallax
  ctx.shader.setInt("u_HeightTex", counter);
  if (material.getHeight()) {
    glBindTextureUnit(counter, material.getHeight()->getTexID());
  } else {
    glBindTextureUnit(
        counter, TextureManager::getTexture(STATIC_BLACK_TEXTURE)->getTexID());
  }
  counter++;

  // 4. Metallic
  ctx.shader.setInt("u_MetallicTex", counter);
  if (material.getMetallic()) {
    glBindTextureUnit(counter, material.getMetallic()->getTexID());
  } else {
    // Default Metallic = 0.0 (Black)
    glBindTextureUnit(
        counter, TextureManager::getTexture(STATIC_BLACK_TEXTURE)->getTexID());
  }
  counter++;

  // 5. Roughness
  ctx.shader.setInt("u_RoughnessTex", counter);
  if (material.getRoughness()) {
    glBindTextureUnit(counter, material.getRoughness()->getTexID());
  } else {
    // Default Roughness = 1.0 (White)
    glBindTextureUnit(
        counter, TextureManager::getTexture(STATIC_WHITE_TEXTURE)->getTexID());
  }
  counter++;

  // 6. AO
  ctx.shader.setInt("u_AOTex", counter);
  if (material.getAO()) {
    glBindTextureUnit(counter, material.getAO()->getTexID());
  } else {
    glBindTextureUnit(
        counter, TextureManager::getTexture(STATIC_WHITE_TEXTURE)->getTexID());
  }
  counter++;

  // Factors
  bool usePackedMR = false;
  if (material.getMetallic() && material.getRoughness() &&
      material.getMetallic()->getTexID() ==
          material.getRoughness()->getTexID()) {
    usePackedMR = true;
  }
  ctx.shader.setBool("u_UsePackedMR", usePackedMR);

  ctx.shader.setVec3("u_BaseColor", m_baseColor);
  ctx.shader.setFloat("u_Opacity", m_opacity);
  ctx.shader.setFloat("u_MetallicFactor", material.getMetallicFactor());
  ctx.shader.setFloat("u_RoughnessFactor", material.getRoughnessFactor());
  ctx.shader.setFloat("u_AOFactor", material.getAOFactor());

  glBindVertexArray(m_vao);
  glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0);
}

