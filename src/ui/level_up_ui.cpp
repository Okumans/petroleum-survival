#include "ui/level_up_ui.hpp"
#include "game/game.hpp"

LevelUpUI::LevelUpUI(UIManager &ui_manager) : m_uiManager(ui_manager) {}

LevelUpUI::~LevelUpUI() {}

void LevelUpUI::show(const std::vector<Upgrade> &upgrades, Game *game) {
  m_upgrades = upgrades;
  m_game = game;
  m_selectedIndex = -1;

  _createBackground();
  _createUpgradeButtons();
}

void LevelUpUI::hide() {
  // Remove all level-up UI elements
  // This would require tracking element names and removing them from UIManager
  // For now, we just reset state
  m_selectedIndex = -1;
  m_upgrades.clear();
  m_game = nullptr;
}

void LevelUpUI::selectUpgrade(int index) {
  if (index >= 0 && index < static_cast<int>(m_upgrades.size())) {
    m_selectedIndex = index;
  }
}

void LevelUpUI::applySelection() {
  if (m_selectedIndex >= 0 &&
      m_selectedIndex < static_cast<int>(m_upgrades.size()) && m_game) {
    m_upgrades[m_selectedIndex].apply(*m_game);
    hide();

    if (m_onUpgradeApplied) {
      m_onUpgradeApplied();
    }
  }
}

void LevelUpUI::cancel() {
  hide();
  if (m_onUpgradeApplied) {
    m_onUpgradeApplied();
  }
}

void LevelUpUI::_createBackground() {
  // Create semi-transparent overlay
  UIHitbox screenBounds = {0.0f, 0.0f, 1.0f, 1.0f};
  m_uiManager.addStaticElement("level_up_overlay", screenBounds,
                               glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
}

void LevelUpUI::_createUpgradeButtons() {
  if (m_upgrades.size() > 3) {
    m_upgrades.resize(3);
  }

  // Create 3 buttons in a row centered on screen
  float buttonWidth = 0.25f;
  float buttonHeight = 0.3f;
  float startX = (1.0f - (buttonWidth * 3 + 0.05f * 2)) * 0.5f;
  float startY = (1.0f - buttonHeight) * 0.5f;
  float spacing = buttonWidth + 0.05f;

  for (size_t i = 0; i < m_upgrades.size(); ++i) {
    float x = startX + (i * spacing);
    float y = startY;

    UIHitbox bounds = {x, y, buttonWidth, buttonHeight};
    std::string buttonName = "upgrade_btn_" + std::to_string(i);

    // Create button callback
    int index = static_cast<int>(i);
    m_uiManager.addInteractiveElement(
        buttonName, bounds, glm::vec4(0.2f, 0.2f, 0.8f, 0.8f), [this, index]() {
          this->selectUpgrade(index);
          this->applySelection();
        });

    // TODO: Add text labels with upgrade titles and descriptions
    // This requires accessing the UI font from the app context
  }
}
