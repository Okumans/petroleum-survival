#include "ui/ui_level_up.hpp"
#include "game/game.hpp"
#include "resource/texture_manager.hpp"
#include "ui/font.hpp"
#include <cmath>
#include <string>

LevelUI::LevelUI(UIManager &ui_manager, const BitmapFont &font)
    : m_uiManager(ui_manager), m_font(font) {}

LevelUI::~LevelUI() {}

void LevelUI::setup(Game &game) {
  _setupHUD();
  _setupLevelUpOverlay(game);
  _setupGameOverOverlay(game);
}

void LevelUI::update(const Game &game, float virtualWidth) {
  _updateHUD(game, virtualWidth);
  _updateLevelUpOverlay(game, virtualWidth);
  _updateGameOverOverlay(game, virtualWidth);
}

void LevelUI::_setupHUD() {
  m_uiManager.addStaticElement("exp_bg", {0.0f, 0.0f, 100.0f, 1.0f},
                               {0.1f, 0.1f, 0.1f, 1.0f});
  m_uiManager.addStaticElement("exp_fill", {0.0f, 0.0f, 0.0f, 1.0f},
                               {0.2f, 0.5f, 1.0f, 0.8f});
  m_uiManager.addStaticElement("health_bg", {0.8f, 2.0f, 2.0f, 20.0f},
                               {0.1f, 0.08f, 0.08f, 0.95f});
  m_uiManager.addStaticElement("health_fill", {0.8f, 2.0f, 2.0f, 20.0f},
                               {0.78f, 0.15f, 0.15f, 0.95f});
  m_uiManager.addTextElement("health_text", {0.75f, 22.6f, 0.0f, 0.0f}, "HP",
                             m_font, {1.0f, 0.9f, 0.9f, 1.0f}, 0.08f);
  m_uiManager.addTextElement("level_text", {0.0f, 1.5f, 0.0f, 0.0f}, "LV 1",
                             m_font, {1.0f, 1.0f, 1.0f, 1.0f}, 0.1f);
}

void LevelUI::_setupLevelUpOverlay(Game &game) {
  // Level Up Overlay
  m_uiManager.addStaticElement("level_up_bg", {0.0f, 5.0f, 52.0f, 30.0f},
                               {0.1f, 0.1f, 0.1f, 0.9f});
  m_uiManager.addTextElement("level_up_title", {0.0f, 7.0f, 0.0f, 0.0f},
                             "LEVEL UP!", m_font, {1.0f, 0.8f, 0.0f, 1.0f},
                             0.2f);
  m_uiManager.addTextElement("level_up_hint", {0.0f, 15.0f, 0.0f, 0.0f},
                             "Select an upgrade:", m_font,
                             {0.7f, 0.7f, 0.7f, 1.0f}, 0.1f);

  // Level Up Option Buttons (Title + Wrapping Description)
  for (int i = 0; i < 3; ++i) {
    float btnX = 0.0f; // Handled dynamically in update loop
    float btnY = 18.0f;
    float btnW = 16.0f;
    float btnH = 6.5f;

    std::string btnName = "upgrade_option_" + std::to_string(i);
    std::string iconName = "upgrade_option_icon_" + std::to_string(i);
    std::string titleName = "upgrade_option_title_" + std::to_string(i);
    std::string descName = "upgrade_option_desc_" + std::to_string(i);

    // Interactive Box
    m_uiManager.addInteractiveElement(btnName, {btnX, btnY, btnW, btnH},
                                      {0.2f, 0.2f, 0.8f, 0.8f}, [&game, i]() {
                                        game.selectLevelUpOption(i);
                                        game.confirmLevelUpSelection();
                                      });

    // Option Icon
    GLuint defaultIcon =
        TextureManager::get(TextureName("icon_no_icon")).getTexID();
    m_uiManager.addStaticElement(
        iconName, {btnX + (btnW - 2.8f) * 0.5f, btnY + 0.3f, 2.8f, 2.8f},
        defaultIcon);

    // Option Title (Standard TextElement)
    m_uiManager.addTextElement(titleName,
                               {btnX + 0.5f, btnY + 3.25f, 0.0f, 0.0f},
                               "Option " + std::to_string(i + 1), m_font,
                               {1.0f, 1.0f, 1.0f, 1.0f}, 0.08f);

    // Option Description (Wrapping TextBoxElement)
    m_uiManager.addTextBoxElement(
        descName, {btnX + 0.6f, btnY + 4.15f, btnW - 1.2f, btnH - 4.55f},
        "Description...", m_font, {1.0f, 1.0f, 1.0f, 1.0f}, 0.05f);
  }

  // Level Up Skip Button
  m_uiManager.addInteractiveElement("level_up_btn", {0.0f, 25.0f, 10.0f, 3.0f},
                                    {0.2f, 0.6f, 0.2f, 1.0f},
                                    [&game]() { game.skipLevelUp(); });
  m_uiManager.addTextElement("level_up_btn_text", {0.0f, 25.5f, 0.0f, 0.0f},
                             "SKIP", m_font, {1.0f, 1.0f, 1.0f, 1.0f}, 0.1f);
}

