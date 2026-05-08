#pragma once

#include "graphics/shader.hpp"
#include "utility/enum_map.hpp"
#include "utility/not_initialized.hpp"
#include <memory>
enum class ShaderType { UI, PBR, SKYBOX, SHADOW, IRRADIANCE, DEBUG };

class ShaderManager {
public:
  static Utility::SettableNotInitialized<
      EnumMap<ShaderType, std::unique_ptr<Shader>>, "s_shaders",
      EnumMapValidator<EnumMap<ShaderType, std::unique_ptr<Shader>>>>
      s_shaders;

  static Shader &loadFromPath(ShaderType type, const char *vert_shader_path,
                              const char *frag_shader_path);
  static Shader &loadFromSource(ShaderType type, const char *vert_shader_path,
                                const char *frag_shader_path);
  [[nodiscard]] static Shader &get(ShaderType type);
  static void ensureInit();
};
