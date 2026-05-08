#pragma once

#include "graphics/camera.hpp"
#include "ui/font.hpp"
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct DamageText {
  glm::vec3 worldPos;
  std::string text;
  float lifetime = 0.0f;
  float maxLifetime = 1.0f;
  glm::vec4 color;
  float scale = 0.15f;
  bool isPlayerDamage = false;
};

class DamageTextManager {
private:
  std::vector<DamageText> m_texts;
  const BitmapFont *m_font = nullptr;
  GLuint m_vao = 0, m_vbo = 0;

  void _setupBuffers();

public:
  DamageTextManager();
  ~DamageTextManager();

  void init(const BitmapFont *font);
  void addText(glm::vec3 pos, float damage, bool is_critical,
               bool is_player_damage = false);
  void update(float dt);

  void render(const Camera &camera, int window_width, int window_height);
};
