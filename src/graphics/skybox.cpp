#include "skybox.hpp"
#include "graphics/idrawable.hpp"
#include <print>

Skybox::Skybox(std::shared_ptr<Texture> skybox_tex)
    : m_vao(0), m_vbo(0), m_skyboxTex(skybox_tex) {
  _setupSkybox();
}

void Skybox::setTexture(std::shared_ptr<Texture> skybox_tex) {
  m_skyboxTex = std::move(skybox_tex);
}

Skybox::~Skybox() {
  glDeleteBuffers(1, &m_vbo);
  glDeleteVertexArrays(1, &m_vao);
}

void Skybox::_setupSkybox() {
  float vertices[] = {// positions
                      -1.0f,
                      1.0f,
                      -1.0f,
                      -1.0f,
                      -1.0f,
                      -1.0f,
                      1.0f,
                      -1.0f,
                      -1.0f,
                      1.0f,
                      -1.0f,
                      -1.0f,
                      1.0f,
                      1.0f,
                      -1.0f,
                      -1.0f,
                      1.0f,
                      -1.0f,

                      -1.0f,
                      -1.0f,
                      1.0f,
                      -1.0f,
                      -1.0f,
                      -1.0f,
                      -1.0f,
                      1.0f,
                      -1.0f,
                      -1.0f,
                      1.0f,
                      -1.0f,
                      -1.0f,
                      1.0f,
                      1.0f,
                      -1.0f,
                      -1.0f,
                      1.0f,

                      1.0f,
                      -1.0f,
                      -1.0f,
                      1.0f,
                      -1.0f,
                      1.0f,
                      1.0f,
                      1.0f,
                      1.0f,
                      1.0f,
                      1.0f,
                      1.0f,
                      1.0f,
                      1.0f,
                      -1.0f,
                      1.0f,
                      -1.0f,
                      -1.0f,

                      -1.0f,
                      -1.0f,
                      1.0f,
                      -1.0f,
                      1.0f,
                      1.0f,
                      1.0f,
                      1.0f,
                      1.0f,
                      1.0f,
                      1.0f,
                      1.0f,
                      1.0f,
                      -1.0f,
                      1.0f,
                      -1.0f,
                      -1.0f,
                      1.0f,

                      -1.0f,
                      1.0f,
                      -1.0f,
                      1.0f,
                      1.0f,
                      -1.0f,
                      1.0f,
                      1.0f,
                      1.0f,
                      1.0f,
                      1.0f,
                      1.0f,
                      -1.0f,
                      1.0f,
                      1.0f,
                      -1.0f,
                      1.0f,
                      -1.0f,

                      -1.0f,
                      -1.0f,
                      -1.0f,
                      -1.0f,
                      -1.0f,
                      1.0f,
                      1.0f,
                      -1.0f,
                      -1.0f,
                      1.0f,
                      -1.0f,
                      -1.0f,
                      -1.0f,
                      -1.0f,
                      1.0f,
                      1.0f,
                      -1.0f,
                      1.0f};

  glCreateVertexArrays(1, &m_vao);
  glCreateBuffers(1, &m_vbo);

  glNamedBufferStorage(m_vbo, sizeof(vertices), vertices, 0);
  glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, 3 * sizeof(float));

  glEnableVertexArrayAttrib(m_vao, 0);
  glVertexArrayAttribFormat(m_vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
  glVertexArrayAttribBinding(m_vao, 0, 0);
}

void Skybox::draw(const RenderContext &ctx) {
  if (m_skyboxTex == nullptr) {
    std::println(stderr, "Warning: skybox texture doesn't exists");
    return;
  }

  ctx.shader.use();
  ctx.shader.setMat4("u_View", ctx.camera.getViewMatrix());
  ctx.shader.setMat4("u_Projection", ctx.camera.getProjectionMatrix());

  // Change depth function so depth test passes when values are equal to depth
  // buffer's content
  glDepthFunc(GL_LEQUAL);
  // Disable culling for skybox so it's visible from inside
  glDisable(GL_CULL_FACE);

  glBindTextureUnit(0, m_skyboxTex->getTexID());
  ctx.shader.setInt("u_Skybox", 0);

  glBindVertexArray(m_vao);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);

  // Set depth function and culling back to default
  glEnable(GL_CULL_FACE);
  glDepthFunc(GL_LESS);
  glDepthFunc(GL_LESS);
}
