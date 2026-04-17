#include "shader.hpp"
#include <fstream>
#include <iostream>
#include <print>
#include <sstream>

Shader::Shader(const char *vertexShaderPath,
               const char *fragmentShaderPath,
               const char *geometryShaderPath)
    : Shader(
          fromFile(vertexShaderPath, fragmentShaderPath, geometryShaderPath)) {}

Shader::Shader(GLuint ID) : ID(ID) {}

Shader::~Shader() {
  if (ID)
    glDeleteProgram(ID);
}

Shader::Shader(Shader &&other) noexcept : ID(other.ID) { other.ID = 0; }

Shader Shader::fromFile(const char *vertexShaderPath,
                        const char *fragmentShaderPath,
                        const char *geometryShaderPath) {
  std::string vertexShaderSource, fragmentShaderSource, geometryShaderSource;

  {
    std::ifstream vertexShaderFile(vertexShaderPath);
    std::ifstream fragmentShaderFile(fragmentShaderPath);

    if (!vertexShaderFile.is_open()) {
      std::println(stderr,
                   "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: Vertex Shader "
                   "File \"{}\" isn't successfully open!",
                   vertexShaderPath);
      return Shader(0);
    }

    if (!fragmentShaderFile.is_open()) {
      std::println(stderr,
                   "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: Fragment Shader "
                   "File \"{}\" isn't successfully open!",
                   fragmentShaderPath);
      return Shader(0);
    }

    std::stringstream vShaderStream, fShaderStream;
    vShaderStream << vertexShaderFile.rdbuf();
    fShaderStream << fragmentShaderFile.rdbuf();

    vertexShaderSource = vShaderStream.str();
    fragmentShaderSource = fShaderStream.str();
  }

  if (geometryShaderPath != nullptr) {
    std::ifstream geometryShaderFile(geometryShaderPath);
    if (!geometryShaderFile.is_open()) {
      std::println(stderr,
                   "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: Geometry Shader "
                   "File \"{}\" isn't successfully open!",
                   geometryShaderPath);
      return Shader(0);
    }
    std::stringstream gShaderStream;
    gShaderStream << geometryShaderFile.rdbuf();
    geometryShaderSource = gShaderStream.str();
  }

  return fromSource(vertexShaderSource.c_str(),
                    fragmentShaderSource.c_str(),
                    (geometryShaderPath != nullptr)
                        ? geometryShaderSource.c_str()
                        : nullptr);
}

Shader Shader::fromSource(const char *vertexShaderSource,
                          const char *fragmentShaderSource,
                          const char *geometryShaderSource) {
  GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex, 1, &vertexShaderSource, NULL);
  glCompileShader(vertex);
  checkCompileErrors(vertex, "VERTEX");

  GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragment);
  checkCompileErrors(fragment, "FRAGMENT");

  GLuint geometry = 0;
  if (geometryShaderSource != nullptr) {
    geometry = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(geometry, 1, &geometryShaderSource, NULL);
    glCompileShader(geometry);
    checkCompileErrors(geometry, "GEOMETRY");
  }

  GLuint ID = glCreateProgram();
  glAttachShader(ID, vertex);
  glAttachShader(ID, fragment);
  if (geometryShaderSource != nullptr)
    glAttachShader(ID, geometry);

  glLinkProgram(ID);
  checkCompileErrors(ID, "PROGRAM");

  glDeleteShader(vertex);
  glDeleteShader(fragment);
  if (geometryShaderSource != nullptr)
    glDeleteShader(geometry);

  return Shader(ID);
}

void Shader::use() { glUseProgram(ID); }

void Shader::setBool(const std::string &name, bool value) {
  glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::setInt(const std::string &name, int value) {
  glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setFloat(const std::string &name, float value) {
  glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setVec2(const std::string &name, const glm::vec2 &value) {
  glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void Shader::setVec2(const std::string &name, float x, float y) {
  glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
}

void Shader::setVec3(const std::string &name, const glm::vec3 &value) {
  glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void Shader::setVec3(const std::string &name, float x, float y, float z) {
  glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
}

void Shader::setVec4(const std::string &name, const glm::vec4 &value) {
  glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void Shader::setVec4(
    const std::string &name, float x, float y, float z, float w) {
  glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
}

void Shader::setMat2(const std::string &name, const glm::mat2 &mat) {
  glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()),
                     1,
                     GL_FALSE,
                     &mat[0][0]);
}

void Shader::setMat3(const std::string &name, const glm::mat3 &mat) {
  glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()),
                     1,
                     GL_FALSE,
                     &mat[0][0]);
}

void Shader::setMat4(const std::string &name, const glm::mat4 &mat) {
  glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()),
                     1,
                     GL_FALSE,
                     &mat[0][0]);
}

void Shader::checkCompileErrors(GLuint shader, std::string type) {
  GLint success;
  GLchar infoLog[1024];
  if (type != "PROGRAM") {
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(shader, 1024, NULL, infoLog);
      std::println("ERROR::SHADER_COMPILATION_ERROR of type: {}\n{}\n -- "
                   "--------------------------------------------------- -- ",
                   type,
                   infoLog);
    }
  } else {
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(shader, 1024, NULL, infoLog);
      std::println("ERROR::PROGRAM_LINKING_ERROR of type: {}\n{}\n -- "
                   "--------------------------------------------------- -- ",
                   type,
                   infoLog);
    }
  }
}
