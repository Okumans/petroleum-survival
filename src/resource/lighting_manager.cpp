#include "lighting_manager.hpp"
#include "graphics/shader_uniforms.hpp"
#include <glm/gtc/matrix_transform.hpp>

std::vector<Light> LightingManager::m_lights = {};

void LightingManager::add(const Light &light) {
  if (m_lights.size() < MAX_LIGHTS) {
    m_lights.push_back(light);
  }
}

void LightingManager::set(size_t index, const Light &light) {
  if (index < m_lights.size()) {
    m_lights[index] = light;
  }
}

const Light &LightingManager::get(size_t index) { return m_lights.at(index); }

const Light *LightingManager::tryGet(size_t index) {
  if (index >= m_lights.size()) {
    return nullptr;
  }

  return &m_lights[index];
}

bool LightingManager::exists(size_t index) { return index < m_lights.size(); }

size_t LightingManager::count() { return m_lights.size(); }

void LightingManager::clear() { m_lights.clear(); }

void LightingManager::apply(Shader &shader) {
  constexpr auto lighting_uniforms = ShaderUniforms::generateLightUniforms();

  shader.setInt("u_NumLights", static_cast<int>(m_lights.size()));

  for (size_t i = 0; i < m_lights.size(); ++i) {
    shader.setVec3(lighting_uniforms[i].positionHash, m_lights[i].position);
    shader.setVec3(lighting_uniforms[i].colorHash, m_lights[i].color);
    shader.setInt(lighting_uniforms[i].typeHash, (int)m_lights[i].type);
  }
}

glm::mat4
LightingManager::calculateLightSpaceMatrix(const glm::vec3 &target_pos) {
  Light shadow_caster = getShadowCaster();
  glm::vec3 light_dir = glm::normalize(shadow_caster.position);

  // 1. Tune the size to your camera view.
  // If it's too big, shadows are blurry. If too small, they cut off.
  float size = 40.0f;
  float near_plane = -100.0f;
  float far_plane = 100.0f;

  glm::mat4 light_projection =
      glm::ortho(-size, size, -size, size, near_plane, far_plane);

  // 2. View matrix: We move the "eye" back along the light direction.
  // Use targetPos as the center so the shadow map follows the chicken.
  glm::vec3 light_pos = target_pos - (light_dir * 50.0f);
  glm::mat4 light_view =
      glm::lookAt(light_pos, target_pos, glm::vec3(0.0f, 1.0f, 0.0f));

  glm::mat4 light_space_matrix = light_projection * light_view;

  // 3. The Snap: Round the world-space origin in shadow-map texel units
  // This prevents the "shimmering" edges when the camera/player moves.
  glm::vec4 shadow_origin = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
  shadow_origin = light_space_matrix * shadow_origin;
  shadow_origin *= (4096.0f / 2.0f);

  glm::vec4 rounded_origin = glm::round(shadow_origin);
  glm::vec4 round_offset = rounded_origin - shadow_origin;
  round_offset = round_offset * (2.0f / 4096.0f);
  round_offset.z = 0.0f;
  round_offset.w = 0.0f;

  // Apply offset to the projection's translation column
  light_projection[3] += round_offset;

  return light_projection * light_view;
}

bool LightingManager::hasShadowCaster() {
  for (const auto &light : m_lights) {
    if (light.castsShadows)
      return true;
  }

  return false;
}

Light LightingManager::getShadowCaster() {
  for (const auto &light : m_lights) {
    if (light.castsShadows)
      return light;
  }

  // Fallback if none (should check hasShadowCaster first)
  if (!m_lights.empty())
    return m_lights[0];

  return Light();
}