void LevelUI::_updateHUD(const Game &game, float virtualWidth) {
  GameState state = game.getState();
  bool isPlaying = state == GameState::PLAYING || state == GameState::LEVEL_UP;

  m_uiManager.getElement("exp_bg")->visible = isPlaying;
  m_uiManager.getElement("exp_fill")->visible = isPlaying;
  m_uiManager.getElement("health_bg")->visible = isPlaying;
  m_uiManager.getElement("health_fill")->visible = isPlaying;
  m_uiManager.getElement("health_text")->visible = isPlaying;
  m_uiManager.getElement("level_text")->visible = isPlaying;

  if (isPlaying) {
    m_uiManager.getElement("exp_bg")->bounds.w = virtualWidth;
    float pct = (float)game.getCurrentExp() / (float)game.getExpToNextLevel();
    m_uiManager.getElement("exp_fill")->bounds.w = pct * virtualWidth;

    if (auto *levelText =
            dynamic_cast<TextElement *>(m_uiManager.getElement("level_text"))) {
      levelText->text = "LV " + std::to_string(game.getCurrentLevel());
      levelText->bounds.x =
          virtualWidth -
          m_font.getTextWidth(levelText->text, levelText->scale) - 1.0f;
    }

    _updateHealthBar(game, virtualWidth);
  }
}

void LevelUI::_updateHealthBar(const Game &game, float virtualWidth) {
  (void)virtualWidth;

  const Player *player = game.getPlayer();
  if (!player) {
    return;
  }

  float maxHealth = std::max(1.0f, player->getMaxHealth());
  float healthRatio = glm::clamp(player->getHealth() / maxHealth, 0.0f, 1.0f);

  auto *bg = m_uiManager.getElement("health_bg");
  auto *fill = m_uiManager.getElement("health_fill");
  auto *label =
      dynamic_cast<TextElement *>(m_uiManager.getElement("health_text"));
  if (!bg || !fill || !label) {
    return;
  }

  bg->bounds.x = 0.8f;
  bg->bounds.y = 17;
  bg->bounds.w = 2.0f;
  bg->bounds.h = 20.0f;

  fill->bounds.x = bg->bounds.x;
  fill->bounds.y = bg->bounds.y;
  fill->bounds.w = bg->bounds.w;
  fill->bounds.h = bg->bounds.h * healthRatio;

  label->text =
      std::to_string(static_cast<int>(std::ceil(player->getHealth()))) + " / " +
      std::to_string(static_cast<int>(std::ceil(player->getMaxHealth())));
  label->bounds.x = bg->bounds.x - 0.2f;
  label->bounds.y = bg->bounds.y + bg->bounds.h + 0.4f;
}

