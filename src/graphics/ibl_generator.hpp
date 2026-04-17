#pragma once

#include "graphics/shader.hpp"
#include "graphics/skybox.hpp"
#include "graphics/texture.hpp"
#include <memory>

class IBLGenerator {
public:
  static std::shared_ptr<Texture>
  generateIrradianceMap(const Texture &skybox, const Skybox &cubeMesh,
                        Shader &irradianceShader);
};
