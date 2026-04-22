#pragma once

#include "graphics/shader.hpp"
#include <glm/glm.hpp>
#include <vector>

enum class LightType : int { POINT = 0, DIRECTIONAL = 1 };

struct Light {
  LightType type = LightType::POINT;
  glm::vec3 position = glm::vec3(0.0f); // direction if directional
  glm::vec3 color = glm::vec3(1.0f);
  bool castsShadows = false;
};

class LightingManager {
private:
  static std::vector<Light> m_lights;
  static const int MAX_LIGHTS = 4;

public:
  // Consistent API
  static void add(const Light &light);
  static void set(size_t index, const Light &light);
  static const Light &get(size_t index);
  static const Light *tryGet(size_t index);
  static bool exists(size_t index);
  static size_t count();
  static void clear();

  static void addLight(const Light &light);
  static void setLight(size_t index, const Light &light);
  static void clearLights();

  // Apply all light uniforms to the given shader
  static void apply(Shader &shader);

  // Shadow utilities
  static glm::mat4 calculateLightSpaceMatrix(const glm::vec3 &targetPos);
  static bool hasShadowCaster();
  static Light getShadowCaster();
};
