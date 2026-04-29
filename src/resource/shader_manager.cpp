#include "shader_manager.hpp"
#include "utility/enum_map.hpp"
#include "utility/not_initialized.hpp"

#include <memory>

Utility::SettableNotInitialized<
    EnumMap<ShaderType, std::unique_ptr<Shader>>, "s_shaders",
    EnumMapValidator<EnumMap<ShaderType, std::unique_ptr<Shader>>>>
    ShaderManager::s_shaders;

Shader &ShaderManager::loadFromPath(ShaderType type, const char *vertShaderPath,
                                    const char *fragShaderPath) {
  s_shaders.set(
      type, std::make_unique<Shader>(Shader(vertShaderPath, fragShaderPath)));

  return *s_shaders.getUnvalidated(type);
}

Shader &ShaderManager::loadFromSource(ShaderType type,
                                      const char *vertShaderSource,
                                      const char *fragShaderSource) {
  s_shaders.set(type, std::make_unique<Shader>(Shader::fromSource(
                          vertShaderSource, fragShaderSource)));

  return *s_shaders.getUnvalidated(type);
}

Shader &ShaderManager::get(ShaderType type) {
  return *s_shaders.ensureInitialized().at(type);
}

void ShaderManager::ensureInit() { (void)s_shaders.ensureInitialized(); }
