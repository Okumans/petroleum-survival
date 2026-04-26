#pragma once

#include "graphics/shader.hpp"
#include "utility/enum_map.hpp"
#include "utility/not_initialized.hpp"
#include <memory>

enum class ShaderType { UI, PBR, SKYBOX, SHADOW, IRRADIANCE, DEBUG };

class ShaderManager {
public:
  static SettableNotInitialized<
      EnumMap<ShaderType, std::unique_ptr<Shader>>, "s_shaders",
      EnumMapValidator<EnumMap<ShaderType, std::unique_ptr<Shader>>>>
      s_shaders;

  static Shader &loadFromPath(ShaderType type, const char *vertShaderPath,
                              const char *fragShaderPath);
  static Shader &loadFromSource(ShaderType type, const char *vertShaderSource,
                                const char *fragShaderSource);
  [[nodiscard]] static Shader &get(ShaderType type);
  static void ensureInit();
};
