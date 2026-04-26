#pragma once

#include "graphics/render_context.hpp"
#include "graphics/texture.hpp"
#include <glad/gl.h>
#include <memory>

class Skybox {
private:
  GLuint m_vao, m_vbo;
  std::shared_ptr<Texture> m_skyboxTex;

public:
  Skybox(std::shared_ptr<Texture> skybox = nullptr);
  ~Skybox();

  void setTexture(std::shared_ptr<Texture> skybox_tex);

  void draw(const RenderContext &ctx);
  [[nodiscard]] GLuint getVAO() const { return m_vao; }

private:
  void _setupSkybox();
};
