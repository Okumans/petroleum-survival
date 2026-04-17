#pragma once

#include "graphics/shader.hpp"
#include "utility/enum_map.hpp"
#include "utility/not_initialized.hpp"
#include <memory>

enum class ShaderType { UI, PBR, SKYBOX, SHADOW, IRRADIANCE, WATER, DEBUG };

class ShaderManager {
public:
  static SettableNotInitialized<
      EnumMap<ShaderType, std::unique_ptr<Shader>>, "s_shaders",
      EnumMapValidator<EnumMap<ShaderType, std::unique_ptr<Shader>>>>
      s_shaders;

  static Shader &loadShader(ShaderType type, const char *vertShaderPath,
                            const char *fragShaderPath);

  static Shader &loadShaderSource(ShaderType type, const char *vertShaderSource,
                                  const char *fragShaderSource);

  static Shader &getShader(ShaderType type);
};
