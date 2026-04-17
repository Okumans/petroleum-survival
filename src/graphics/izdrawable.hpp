#pragma once

#include "graphics/idrawable.hpp"

class IZDrawable {
public:
  virtual ~IZDrawable() = default;
  virtual void draw(const RenderContext &ctx, float z) = 0;
};
