#pragma once

#include "graphics/idrawable.hpp"
#include "graphics/texture.hpp"
#include <glad/gl.h>
#include <memory>

class Skybox : public IDrawable {
private:
  GLuint m_vao, m_vbo;
  std::shared_ptr<Texture> m_skyboxTex;

public:
  Skybox(std::shared_ptr<Texture> skybox = nullptr);
  ~Skybox() override;

  void setTexture(std::shared_ptr<Texture> skybox_tex);

  virtual void draw(const RenderContext &ctx) override;
  [[nodiscard]] GLuint getVAO() const { return m_vao; }

private:
  void _setupSkybox();
};
