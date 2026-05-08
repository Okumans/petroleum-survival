#include "shader.hpp"
#include <fstream>
#include <iostream>
#include <print>
#include <sstream>
#include <stdexcept>

Shader::Shader(const char *vertex_shader_path, const char *fragment_shader_path,
               const char *geometry_shader_path)
    : Shader(fromFile(vertex_shader_path, fragment_shader_path,
                      geometry_shader_path)) {}

Shader::Shader(GLuint id) : m_id(id) {}

Shader::~Shader() {
  if (m_id)
    glDeleteProgram(m_id);
}

Shader::Shader(Shader &&other) noexcept
    : m_id(other.m_id),
      m_uniformLocationCache(std::move(other.m_uniformLocationCache)) {
  other.m_id = 0;
}

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
  _checkCompileErrors(vertex, "VERTEX");

  GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment, 1, &fragment_shader_source, NULL);
  glCompileShader(fragment);
  _checkCompileErrors(fragment, "FRAGMENT");

  GLuint geometry = 0;
  if (geometry_shader_source != nullptr) {
    geometry = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(geometry, 1, &geometry_shader_source, NULL);
    glCompileShader(geometry);
    _checkCompileErrors(geometry, "GEOMETRY");
  }

  GLuint id = glCreateProgram();
  glAttachShader(id, vertex);
  glAttachShader(id, fragment);
  if (geometry_shader_source != nullptr)
    glAttachShader(id, geometry);

  glLinkProgram(id);
  _checkCompileErrors(id, "PROGRAM");

  glDeleteShader(vertex);
  glDeleteShader(fragment);
  if (geometry_shader_source != nullptr)
    glDeleteShader(geometry);

  return Shader(id);
}

void Shader::use() { glUseProgram(m_id); }

GLint Shader::_getUniformLocation(NameHash name) {
  auto it = m_uniformLocationCache.find(name);

  if (it == m_uniformLocationCache.end()) {
    throw std::runtime_error(std::format(
        "Uniform with hash \"{}\" need to be define, before set", name.hash));
  }

  return it->second.second;
}

void Shader::setBool(NameHash name, bool value) {
  const GLint location = _getUniformLocation(name);
  if (m_uniformValueCache.set(location, value))
    glUniform1i(location, (int)value);
}

void Shader::setInt(NameHash name, int value) {
  const GLint location = _getUniformLocation(name);
  if (m_uniformValueCache.set(location, value))
    glUniform1i(location, value);
}

void Shader::setFloat(NameHash name, float value) {
  const GLint location = _getUniformLocation(name);
  if (m_uniformValueCache.set(location, value))
    glUniform1f(location, value);
}

void Shader::setVec2(NameHash name, const glm::vec2 &value) {
  const GLint location = _getUniformLocation(name);
  if (m_uniformValueCache.set(location, value))
    glUniform2fv(location, 1, &value[0]);
}

void Shader::setVec2(NameHash name, float x, float y) {
  const GLint location = _getUniformLocation(name);
  const glm::vec2 value{x, y};
  if (m_uniformValueCache.set(location, value))
    glUniform2f(location, x, y);
}

void Shader::setVec3(NameHash name, const glm::vec3 &value) {
  const GLint location = _getUniformLocation(name);
  if (m_uniformValueCache.set(location, value))
    glUniform3fv(location, 1, &value[0]);
}

void Shader::setVec3(NameHash name, float x, float y, float z) {
  const GLint location = _getUniformLocation(name);
  const glm::vec3 value{x, y, z};
  if (m_uniformValueCache.set(location, value))
    glUniform3f(location, x, y, z);
}

void Shader::setVec4(NameHash name, const glm::vec4 &value) {
  const GLint location = _getUniformLocation(name);
  if (m_uniformValueCache.set(location, value))
    glUniform4fv(location, 1, &value[0]);
}

void Shader::setVec4(NameHash name, float x, float y, float z, float w) {
  const GLint location = _getUniformLocation(name);
  const glm::vec4 value{x, y, z, w};
  if (m_uniformValueCache.set(location, value))
    glUniform4f(location, x, y, z, w);
}

void Shader::setMat2(NameHash name, const glm::mat2 &mat) {
  const GLint location = _getUniformLocation(name);
  if (m_uniformValueCache.set(location, mat))
    glUniformMatrix2fv(location, 1, GL_FALSE, &mat[0][0]);
}

void Shader::setMat3(NameHash name, const glm::mat3 &mat) {
  const GLint location = _getUniformLocation(name);
  if (m_uniformValueCache.set(location, mat))
    glUniformMatrix3fv(location, 1, GL_FALSE, &mat[0][0]);
}

void Shader::setMat4(NameHash name, const glm::mat4 &mat) {
  const GLint location = _getUniformLocation(name);
  if (m_uniformValueCache.set(location, mat))
    glUniformMatrix4fv(location, 1, GL_FALSE, &mat[0][0]);
}

void Shader::define(const std::string name) {
  NameHash name_hash(name);

  auto it = m_uniformLocationCache.find(name_hash);

  if (it != m_uniformLocationCache.end())
    return;

  GLint location = glGetUniformLocation(m_id, name.c_str());
  m_uniformLocationCache[name_hash] = {name, location};
}

void Shader::_checkCompileErrors(GLuint shader, std::string type) {
  GLint success;
  GLchar info_log[1024];
  if (type != "PROGRAM") {
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(shader, 1024, NULL, info_log);
      std::println("ERROR::SHADER_COMPILATION_ERROR of type: {}\n{}\n -- "
                   "--------------------------------------------------- -- ",
                   type, info_log);
    }
  } else {
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(shader, 1024, NULL, info_log);
      std::println("ERROR::PROGRAM_LINKING_ERROR of type: {}\n{}\n -- "
                   "--------------------------------------------------- -- ",
                   type, info_log);
    }
  }
}
