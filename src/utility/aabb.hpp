#pragma once

#include <glm/glm.hpp>
#include <limits>

struct AABB {
  glm::vec3 min{0.0f};
  glm::vec3 max{0.0f};

  glm::vec3 getCenter() const { return (min + max) * 0.5f; }

  glm::vec3 getSize() const { return max - min; }

  bool intersects(const AABB &other) const {
    return glm::all(glm::lessThanEqual(min, other.max)) &&
           glm::all(glm::greaterThanEqual(max, other.min));
  }

  bool contains(const glm::vec3 &point) const {
    return glm::all(glm::greaterThanEqual(point, min)) &&
           glm::all(glm::lessThanEqual(point, max));
  }

  glm::vec3 getClosestPoint(const glm::vec3 &point) const {
    return glm::clamp(point, min, max);
  }

  void grow(const glm::vec3 &point) {
    min = glm::min(min, point);
    max = glm::max(max, point);
  }

  void grow(const AABB &other) {
    if (other.isEmpty())
      return;
    min = glm::min(min, other.min);
    max = glm::max(max, other.max);
  }

  void translate(const glm::vec3 &offset) {
    min += offset;
    max += offset;
  }

  void transform(const glm::mat4 &matrix) {
    glm::vec3 corners[8] = {{min.x, min.y, min.z},
                            {max.x, min.y, min.z},
                            {min.x, max.y, min.z},
                            {max.x, max.y, min.z},
                            {min.x, min.y, max.z},
                            {max.x, min.y, max.z},
                            {min.x, max.y, max.z},
                            {max.x, max.y, max.z}};

    min = glm::vec3(std::numeric_limits<float>::max());
    max = glm::vec3(std::numeric_limits<float>::lowest());

    for (size_t i = 0; i < 8; ++i) {
      glm::vec3 transformed = glm::vec3(matrix * glm::vec4(corners[i], 1.0f));
      grow(transformed);
    }
  }

  static AABB empty() {
    return {glm::vec3(std::numeric_limits<float>::max()),
            glm::vec3(std::numeric_limits<float>::lowest())};
  }

  bool isEmpty() const {
    return min.x > max.x || min.y > max.y || min.z > max.z;
  }
};
