#pragma once

#include "graphics/mesh.hpp"
#include "graphics/model.hpp"
#include "graphics/render_context.hpp"

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>

class Animator;

class Renderer {
public:
  static constexpr size_t MAX_INSTANCES = 50000;
  static constexpr size_t MAX_BONES = 200;

  struct InstanceData {
    glm::mat4 model;
    glm::vec4 emission;
  };

private:
  GLuint m_instanceSSBO = 0;
  GLuint m_boneSSBO = 0;
  InstanceData *m_instanceMapped = nullptr;
  glm::mat4 *m_boneMapped = nullptr;
  GLsync m_fence = nullptr;
  size_t m_instanceOffset = 0;

  struct ModelDrawCommand {
    const Model *model;
    glm::mat4 transform;
    const Animator *animator;
    glm::vec3 emission;
  };

  struct MeshDrawCommand {
    const Mesh *mesh;
    glm::mat4 transform;
    glm::vec3 emission;
  };

  std::vector<ModelDrawCommand> m_modelQueue;
  std::vector<MeshDrawCommand> m_meshQueue;

public:
  Renderer();
  ~Renderer();

  void setup();
  void beginFrame();

  void submit(const Model *model, const glm::mat4 &transform,
              const Animator *animator = nullptr,
              const glm::vec3 &emission = glm::vec3(0.0f));
  void submit(const Mesh *mesh, const glm::mat4 &transform,
              const glm::vec3 &emission = glm::vec3(0.0f));

  void flush(const RenderContext &ctx);
};
