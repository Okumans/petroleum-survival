#include "shader_manager.hpp"
#include "utility/enum_map.hpp"
#include "utility/not_initialized.hpp"

#include <memory>

SettableNotInitialized<
    EnumMap<ShaderType, std::unique_ptr<Shader>>, "s_shaders",
    EnumMapValidator<EnumMap<ShaderType, std::unique_ptr<Shader>>>>
    ShaderManager::s_shaders;

Shader &ShaderManager::loadShader(ShaderType type, const char *vertShaderPath,
                                  const char *fragShaderPath) {
  s_shaders.set(
      type, std::make_unique<Shader>(Shader(vertShaderPath, fragShaderPath)));

  return *s_shaders.getUnvalidated(type);
}

Shader &ShaderManager::loadShaderSource(ShaderType type,
                                        const char *vertShaderSource,
                                        const char *fragShaderSource) {
  s_shaders.set(type, std::make_unique<Shader>(Shader::fromSource(
                          vertShaderSource, fragShaderSource)));

  return *s_shaders.getUnvalidated(type);
}

Shader &ShaderManager::getShader(ShaderType type) {
  return *s_shaders.ensureInitialized().at(type);
}