void LevelUI::_updateLevelUpOverlay(const Game &game, float virtualWidth) {
  GameState state = game.getState();
  bool isLevelUp = state == GameState::LEVEL_UP;

  m_uiManager.getElement("level_up_bg")->visible = isLevelUp;
  m_uiManager.getElement("level_up_title")->visible = isLevelUp;
  m_uiManager.getElement("level_up_hint")->visible = isLevelUp;
  m_uiManager.getElement("level_up_btn")->visible = isLevelUp;
  m_uiManager.getElement("level_up_btn_text")->visible = isLevelUp;

  // Toggle Visibility for Options
  for (int i = 0; i < 3; ++i) {
    std::string btnName = "upgrade_option_" + std::to_string(i);
    std::string iconName = "upgrade_option_icon_" + std::to_string(i);
    std::string titleName = "upgrade_option_title_" + std::to_string(i);
    std::string descName = "upgrade_option_desc_" + std::to_string(i);

    if (auto *btn = m_uiManager.getElement(btnName))
      btn->visible = isLevelUp;
    if (auto *icon = m_uiManager.getElement(iconName))
      icon->visible = isLevelUp;
    if (auto *titleTxt = m_uiManager.getElement(titleName))
      titleTxt->visible = isLevelUp;
    if (auto *descTxt = m_uiManager.getElement(descName))
      descTxt->visible = isLevelUp;
  }

  if (isLevelUp) {
    float center_x = virtualWidth / 2.0f;

    // Center Background & Main Title
    m_uiManager.getElement("level_up_bg")->bounds.x = center_x - 26.0f;
    if (auto *title = dynamic_cast<TextElement *>(
            m_uiManager.getElement("level_up_title"))) {
      title->bounds.x =
          center_x - m_font.getTextWidth(title->text, title->scale) / 2.0f;
    }

    // Update Upgrade Options (Buttons, Titles, Descriptions)
    const auto &upgrades = game.getLevelUpCandidates();
    float btn_w = 16.0f;
    float spacing = 2.0f;
    float total_w = (3.0f * btn_w) + (2.0f * spacing);
    float start_x = center_x - (total_w / 2.0f);

    for (int i = 0; i < 3; ++i) {
      std::string btnName = "upgrade_option_" + std::to_string(i);
      std::string iconName = "upgrade_option_icon_" + std::to_string(i);
      std::string titleName = "upgrade_option_title_" + std::to_string(i);
      std::string descName = "upgrade_option_desc_" + std::to_string(i);

      auto *btn = m_uiManager.getElement(btnName);
      auto *icon =
          dynamic_cast<StaticElement *>(m_uiManager.getElement(iconName));
      auto *titleTxt =
          dynamic_cast<TextElement *>(m_uiManager.getElement(titleName));
      auto *descTxt =
          dynamic_cast<TextBoxElement *>(m_uiManager.getElement(descName));

      if (btn) {
        // Position Box
        btn->bounds.x = start_x + (i * (btn_w + spacing));
        btn->bounds.w = btn_w;

        if (i < static_cast<int>(upgrades.size())) {
          glm::vec4 highlightColor = (game.getLevelUpSelection() == i)
                                         ? glm::vec4{1.0f, 1.0f, 0.0f, 1.0f}
                                         : glm::vec4{1.0f, 1.0f, 1.0f, 1.0f};

          if (icon) {
            icon->bounds.w = 2.8f;
            icon->bounds.h = 2.8f;
            icon->bounds.x = btn->bounds.x + (btn_w - icon->bounds.w) * 0.5f;
            icon->bounds.y = btn->bounds.y + 0.3f;
            icon->textureID =
                TextureManager::get(TextureName(upgrades[i].iconName))
                    .getTexID();
            icon->hasTexture = true;
            icon->color = highlightColor;
          }

          // Position & Format Option Title
          if (titleTxt) {
            titleTxt->text = upgrades[i].title;
            titleTxt->color = highlightColor;
            float tw = m_font.getTextWidth(titleTxt->text, titleTxt->scale);
            titleTxt->bounds.x = btn->bounds.x + (btn_w - tw) * 0.5f;
            titleTxt->bounds.y = btn->bounds.y + 3.25f;
          }

          // Position & Format Option Description (TextBox)
          if (descTxt) {
            descTxt->text = upgrades[i].description;
            descTxt->color = highlightColor;
            float padding = 0.5f;
            descTxt->bounds.x = btn->bounds.x + padding;
            descTxt->bounds.w = btn_w - (padding * 2.0f);
            descTxt->bounds.y = btn->bounds.y + 4.15f;
          }
        }
      }
    }

    // Center Hint Text
    if (auto *hint = dynamic_cast<TextElement *>(
            m_uiManager.getElement("level_up_hint"))) {
      hint->text = "Select an upgrade:";
      hint->bounds.x =
          center_x - m_font.getTextWidth(hint->text, hint->scale) / 2.0f;
    }

    // Center Skip Button
    m_uiManager.getElement("level_up_btn")->bounds.x = center_x - 5.0f;
    if (auto *btnText = dynamic_cast<TextElement *>(
            m_uiManager.getElement("level_up_btn_text"))) {
      btnText->bounds.x =
          center_x - m_font.getTextWidth(btnText->text, btnText->scale) / 2.0f;
    }
  }
}

