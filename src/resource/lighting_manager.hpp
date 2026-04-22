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
  [[nodiscard]] static const Light &get(size_t index);
  [[nodiscard]] static const Light *tryGet(size_t index);
  [[nodiscard]] static bool exists(size_t index);
  [[nodiscard]] static size_t count();
  static void apply(Shader &shader);
  static void clear();

  static glm::mat4 calculateLightSpaceMatrix(const glm::vec3 &targetPos);
  [[nodiscard]] static bool hasShadowCaster();
  [[nodiscard]] static Light getShadowCaster();
};
