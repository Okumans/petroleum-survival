#pragma once

#include "graphics/material.hpp"

#include "graphics/idrawable.hpp"

#include <glad/gl.h>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>

#include <vector>

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texCoords;
  glm::vec3 tangent;
  glm::vec3 bitangent;
};

class Mesh : public IDrawable {
private:
  // mesh data
  std::vector<Vertex> m_vertices;
  std::vector<uint32_t> m_indices;
  Material m_material;

  glm::vec3 m_baseColor;
  float m_opacity;

  // render data
  GLuint m_vao, m_vbo, m_ebo;

public:
  Mesh(std::vector<Vertex> &&vertices, std::vector<uint32_t> &&indices,
       Material material, glm::vec3 color, float opacity = 1.0f);

  ~Mesh();

  // Delete all kind copy constrcutor for now
  Mesh(const Mesh &) = delete;
  Mesh &operator=(const Mesh &) = delete;
  Mesh(Mesh &&other) noexcept;

  const std::vector<Vertex> &getVertices() const { return m_vertices; }
  const std::vector<uint32_t> &getIndices() const { return m_indices; }
  const Material &getMaterial() const { return m_material; }
  const glm::vec3 &getBaseColor() const { return m_baseColor; }
  float getOpacity() const { return m_opacity; }
  void setBaseColor(const glm::vec3 &color) { m_baseColor = color; }
  void setOpacity(float opacity) { m_opacity = opacity; }
  void setMaterial(const Material &material) { m_material = material; }

  virtual void draw(const RenderContext &ctx) override;
  void draw(const RenderContext &ctx, const Material &material);

private:
  void _setupMesh();
};
