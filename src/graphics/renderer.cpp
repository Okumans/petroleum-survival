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
  GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
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

  const Model *currentModel = nullptr;
  bool hasAnimation = false;
  size_t batchCount = 0;
  size_t batchStartOffset = m_instanceOffset;

  auto flushModelBatch = [&]() {
    if (batchCount == 0 || !currentModel)
      return;

    ctx.shader.setBool("u_HasAnimation", hasAnimation);
    ctx.shader.setInt("u_BaseInstance", batchStartOffset);
    const_cast<Model *>(currentModel)->drawInstanced(ctx, batchCount);

    hasAnimation = false;
    batchCount = 0;
    batchStartOffset = m_instanceOffset;
  };

  for (const auto &cmd : m_modelQueue) {
    if (cmd.model != currentModel) {
      flushModelBatch();
      currentModel = cmd.model;
    }

    if (m_instanceOffset >= MAX_INSTANCES) {
      break;
    }

    m_instanceMapped[m_instanceOffset].model = cmd.transform;
    m_instanceMapped[m_instanceOffset].emission = glm::vec4(cmd.emission, 0.0f);

    if (cmd.animator) {
      hasAnimation = true;
      const auto &bones = cmd.animator->getFinalBoneMatrices();
      size_t boneCount = std::min(bones.size(), MAX_BONES);
      size_t boneStart = m_instanceOffset * MAX_BONES;
      for (size_t i = 0; i < boneCount; ++i) {
        m_boneMapped[boneStart + i] = bones[i];
      }
      for (size_t i = boneCount; i < MAX_BONES; ++i) {
        m_boneMapped[boneStart + i] = glm::mat4(1.0f);
      }
    } else if (hasAnimation) { // Pad with identity if batch is mixed
      size_t boneStart = m_instanceOffset * MAX_BONES;
      for (size_t i = 0; i < MAX_BONES; ++i) {
        m_boneMapped[boneStart + i] = glm::mat4(1.0f);
      }
    }

    m_instanceOffset++;
    batchCount++;
  }
  flushModelBatch();

  // 2. Process Mesh Queue (Terrain)
  std::sort(m_meshQueue.begin(), m_meshQueue.end(),
            [](const MeshDrawCommand &a, const MeshDrawCommand &b) {
              return a.mesh < b.mesh;
            });

  const Mesh *currentMesh = nullptr;
  batchCount = 0;
  batchStartOffset = m_instanceOffset;

  auto flushMeshBatch = [&]() {
    if (batchCount == 0 || !currentMesh)
      return;

    ctx.shader.setBool("u_HasAnimation", false);
    ctx.shader.setInt("u_BaseInstance", batchStartOffset);
    const_cast<Mesh *>(currentMesh)->drawInstanced(ctx, batchCount);

    batchCount = 0;
    batchStartOffset = m_instanceOffset;
  };

  for (const auto &cmd : m_meshQueue) {
    if (cmd.mesh != currentMesh) {
      flushMeshBatch();
      currentMesh = cmd.mesh;
    }

    if (m_instanceOffset >= MAX_INSTANCES) {
      break;
    }

    m_instanceMapped[m_instanceOffset].model = cmd.transform;
    m_instanceMapped[m_instanceOffset].emission = glm::vec4(cmd.emission, 0.0f);
    m_instanceOffset++;
    batchCount++;
  }
  flushMeshBatch();

  m_fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}
