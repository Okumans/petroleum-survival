#include "renderer.hpp"
#include "graphics/animator.hpp"
#include <algorithm>

Renderer::Renderer() {}

Renderer::~Renderer() {
  if (m_instanceSSBO)
    glDeleteBuffers(1, &m_instanceSSBO);
  if (m_boneSSBO)
    glDeleteBuffers(1, &m_boneSSBO);
}

void Renderer::setup() {
  if (!m_instanceSSBO) {
    glCreateBuffers(1, &m_instanceSSBO);
    glNamedBufferStorage(m_instanceSSBO, MAX_INSTANCES * sizeof(glm::mat4),
                         nullptr, GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);
  }
  if (!m_boneSSBO) {
    glCreateBuffers(1, &m_boneSSBO);
    glNamedBufferStorage(m_boneSSBO,
                         MAX_INSTANCES * MAX_BONES * sizeof(glm::mat4), nullptr,
                         GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);
  }
}

void Renderer::beginFrame() {
  m_modelQueue.clear();
  m_meshQueue.clear();
}

void Renderer::submit(const Model *model, const glm::mat4 &transform,
                      const Animator *animator) {
  m_modelQueue.push_back({model, transform, animator});
}

void Renderer::submit(const Mesh *mesh, const glm::mat4 &transform) {
  m_meshQueue.push_back({mesh, transform});
}

void Renderer::flush(const RenderContext &ctx) {
  // 1. Sort Model Queue by Model Address
  std::sort(m_modelQueue.begin(), m_modelQueue.end(),
            [](const ModelDrawCommand &a, const ModelDrawCommand &b) {
              return a.model < b.model;
            });

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_instanceSSBO);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_boneSSBO);

  std::vector<glm::mat4> instanceData;
  std::vector<glm::mat4> boneData;
  instanceData.reserve(MAX_INSTANCES);
  boneData.reserve(MAX_INSTANCES * MAX_BONES);

  const Model *currentModel = nullptr;
  bool hasAnimation = false;

  auto flushModelBatch = [&]() {
    if (instanceData.empty() || !currentModel)
      return;

    glNamedBufferSubData(m_instanceSSBO, 0,
                         instanceData.size() * sizeof(glm::mat4),
                         instanceData.data());

    if (hasAnimation) {
      glNamedBufferSubData(m_boneSSBO, 0, boneData.size() * sizeof(glm::mat4),
                           boneData.data());
    }

    ctx.shader.setBool("u_HasAnimation", hasAnimation);
    const_cast<Model *>(currentModel)->drawInstanced(ctx, instanceData.size());

    instanceData.clear();
    boneData.clear();
    hasAnimation = false;
  };

  for (const auto &cmd : m_modelQueue) {
    if (cmd.model != currentModel) {
      flushModelBatch();
      currentModel = cmd.model;
    }

    if (instanceData.size() >= MAX_INSTANCES) {
      flushModelBatch();
    }

    instanceData.push_back(cmd.transform);

    if (cmd.animator) {
      hasAnimation = true;
      const auto &bones = cmd.animator->getFinalBoneMatrices();
      size_t boneCount = std::min(bones.size(), MAX_BONES);
      for (size_t i = 0; i < boneCount; ++i) {
        boneData.push_back(bones[i]);
      }
      for (size_t i = boneCount; i < MAX_BONES; ++i) {
        boneData.push_back(glm::mat4(1.0f));
      }
    } else if (hasAnimation) { // Pad with identity if batch is mixed
      for (size_t i = 0; i < MAX_BONES; ++i) {
        boneData.push_back(glm::mat4(1.0f));
      }
    }
  }
  flushModelBatch();

  // 2. Process Mesh Queue (Terrain)
  std::sort(m_meshQueue.begin(), m_meshQueue.end(),
            [](const MeshDrawCommand &a, const MeshDrawCommand &b) {
              return a.mesh < b.mesh;
            });

  const Mesh *currentMesh = nullptr;

  auto flushMeshBatch = [&]() {
    if (instanceData.empty() || !currentMesh)
      return;

    glNamedBufferSubData(m_instanceSSBO, 0,
                         instanceData.size() * sizeof(glm::mat4),
                         instanceData.data());

    ctx.shader.setBool("u_HasAnimation", false);
    const_cast<Mesh *>(currentMesh)->drawInstanced(ctx, instanceData.size());

    instanceData.clear();
  };

  for (const auto &cmd : m_meshQueue) {
    if (cmd.mesh != currentMesh) {
      flushMeshBatch();
      currentMesh = cmd.mesh;
    }

    if (instanceData.size() >= MAX_INSTANCES) {
      flushMeshBatch();
    }

    instanceData.push_back(cmd.transform);
  }
  flushMeshBatch();
}
