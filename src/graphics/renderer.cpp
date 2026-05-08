#include "renderer.hpp"
#include "graphics/animator.hpp"
#include <algorithm>

Renderer::Renderer() {}

Renderer::~Renderer() {
  if (m_fence) {
    glDeleteSync(m_fence);
  }
  if (m_instanceMapped) {
    glUnmapNamedBuffer(m_instanceSSBO);
  }
  if (m_boneMapped) {
    glUnmapNamedBuffer(m_boneSSBO);
  }
  if (m_instanceSSBO)
    glDeleteBuffers(1, &m_instanceSSBO);
  if (m_boneSSBO)
    glDeleteBuffers(1, &m_boneSSBO);
}

void Renderer::setup() {
  GLbitfield flags =
      GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
  if (!m_instanceSSBO) {
    glCreateBuffers(1, &m_instanceSSBO);
    glNamedBufferStorage(m_instanceSSBO, MAX_INSTANCES * sizeof(InstanceData),
                         nullptr, flags);
    m_instanceMapped = (InstanceData *)glMapNamedBufferRange(
        m_instanceSSBO, 0, MAX_INSTANCES * sizeof(InstanceData), flags);
  }
  if (!m_boneSSBO) {
    glCreateBuffers(1, &m_boneSSBO);
    glNamedBufferStorage(m_boneSSBO,
                         MAX_INSTANCES * MAX_BONES * sizeof(glm::mat4), nullptr,
                         flags);
    m_boneMapped = (glm::mat4 *)glMapNamedBufferRange(
        m_boneSSBO, 0, MAX_INSTANCES * MAX_BONES * sizeof(glm::mat4), flags);
  }
}

void Renderer::beginFrame() {
  if (m_fence) {
    glClientWaitSync(m_fence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
    glDeleteSync(m_fence);
    m_fence = nullptr;
  }
  m_instanceOffset = 0;
  m_modelQueue.clear();
  m_meshQueue.clear();
}

void Renderer::submit(const Model *model, const glm::mat4 &transform,
                      const Animator *animator, const glm::vec3 &emission) {
  m_modelQueue.push_back({model, transform, animator, emission});
}

void Renderer::submit(const Mesh *mesh, const glm::mat4 &transform,
                      const glm::vec3 &emission) {
  m_meshQueue.push_back({mesh, transform, emission});
}

void Renderer::flush(const RenderContext &ctx) {
  // 1. Sort Model Queue by Model Address
  std::sort(m_modelQueue.begin(), m_modelQueue.end(),
            [](const ModelDrawCommand &a, const ModelDrawCommand &b) {
              return a.model < b.model;
            });

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_instanceSSBO);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_boneSSBO);

  const Model *current_model = nullptr;
  bool has_animation = false;
  size_t batch_count = 0;
  size_t batch_start_offset = m_instanceOffset;

  auto flush_model_batch = [&]() {
    if (batch_count == 0 || !current_model)
      return;

    ctx.shader.setBool("u_EnableTerrainTint", false);
    ctx.shader.setBool("u_HasAnimation", has_animation);
    ctx.shader.setInt("u_BaseInstance", batch_start_offset);
    const_cast<Model *>(current_model)->drawInstanced(ctx, batch_count);

    has_animation = false;
    batch_count = 0;
    batch_start_offset = m_instanceOffset;
  };

  for (const auto &cmd : m_modelQueue) {
    if (cmd.model != current_model) {
      flush_model_batch();
      current_model = cmd.model;
    }

    if (m_instanceOffset >= MAX_INSTANCES) {
      break;
    }

    m_instanceMapped[m_instanceOffset].model = cmd.transform;
    m_instanceMapped[m_instanceOffset].emission = glm::vec4(cmd.emission, 0.0f);

    if (cmd.animator) {
      has_animation = true;
      const auto &bones = cmd.animator->getFinalBoneMatrices();
      size_t bone_count = std::min(bones.size(), MAX_BONES);
      size_t bone_start = m_instanceOffset * MAX_BONES;

      for (size_t i = 0; i < bone_count; ++i)
        m_boneMapped[bone_start + i] = bones[i];
      for (size_t i = bone_count; i < MAX_BONES; ++i)
        m_boneMapped[bone_start + i] = glm::mat4(1.0f);
    }

    else if (has_animation) { // Pad with identity if batch is mixed
      size_t bone_start = m_instanceOffset * MAX_BONES;
      for (size_t i = 0; i < MAX_BONES; ++i) {
        m_boneMapped[bone_start + i] = glm::mat4(1.0f);
      }
    }

    m_instanceOffset++;
    batch_count++;
  }
  flush_model_batch();

  // 2. Process Mesh Queue (Terrain)
  std::sort(m_meshQueue.begin(), m_meshQueue.end(),
            [](const MeshDrawCommand &a, const MeshDrawCommand &b) {
              return a.mesh < b.mesh;
            });

  const Mesh *current_mesh = nullptr;
  batch_count = 0;
  batch_start_offset = m_instanceOffset;

  auto flush_mesh_batch = [&]() {
    if (batch_count == 0 || !current_mesh)
      return;

    ctx.shader.setBool("u_EnableTerrainTint", true);
    ctx.shader.setVec3("u_TerrainTintLow", glm::vec3(0.50f, 0.70f, 0.55f));
    ctx.shader.setVec3("u_TerrainTintHigh", glm::vec3(1.45f, 1.20f, 0.85f));
    ctx.shader.setFloat("u_TerrainTintScale", 0.006f);
    ctx.shader.setFloat("u_TerrainTintStrength", 0.9f);
    ctx.shader.setBool("u_HasAnimation", false);
    ctx.shader.setInt("u_BaseInstance", batch_start_offset);
    const_cast<Mesh *>(current_mesh)->drawInstanced(ctx, batch_count);

    batch_count = 0;
    batch_start_offset = m_instanceOffset;
  };

  for (const auto &cmd : m_meshQueue) {
    if (cmd.mesh != current_mesh) {
      flush_mesh_batch();
      current_mesh = cmd.mesh;
    }

    if (m_instanceOffset >= MAX_INSTANCES) {
      break;
    }

    m_instanceMapped[m_instanceOffset].model = cmd.transform;
    m_instanceMapped[m_instanceOffset].emission = glm::vec4(cmd.emission, 0.0f);
    m_instanceOffset++;
    batch_count++;
  }
  flush_mesh_batch();

  m_fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}
