#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>

class Shader {
private:
  unsigned int m_id;

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
  static void checkCompileErrors(GLuint shader, std::string type);
};
