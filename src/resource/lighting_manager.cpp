#include "lighting_manager.hpp"
#include <format>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

std::vector<Light> LightingManager::m_lights = {};

void LightingManager::addLight(const Light &light) {
  if (m_lights.size() < MAX_LIGHTS) {
    m_lights.push_back(light);
  }
}

void LightingManager::setLight(size_t index, const Light &light) {
  if (index < m_lights.size()) {
    m_lights[index] = light;
  }
}

void LightingManager::clearLights() { m_lights.clear(); }

void LightingManager::apply(Shader &shader) {
  shader.setInt("u_NumLights", (int)m_lights.size());

  for (size_t i = 0; i < m_lights.size(); ++i) {
    std::string base = std::format("u_Lights[{}]", i);
    shader.setVec3(base + ".position", m_lights[i].position);
    shader.setVec3(base + ".color", m_lights[i].color);
    shader.setInt(base + ".type", (int)m_lights[i].type);
  }
}

glm::mat4
LightingManager::calculateLightSpaceMatrix(const glm::vec3 &targetPos) {
  Light shadowCaster = getShadowCaster();
  glm::vec3 lightDir = glm::normalize(shadowCaster.position);

  // 1. Tune the size to your camera view.
  // If it's too big, shadows are blurry. If too small, they cut off.
  float size = 25.0f;
  float nearPlane = -100.0f;
  float farPlane = 100.0f;

  glm::mat4 lightProjection =
      glm::ortho(-size, size, -size, size, nearPlane, farPlane);

  // 2. View matrix: We move the "eye" back along the light direction.
  // Use targetPos as the center so the shadow map follows the chicken.
  glm::vec3 lightPos = targetPos - (lightDir * 50.0f);
  glm::mat4 lightView =
      glm::lookAt(lightPos, targetPos, glm::vec3(0.0f, 1.0f, 0.0f));

  glm::mat4 lightSpaceMatrix = lightProjection * lightView;

  // 3. The Snap: Round the world-space origin in shadow-map texel units
  // This prevents the "shimmering" edges when the camera/player moves.
  glm::vec4 shadowOrigin = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
  shadowOrigin = lightSpaceMatrix * shadowOrigin;
  shadowOrigin *= (4096.0f / 2.0f);

  glm::vec4 roundedOrigin = glm::round(shadowOrigin);
  glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
  roundOffset = roundOffset * (2.0f / 4096.0f);
  roundOffset.z = 0.0f;
  roundOffset.w = 0.0f;

  // Apply offset to the projection's translation column
  lightProjection[3] += roundOffset;

  return lightProjection * lightView;
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
