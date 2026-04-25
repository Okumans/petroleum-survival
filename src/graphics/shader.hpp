#pragma once

#include <cstdint>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <unordered_map>

class GLUniformCache {
  struct UniformValue {
    union {
      bool b;
      int i;
      float f;
      glm::vec2 v2;
      glm::vec3 v3;
      glm::vec4 v4;
      glm::mat2 m2;
      glm::mat3 m3;
      glm::mat4 m4;
    };

    template <typename T> bool isEqualTo(const T &val) const {
      if constexpr (std::is_same_v<T, bool>)
        return b == val;
      else if constexpr (std::is_same_v<T, int>)
        return i == val;
      else if constexpr (std::is_same_v<T, float>)
        return f == val;
      else if constexpr (std::is_same_v<T, glm::vec2>)
        return v2 == val;
      else if constexpr (std::is_same_v<T, glm::vec3>)
        return v3 == val;
      else if constexpr (std::is_same_v<T, glm::vec4>)
        return v4 == val;
      else if constexpr (std::is_same_v<T, glm::mat2>)
        return m2 == val;
      else if constexpr (std::is_same_v<T, glm::mat3>)
        return m3 == val;
      else if constexpr (std::is_same_v<T, glm::mat4>)
        return m4 == val;
      return false;
    }

    template <typename T> void set(const T &val) {
      if constexpr (std::is_same_v<T, bool>)
        b = val;
      else if constexpr (std::is_same_v<T, int>)
        i = val;
      else if constexpr (std::is_same_v<T, float>)
        f = val;
      else if constexpr (std::is_same_v<T, glm::vec2>)
        v2 = val;
      else if constexpr (std::is_same_v<T, glm::vec3>)
        v3 = val;
      else if constexpr (std::is_same_v<T, glm::vec4>)
        v4 = val;
      else if constexpr (std::is_same_v<T, glm::mat2>)
        m2 = val;
      else if constexpr (std::is_same_v<T, glm::mat3>)
        m3 = val;
      else if constexpr (std::is_same_v<T, glm::mat4>)
        m4 = val;
    }
  };

  std::unordered_map<GLint, UniformValue> m_cache;

public:
  template <typename T> bool set(GLint location, const T &value) {
    if (location == -1)
      return false;

    auto it = m_cache.find(location);

    if (it != m_cache.end() && it->second.isEqualTo(value)) {
      return false;
    }

    m_cache[location].set(value);
    return true;
  }
};

class Shader {
private:
  unsigned int m_id;

  std::unordered_map<uint32_t, GLint> m_uniformLocationCache;
  GLUniformCache m_uniformValueCache;

public:
  Shader(const char *vertex_shader_path, const char *fragment_shader_path,
         const char *geometry_shader_path = nullptr);
  Shader(GLuint ID);
  ~Shader();

  Shader(const Shader &) = delete;
  Shader &operator=(const Shader &) = delete;
  Shader(Shader &&other) noexcept;

  static Shader fromFile(const char *vertex_shader_path,
                         const char *fragment_shader_path,
                         const char *geometry_shader_path = nullptr);

  static Shader fromSource(const char *vertex_shader_source,
                           const char *fragment_shader_source,
                           const char *geometry_shader_source = nullptr);

  void use();

  void setBool(const std::string &name, bool value);
  void setInt(const std::string &name, int value);
  void setFloat(const std::string &name, float value);
  void setVec2(const std::string &name, const glm::vec2 &value);
  void setVec2(const std::string &name, float x, float y);
  void setVec3(const std::string &name, const glm::vec3 &value);
  void setVec3(const std::string &name, float x, float y, float z);
  void setVec4(const std::string &name, const glm::vec4 &value);
  void setVec4(const std::string &name, float x, float y, float z, float w);
  void setMat2(const std::string &name, const glm::mat2 &mat);
  void setMat3(const std::string &name, const glm::mat3 &mat);
  void setMat4(const std::string &name, const glm::mat4 &mat);

  [[nodiscard]] GLuint getID() const { return m_id; }

private:
  GLint _getUniformLocation(std::string_view name);
  static void checkCompileErrors(GLuint shader, std::string type);
};
