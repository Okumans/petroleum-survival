#include "ui/damage_text_manager.hpp"
#include "resource/shader_manager.hpp"
#include <glm/gtc/matrix_transform.hpp>

DamageTextManager::DamageTextManager() { _setupBuffers(); }

DamageTextManager::~DamageTextManager() {
  glDeleteBuffers(1, &m_vbo);
  glDeleteVertexArrays(1, &m_vao);
}

void DamageTextManager::init(const BitmapFont *font) { m_font = font; }

void DamageTextManager::addText(glm::vec3 pos, float damage, bool is_critical,
                                bool is_player_damage) {
  DamageText dt;
  dt.worldPos = pos + glm::vec3(0.0f, 1.0f, 0.0f); // offset slightly up
  dt.text = std::to_string(static_cast<int>(damage));

  if (is_player_damage) {
    dt.color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); // Red for player damage
  } else {
    dt.color = is_critical ? glm::vec4(1.0f, 0.8f, 0.0f, 1.0f) // Gold for crits
                           : glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
  }

  dt.maxLifetime = is_critical ? 1.0f : 0.8f;
  dt.lifetime = 0.0f;
  dt.scale = is_critical ? 0.35f : 0.15f;
  dt.isPlayerDamage = is_player_damage;
  m_texts.push_back(dt);
}

void DamageTextManager::update(float dt) {
  for (auto &t : m_texts) {
    t.lifetime += dt;
    t.worldPos.y += dt * 2.0f; // float upwards
  }

  std::erase_if(
      m_texts, [](const DamageText &t) { return t.lifetime >= t.maxLifetime; });
}

void DamageTextManager::render(const Camera &camera, int window_width,
                               int window_height) {
  if (m_texts.empty() || !m_font)
    return;

  Shader &shader = ShaderManager::get(ShaderType::UI);
  shader.use();

  float virtual_height = 40.0f;
  float aspect = (float)window_width / (float)window_height;
  float virtual_width = virtual_height * aspect;

  glm::mat4 projection = glm::ortho(0.0f, virtual_width, virtual_height, 0.0f);
  shader.setMat4("u_projection", projection);

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glBindVertexArray(m_vao);

  shader.setBool("u_hasTexture", true);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_font->getTexID());
  shader.setInt("u_icon", 0);

  glm::mat4 view_proj = camera.getProjectionMatrix() * camera.getViewMatrix();

  for (const auto &dt : m_texts) {
    glm::vec4 clip_pos = view_proj * glm::vec4(dt.worldPos, 1.0f);
    if (clip_pos.w <= 0.0f)
      continue;

    glm::vec3 ndc_pos = glm::vec3(clip_pos) / clip_pos.w;
    glm::vec2 screen_pos =
        glm::vec2((ndc_pos.x + 1.0f) / 2.0f, (1.0f - ndc_pos.y) / 2.0f);

    float start_x = screen_pos.x * virtual_width;
    float start_y = screen_pos.y * virtual_height;

    float t = dt.lifetime / dt.maxLifetime;

    // Animation phases
    float alpha = 1.0f;
    float current_scale = dt.scale;

    // Quick fade in and scale up (0% - 20%)
    if (t < 0.2f) {
      float p = t / 0.2f;
      alpha = p;
      current_scale = dt.scale * glm::mix(0.5f, 1.3f, p);
    }
    // Scale back down (20% - 40%)
    else if (t < 0.4f) {
      float p = (t - 0.2f) / 0.2f;
      current_scale = dt.scale * glm::mix(1.3f, 1.0f, p);
    }
    // Fade out (60% - 100%)
    else if (t > 0.6f) {
      float p = (t - 0.6f) / 0.4f;
      alpha = 1.0f - p;
    }

    auto draw_string = [&](float x, float y, const glm::vec4 &color) {
      shader.setVec4("u_color", color);
      float text_width = 0.0f;
      for (char c : dt.text) {
        text_width += m_font->getCharacter(c).advance * current_scale;
      }
      float cursor_x = x - text_width / 2.0f;

      for (char c : dt.text) {
        const Character &ch = m_font->getCharacter(c);
        float w = ch.size.x * current_scale;
        float h = ch.size.y * current_scale;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(cursor_x, y, 0.0f));
        model = glm::scale(model, glm::vec3(w, h, 1.0f));

        shader.setMat4("u_model", model);
        shader.setVec2("u_uv_min", ch.uvMin);
        shader.setVec2("u_uv_max", ch.uvMax);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        cursor_x += ch.advance * current_scale;
      }
    };

    // Draw border (dark shadow)
    glm::vec4 border_color = glm::vec4(0.0f, 0.0f, 0.0f, alpha * 0.8f);
    float offset = 0.05f;
    draw_string(start_x + offset, start_y + offset, border_color);

    // Draw main text
    glm::vec4 main_color = dt.color;
    main_color.a *= alpha;
    draw_string(start_x, start_y, main_color);
  }

  shader.setVec2("u_uv_min", glm::vec2(0.0f, 0.0f));
  shader.setVec2("u_uv_max", glm::vec2(1.0f, 1.0f));
  glEnable(GL_DEPTH_TEST);
}

void DamageTextManager::_setupBuffers() {
  struct UIVertex {
    glm::vec2 pos;
    glm::vec2 uv;
  };
  UIVertex vertices[] = {
      {{0.0f, 0.0f}, {0.0f, 0.0f}}, {{0.0f, 1.0f}, {0.0f, 1.0f}},
      {{1.0f, 1.0f}, {1.0f, 1.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}},
      {{1.0f, 1.0f}, {1.0f, 1.0f}}, {{1.0f, 0.0f}, {1.0f, 0.0f}}};

  glCreateBuffers(1, &m_vbo);
  glNamedBufferStorage(m_vbo, sizeof(vertices), vertices, 0);

  glCreateVertexArrays(1, &m_vao);

  glEnableVertexArrayAttrib(m_vao, 0);
  glVertexArrayAttribFormat(m_vao, 0, 2, GL_FLOAT, GL_FALSE,
                            offsetof(UIVertex, pos));
  glVertexArrayAttribBinding(m_vao, 0, 0);

  glEnableVertexArrayAttrib(m_vao, 1);
  glVertexArrayAttribFormat(m_vao, 1, 2, GL_FLOAT, GL_FALSE,
                            offsetof(UIVertex, uv));
  glVertexArrayAttribBinding(m_vao, 1, 0);

  glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, sizeof(UIVertex));
}
