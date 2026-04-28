#include "ui/damage_text_manager.hpp"
#include "resource/shader_manager.hpp"
#include <glm/gtc/matrix_transform.hpp>

DamageTextManager::DamageTextManager() { _setup_buffers(); }

DamageTextManager::~DamageTextManager() {
  glDeleteBuffers(1, &m_vbo);
  glDeleteVertexArrays(1, &m_vao);
}

void DamageTextManager::init(const BitmapFont *font) { m_font = font; }

void DamageTextManager::addText(glm::vec3 pos, float damage, bool isCritical) {
  DamageText dt;
  dt.worldPos = pos + glm::vec3(0.0f, 1.0f, 0.0f); // offset slightly up
  dt.text = std::to_string(static_cast<int>(damage));
  dt.color = isCritical ? glm::vec4(1.0f, 0.8f, 0.0f, 1.0f) // Gold for crits
                        : glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
  dt.maxLifetime = isCritical ? 1.0f : 0.8f;
  dt.lifetime = 0.0f;
  dt.scale = isCritical ? 0.35f : 0.15f;
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

void DamageTextManager::render(const Camera &camera, int windowWidth,
                               int windowHeight) {
  if (m_texts.empty() || !m_font)
    return;

  Shader &shader = ShaderManager::get(ShaderType::UI);
  shader.use();

  float virtualHeight = 40.0f;
  float aspect = (float)windowWidth / (float)windowHeight;
  float virtualWidth = virtualHeight * aspect;

  glm::mat4 projection = glm::ortho(0.0f, virtualWidth, virtualHeight, 0.0f);
  shader.setMat4("u_projection", projection);

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glBindVertexArray(m_vao);

  shader.setBool("u_hasTexture", true);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_font->getTexID());
  shader.setInt("u_icon", 0);

  glm::mat4 viewProj = camera.getProjectionMatrix() * camera.getViewMatrix();

  for (const auto &dt : m_texts) {
    glm::vec4 clipPos = viewProj * glm::vec4(dt.worldPos, 1.0f);
    if (clipPos.w <= 0.0f)
      continue;

    glm::vec3 ndcPos = glm::vec3(clipPos) / clipPos.w;
    glm::vec2 screenPos =
        glm::vec2((ndcPos.x + 1.0f) / 2.0f, (1.0f - ndcPos.y) / 2.0f);

    float x = screenPos.x * virtualWidth;
    float y = screenPos.y * virtualHeight;

    float alpha = 1.0f - (dt.lifetime / dt.maxLifetime);
    glm::vec4 color = dt.color;
    color.a *= alpha;
    shader.setVec4("u_color", color);

    // Center text
    float textWidth = 0.0f;
    for (char c : dt.text) {
      textWidth += m_font->getCharacter(c).advance * dt.scale;
    }
    x -= textWidth / 2.0f;

    for (char c : dt.text) {
      const Character &ch = m_font->getCharacter(c);
      float w = ch.size.x * dt.scale;
      float h = ch.size.y * dt.scale;

      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, glm::vec3(x, y, 0.0f));
      model = glm::scale(model, glm::vec3(w, h, 1.0f));

      shader.setMat4("u_model", model);
      shader.setVec2("u_uv_min", ch.uvMin);
      shader.setVec2("u_uv_max", ch.uvMax);

      glDrawArrays(GL_TRIANGLES, 0, 6);
      x += ch.advance * dt.scale;
    }
  }

  shader.setVec2("u_uv_min", glm::vec2(0.0f, 0.0f));
  shader.setVec2("u_uv_max", glm::vec2(1.0f, 1.0f));
  glEnable(GL_DEPTH_TEST);
}

void DamageTextManager::_setup_buffers() {
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