void LevelUI::_setupGameOverOverlay(Game &game) {
  m_uiManager.addStaticElement("game_over_bg", {0.0f, 0.0f, 100.0f, 100.0f},
                               {0.0f, 0.0f, 0.0f, 0.7f});
  m_uiManager.addTextElement("game_over_title", {0.0f, 15.0f, 0.0f, 0.0f},
                             "GAME OVER", m_font, {1.0f, 0.2f, 0.2f, 1.0f},
                             0.3f);
  m_uiManager.addTextElement("game_over_score", {0.0f, 20.0f, 0.0f, 0.0f},
                             "Score: 0", m_font, {1.0f, 1.0f, 1.0f, 1.0f}, 0.15f);

  m_uiManager.addInteractiveElement("game_over_retry", {0.0f, 28.0f, 12.0f, 3.5f},
                                    {0.2f, 0.5f, 0.2f, 1.0f},
                                    [&game]() { 
                                      game.reset(); 
                                      game.startGame(); 
                                    });
  m_uiManager.addTextElement("game_over_retry_text", {0.0f, 28.75f, 0.0f, 0.0f},
                             "RETRY", m_font, {1.0f, 1.0f, 1.0f, 1.0f}, 0.12f);
}

void LevelUI::_updateGameOverOverlay(const Game &game, float virtualWidth) {
  GameState state = game.getState();
  bool isGameOver = state == GameState::GAME_OVER;

  m_uiManager.getElement("game_over_bg")->visible = isGameOver;
  m_uiManager.getElement("game_over_title")->visible = isGameOver;
  m_uiManager.getElement("game_over_score")->visible = isGameOver;
  m_uiManager.getElement("game_over_retry")->visible = isGameOver;
  m_uiManager.getElement("game_over_retry_text")->visible = isGameOver;

  if (isGameOver) {
    float center_x = virtualWidth / 2.0f;

    m_uiManager.getElement("game_over_bg")->bounds.w = virtualWidth;
    m_uiManager.getElement("game_over_bg")->bounds.h = 40.0f; // virtual height

    if (auto *title = dynamic_cast<TextElement *>(
            m_uiManager.getElement("game_over_title"))) {
      title->bounds.x =
          center_x - m_font.getTextWidth(title->text, title->scale) / 2.0f;
    }

    if (auto *scoreTxt = dynamic_cast<TextElement *>(
            m_uiManager.getElement("game_over_score"))) {
      scoreTxt->text = "Score: " + std::to_string(game.getScore());
      scoreTxt->bounds.x =
          center_x - m_font.getTextWidth(scoreTxt->text, scoreTxt->scale) / 2.0f;
    }

    m_uiManager.getElement("game_over_retry")->bounds.x = center_x - 6.0f;
    if (auto *btnText = dynamic_cast<TextElement *>(
            m_uiManager.getElement("game_over_retry_text"))) {
      btnText->bounds.x =
          center_x - m_font.getTextWidth(btnText->text, btnText->scale) / 2.0f;
    }
  }
}
