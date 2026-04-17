#pragma once

#include "graphics/camera.hpp"
#include "shader.hpp"

struct RenderContext {
  Shader &shader;
  Camera &camera;
  double deltaTime;
};

class IDrawable {
public:
  virtual void draw(const RenderContext &ctx) = 0;
};
