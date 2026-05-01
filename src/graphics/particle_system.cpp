#include "graphics/particle_system.hpp"

#include <random>

ParticleSystem::ParticleSystem(uint32_t maxParticles)
    : m_poolIndex(maxParticles - 1) {
  m_particlePool.resize(maxParticles);
}

ParticleSystem::~ParticleSystem() {
  if (m_fence) {
    glDeleteSync(m_fence);
  }
  if (m_ssboMapped) {
    glUnmapNamedBuffer(m_ssbo);
  }
  if (m_ssbo) {
    glDeleteBuffers(1, &m_ssbo);
  }
  if (m_quadVAO) {
    glDeleteVertexArrays(1, &m_quadVAO);
  }
  if (m_quadVBO) {
    glDeleteBuffers(1, &m_quadVBO);
  }
}

void ParticleSystem::setup() {
  // Setup shader
  m_shader =
      std::make_shared<Shader>(ASSETS_PATH "/shaders/particle.vert.glsl",
                               ASSETS_PATH "/shaders/particle.frag.glsl");
  m_shader->use();
  m_shader->define("u_View");
  m_shader->define("u_Projection");

  // Setup basic quad VAO
  float vertices[] = {
      -0.5f, -0.5f, 0.5f,  -0.5f, 0.5f,  0.5f,
      0.5f,  0.5f,  -0.5f, 0.5f,  -0.5f, -0.5f,
  };

  glCreateVertexArrays(1, &m_quadVAO);
  glCreateBuffers(1, &m_quadVBO);
  glNamedBufferData(m_quadVBO, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glEnableVertexArrayAttrib(m_quadVAO, 0);
  glVertexArrayAttribFormat(m_quadVAO, 0, 2, GL_FLOAT, GL_FALSE, 0);
  glVertexArrayVertexBuffer(m_quadVAO, 0, m_quadVBO, 0, 2 * sizeof(float));
  glVertexArrayAttribBinding(m_quadVAO, 0, 0);

  // Setup SSBO
  GLbitfield flags =
      GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
  glCreateBuffers(1, &m_ssbo);
  glNamedBufferStorage(m_ssbo, m_particlePool.size() * sizeof(ParticleGPUData),
                       nullptr, flags);
  m_ssboMapped = (ParticleGPUData *)glMapNamedBufferRange(
      m_ssbo, 0, m_particlePool.size() * sizeof(ParticleGPUData), flags);
}

void ParticleSystem::update(double delta_time) {
  for (auto &particle : m_particlePool) {
    if (!particle.active)
      continue;

    if (particle.lifeRemaining <= 0.0f) {
      particle.active = false;
      continue;
    }

    particle.lifeRemaining -= static_cast<float>(delta_time);
    particle.position += particle.velocity * static_cast<float>(delta_time);
  }
}

void ParticleSystem::render(const RenderContext &ctx) {
  if (m_fence) {
    glClientWaitSync(m_fence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
    glDeleteSync(m_fence);
    m_fence = nullptr;
  }

  uint32_t activeParticleCount = 0;
  for (const auto &particle : m_particlePool) {
    if (!particle.active)
      continue;

    float life =
        particle.lifeRemaining / particle.lifeTime; // 1.0 (new) -> 0.0 (dead)
    float t = 1.0f - life;                          // 0.0 (new) -> 1.0 (dead)

    // Interpolate size and color
    float size = glm::mix(particle.sizeBegin, particle.sizeEnd, t);
    glm::vec4 color = glm::mix(particle.colorBegin, particle.colorEnd, t);

    m_ssboMapped[activeParticleCount].positionSize =
        glm::vec4(particle.position, size);
    m_ssboMapped[activeParticleCount].directionStretch =
        glm::vec4(particle.direction, particle.stretch);
    m_ssboMapped[activeParticleCount].color = color;

    activeParticleCount++;
  }

  if (activeParticleCount == 0)
    return;

  m_fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

  m_shader->use();
  m_shader->setMat4("u_View", ctx.camera.getViewMatrix());
  m_shader->setMat4("u_Projection", ctx.camera.getProjectionMatrix());

  // Disable depth writing so particles don't occlude each other weirdly
  glDepthMask(GL_FALSE);

  // Enable additive blending for glowing effect
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);

  glBindVertexArray(m_quadVAO);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_ssbo);

  glDrawArraysInstanced(GL_TRIANGLES, 0, 6, activeParticleCount);

  // Restore state
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_TRUE);
}

void ParticleSystem::emit(const ParticleProps &particleProps) {
  Particle &particle = m_particlePool[m_poolIndex];
  particle.active = true;
  particle.position = particleProps.position;

  // Add random variation to velocity
  particle.velocity = particleProps.velocity;
  particle.velocity.x +=
      particleProps.velocityVariation.x * (_randomFloat() - 0.5f);
  particle.velocity.y +=
      particleProps.velocityVariation.y * (_randomFloat() - 0.5f);
  particle.velocity.z +=
      particleProps.velocityVariation.z * (_randomFloat() - 0.5f);

  particle.colorBegin = particleProps.colorBegin;
  particle.colorEnd = particleProps.colorEnd;

  particle.lifeTime = particleProps.lifeTime;
  particle.lifeRemaining = particleProps.lifeTime;

  particle.direction = particleProps.direction;
  if (glm::length(particle.direction) < 0.001f) {
    particle.direction = glm::vec3(0.0f, 1.0f, 0.0f);
  } else {
    particle.direction = glm::normalize(particle.direction);
  }

  particle.sizeBegin = particleProps.sizeBegin +
                       particleProps.sizeVariation * (_randomFloat() - 0.5f);
  particle.sizeEnd = particleProps.sizeEnd;
  particle.stretch =
      glm::max(1.0f, particleProps.stretch + particleProps.stretchVariation *
                                                 (_randomFloat() - 0.5f));

  m_poolIndex =
      (m_poolIndex == 0) ? m_particlePool.size() - 1 : m_poolIndex - 1;
}

uint32_t ParticleSystem::_randomUint() {
  static std::mt19937 engine(std::random_device{}());
  return engine();
}

float ParticleSystem::_randomFloat() {
  return (float)_randomUint() / (float)std::numeric_limits<uint32_t>::max();
}
