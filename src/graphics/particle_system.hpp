#pragma once

#include "graphics/render_context.hpp"
#include "graphics/shader.hpp"

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

struct ParticleProps {
  glm::vec3 position;
  glm::vec3 velocity;
  glm::vec3 velocityVariation{0.0f};
  glm::vec4 colorBegin;
  glm::vec4 colorEnd;
  float sizeBegin;
  float sizeEnd;
  float sizeVariation{0.0f};
  float lifeTime;
};

class ParticleSystem {
private:
  struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 colorBegin, colorEnd;
    float sizeBegin, sizeEnd;
    float lifeTime = 0.0f;
    float lifeRemaining = 0.0f;

    bool active = false;
  };

  struct ParticleGPUData {
    glm::vec4 positionAndSize;
    glm::vec4 color;
  };

  std::vector<Particle> m_particlePool;
  uint32_t m_poolIndex = 0;

  GLuint m_quadVAO = 0;
  GLuint m_quadVBO = 0;
  
  GLuint m_ssbo = 0;
  ParticleGPUData* m_ssboMapped = nullptr;
  GLsync m_fence = nullptr;

  std::shared_ptr<Shader> m_shader;

public:
  ParticleSystem(uint32_t maxParticles = 10000);
  ~ParticleSystem();

  void setup();
  void update(double delta_time);
  void render(const RenderContext &ctx);

  void emit(const ParticleProps &particleProps);

private:
  uint32_t _randomUint();
  float _randomFloat();
};
