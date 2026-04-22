#include "shader.hpp"
#include <fstream>
#include <iostream>
#include <print>
#include <sstream>

Shader::Shader(const char *vertexShaderPath, const char *fragmentShaderPath,
               const char *geometryShaderPath)
    : Shader(
          fromFile(vertexShaderPath, fragmentShaderPath, geometryShaderPath)) {}

Shader::Shader(GLuint ID) : m_id(ID) {}

Shader::~Shader() {
  if (m_id)
    glDeleteProgram(m_id);
}

Shader::Shader(Shader &&other) noexcept : m_id(other.m_id) { other.m_id = 0; }

Shader Shader::fromFile(const char *vertex_shader_path,
                        const char *fragment_shader_path,
                        const char *geometry_shader_path) {
  std::string vertex_shader_source, fragment_shader_source,
      geometry_shader_source;

  {
    std::ifstream vertex_shader_file(vertex_shader_path);
    std::ifstream fragment_shader_file(fragment_shader_path);

    if (!vertex_shader_file.is_open()) {
      std::println(stderr,
                   "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: Vertex Shader "
                   "File \"{}\" isn't successfully open!",
                   vertex_shader_path);
      return Shader(0);
    }

    if (!fragment_shader_file.is_open()) {
      std::println(stderr,
                   "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: Fragment Shader "
                   "File \"{}\" isn't successfully open!",
                   fragment_shader_path);
      return Shader(0);
    }

    std::stringstream v_shader_stream, f_shader_stream;
    v_shader_stream << vertex_shader_file.rdbuf();
    f_shader_stream << fragment_shader_file.rdbuf();

    vertex_shader_source = v_shader_stream.str();
    fragment_shader_source = f_shader_stream.str();
  }

  if (geometry_shader_path != nullptr) {
    std::ifstream geometry_shader_file(geometry_shader_path);
    if (!geometry_shader_file.is_open()) {
      std::println(stderr,
                   "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: Geometry Shader "
                   "File \"{}\" isn't successfully open!",
                   geometry_shader_path);
      return Shader(0);
    }
    std::stringstream g_shader_stream;
    g_shader_stream << geometry_shader_file.rdbuf();
    geometry_shader_source = g_shader_stream.str();
  }

  return fromSource(
      vertex_shader_source.c_str(), fragment_shader_source.c_str(),
      (geometry_shader_path != nullptr) ? geometry_shader_source.c_str()
                                        : nullptr);
}

Shader Shader::fromSource(const char *vertex_shader_source,
                          const char *fragment_shader_source,
                          const char *geometry_shader_source) {
  GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex, 1, &vertex_shader_source, NULL);
  glCompileShader(vertex);
  checkCompileErrors(vertex, "VERTEX");

  GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment, 1, &fragment_shader_source, NULL);
  glCompileShader(fragment);
  checkCompileErrors(fragment, "FRAGMENT");

  GLuint geometry = 0;
  if (geometry_shader_source != nullptr) {
    geometry = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(geometry, 1, &geometry_shader_source, NULL);
    glCompileShader(geometry);
    checkCompileErrors(geometry, "GEOMETRY");
  }

  GLuint id = glCreateProgram();
  glAttachShader(id, vertex);
  glAttachShader(id, fragment);
  if (geometry_shader_source != nullptr)
    glAttachShader(id, geometry);

  glLinkProgram(id);
  checkCompileErrors(id, "PROGRAM");

  glDeleteShader(vertex);
  glDeleteShader(fragment);
  if (geometry_shader_source != nullptr)
    glDeleteShader(geometry);

  return Shader(id);
}

void Shader::use() { glUseProgram(m_id); }

void Shader::setBool(const std::string &name, bool value) {
  glUniform1i(glGetUniformLocation(m_id, name.c_str()), (int)value);
}

void Shader::setInt(const std::string &name, int value) {
  glUniform1i(glGetUniformLocation(m_id, name.c_str()), value);
}

void Shader::setFloat(const std::string &name, float value) {
  glUniform1f(glGetUniformLocation(m_id, name.c_str()), value);
}

void Shader::setVec2(const std::string &name, const glm::vec2 &value) {
  glUniform2fv(glGetUniformLocation(m_id, name.c_str()), 1, &value[0]);
}

void Shader::setVec2(const std::string &name, float x, float y) {
  glUniform2f(glGetUniformLocation(m_id, name.c_str()), x, y);
}

void Shader::setVec3(const std::string &name, const glm::vec3 &value) {
  glUniform3fv(glGetUniformLocation(m_id, name.c_str()), 1, &value[0]);
}

void Shader::setVec3(const std::string &name, float x, float y, float z) {
  glUniform3f(glGetUniformLocation(m_id, name.c_str()), x, y, z);
}

void Shader::setVec4(const std::string &name, const glm::vec4 &value) {
  glUniform4fv(glGetUniformLocation(m_id, name.c_str()), 1, &value[0]);
}

void Shader::setVec4(const std::string &name, float x, float y, float z,
                     float w) {
  glUniform4f(glGetUniformLocation(m_id, name.c_str()), x, y, z, w);
}

void Shader::setMat2(const std::string &name, const glm::mat2 &mat) {
  glUniformMatrix2fv(glGetUniformLocation(m_id, name.c_str()), 1, GL_FALSE,
                     &mat[0][0]);
}

void Shader::setMat3(const std::string &name, const glm::mat3 &mat) {
  glUniformMatrix3fv(glGetUniformLocation(m_id, name.c_str()), 1, GL_FALSE,
                     &mat[0][0]);
}

void Shader::setMat4(const std::string &name, const glm::mat4 &mat) {
  glUniformMatrix4fv(glGetUniformLocation(m_id, name.c_str()), 1, GL_FALSE,
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
                   type, infoLog);
    }
  } else {
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(shader, 1024, NULL, infoLog);
      std::println("ERROR::PROGRAM_LINKING_ERROR of type: {}\n{}\n -- "
                   "--------------------------------------------------- -- ",
                   type, infoLog);
    }
  }
}
