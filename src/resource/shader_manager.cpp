#include "shader_manager.hpp"
#include "utility/enum_map.hpp"
#include "utility/not_initialized.hpp"

#include <memory>

Utility::SettableNotInitialized<
    EnumMap<ShaderType, std::unique_ptr<Shader>>, "s_shaders",
    EnumMapValidator<EnumMap<ShaderType, std::unique_ptr<Shader>>>>
    ShaderManager::s_shaders;

Shader &ShaderManager::loadFromPath(ShaderType type,
                                    const char *vert_shader_path,
                                    const char *frag_shader_path) {
  s_shaders.set(type, std::make_unique<Shader>(
                          Shader(vert_shader_path, frag_shader_path)));

  return *s_shaders.getUnvalidated(type);
}

Shader &ShaderManager::loadFromSource(ShaderType type,
                                      const char *vert_shader_source,
                                      const char *frag_shader_source) {
  s_shaders.set(type, std::make_unique<Shader>(Shader::fromSource(
                          vert_shader_source, frag_shader_source)));

  return *s_shaders.getUnvalidated(type);
}

Shader &ShaderManager::get(ShaderType type) {
  return *s_shaders.ensureInitialized().at(type);
}

void ShaderManager::ensureInit() { (void)s_shaders.ensureInitialized(); }
