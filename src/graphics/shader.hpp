#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>

class Shader {
public:
  unsigned int ID;

  Shader(const char *vertexShaderPath, const char *fragmentShaderPath,
         const char *geometryShaderPath = nullptr);
  Shader(GLuint ID);
  ~Shader();

  Shader(const Shader &) = delete;
  Shader &operator=(const Shader &) = delete;
  Shader(Shader &&other) noexcept;

  static Shader fromFile(const char *vertexShaderPath,
                         const char *fragmentShaderPath,
                         const char *geometryShaderPath = nullptr);

  static Shader fromSource(const char *vertexShaderSource,
                           const char *fragmentShaderSource,
                           const char *geometryShaderSource = nullptr);

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

private:
  static void checkCompileErrors(GLuint shader, std::string type);
};
