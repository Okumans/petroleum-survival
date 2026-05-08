#include "graphics/particle_system.hpp"

#include <random>

ParticleSystem::ParticleSystem(uint32_t max_particles)
    : m_poolIndex(max_particles - 1) {
  m_particlePool.resize(max_particles);
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

  uint32_t active_particle_count = 0;
  for (const auto &particle : m_particlePool) {
    if (!particle.active)
      continue;

    float life =
        particle.lifeRemaining / particle.lifeTime; // 1.0 (new) -> 0.0 (dead)
    float t = 1.0f - life;                          // 0.0 (new) -> 1.0 (dead)

    // Interpolate size and color
    float size = glm::mix(particle.sizeBegin, particle.sizeEnd, t);
    glm::vec4 color = glm::mix(particle.colorBegin, particle.colorEnd, t);

    m_ssboMapped[active_particle_count].positionSize =
        glm::vec4(particle.position, size);
    m_ssboMapped[active_particle_count].directionStretch =
        glm::vec4(particle.direction, particle.stretch);
    m_ssboMapped[active_particle_count].color = color;

    active_particle_count++;
  }

  if (active_particle_count == 0)
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

  glDrawArraysInstanced(GL_TRIANGLES, 0, 6, active_particle_count);

  // Restore state
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_TRUE);
}

void ParticleSystem::emit(const ParticleProps &particle_props) {
  Particle &particle = m_particlePool[m_poolIndex];
  particle.active = true;
  particle.position = particle_props.position;

  // Add random variation to velocity
  particle.velocity = particle_props.velocity;
  particle.velocity.x +=
      particle_props.velocityVariation.x * (_randomFloat() - 0.5f);
  particle.velocity.y +=
      particle_props.velocityVariation.y * (_randomFloat() - 0.5f);
  particle.velocity.z +=
      particle_props.velocityVariation.z * (_randomFloat() - 0.5f);

  particle.colorBegin = particle_props.colorBegin;
  particle.colorEnd = particle_props.colorEnd;

  particle.lifeTime = particle_props.lifeTime;
  particle.lifeRemaining = particle_props.lifeTime;

  particle.direction = particle_props.direction;
  if (glm::length(particle.direction) < 0.001f) {
    particle.direction = glm::vec3(0.0f, 1.0f, 0.0f);
  } else {
    particle.direction = glm::normalize(particle.direction);
  }

  particle.sizeBegin = particle_props.sizeBegin +
                       particle_props.sizeVariation * (_randomFloat() - 0.5f);
  particle.sizeEnd = particle_props.sizeEnd;
  particle.stretch =
      glm::max(1.0f, particle_props.stretch + particle_props.stretchVariation *
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
