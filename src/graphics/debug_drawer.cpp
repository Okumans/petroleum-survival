#include "graphics/debug_drawer.hpp"
#include "resource/shader_manager.hpp"
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

GLuint DebugDrawer::m_vao = 0;
GLuint DebugDrawer::m_vbo = 0;
GLuint DebugDrawer::m_ebo = 0;
bool DebugDrawer::m_initialized = false;

void DebugDrawer::_init() {
  if (m_initialized)
    return;

  float vertices[] =
      {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1};

  uint32_t indices[] = {
      0,
      1,
      1,
      2,
      2,
      3,
      3,
      0, // bottom
      4,
      5,
      5,
      6,
      6,
      7,
      7,
      4, // top
      0,
      4,
      1,
      5,
      2,
      6,
      3,
      7 // sides
  };

  glCreateVertexArrays(1, &m_vao);
  glCreateBuffers(1, &m_vbo);
  glCreateBuffers(1, &m_ebo);

  glNamedBufferStorage(m_vbo, sizeof(vertices), vertices, 0);
  glNamedBufferStorage(m_ebo, sizeof(indices), indices, 0);

  glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, 3 * sizeof(float));
  glVertexArrayElementBuffer(m_vao, m_ebo);

  glEnableVertexArrayAttrib(m_vao, 0);
  glVertexArrayAttribFormat(m_vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
  glVertexArrayAttribBinding(m_vao, 0, 0);

  m_initialized = true;
}

void DebugDrawer::drawAABB(const RenderContext &ctx,
                           const AABB &aabb,
                           const glm::vec3 &color) {
  if (aabb.isEmpty())
    return;
  _init();

  Shader &shader = ShaderManager::getShader(ShaderType::DEBUG);
  shader.use();

  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, aabb.min);
  model = glm::scale(model, aabb.max - aabb.min);

  shader.setMat4("u_Model", model);
  shader.setMat4("u_View", ctx.camera.getViewMatrix());
  shader.setMat4("u_Projection", ctx.camera.getProjectionMatrix());
  shader.setVec3("u_Color", color);

  glBindVertexArray(m_vao);
  glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
}
