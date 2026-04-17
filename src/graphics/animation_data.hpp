#pragma once

#include <cstdint>
#include <glm/glm.hpp>

struct BoneInfo {
  uint32_t id = 0;
  glm::mat4 offset{1.0f};
};
