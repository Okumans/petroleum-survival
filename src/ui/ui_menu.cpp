#include "ui/ui_menu.hpp"
#include "game/game.hpp"
#include "ui/font.hpp"
#include <GLFW/glfw3.h>
#include <cmath>
#include <string>

MenuUI::MenuUI(UIManager &ui_manager, const BitmapFont &font)
    : m_uiManager(ui_manager), m_font(font) {}

MenuUI::~MenuUI() {}

void MenuUI::setup(Game &game) {
  _setupStartMenu(game);
}

void MenuUI::update(const Game &game, float virtualWidth) {
  _updateStartMenu(game, virtualWidth);
}

void MenuUI::_setupStartMenu(Game &game) {
  m_uiManager.addTextElement("start_title", {0.0f, 10.0f, 0.0f, 0.0f},
                             "VAMPIRE SURVIVOR", m_font,
                             {1.0f, 0.2f, 0.2f, 1.0f}, 0.35f);

  // Start Button
  m_uiManager.addInteractiveElement("menu_start_btn", {0.0f, 20.0f, 15.0f, 3.5f},
                                    {0.2f, 0.2f, 0.8f, 0.8f}, [&game]() {
                                      game.startGame();
                                    });
  m_uiManager.addTextElement("menu_start_text", {0.0f, 20.75f, 0.0f, 0.0f},
                             "START GAME", m_font, {1.0f, 1.0f, 1.0f, 1.0f}, 0.12f);

  // Settings Button (Placeholder for now)
  m_uiManager.addInteractiveElement("menu_settings_btn", {0.0f, 25.0f, 15.0f, 3.5f},
                                    {0.3f, 0.3f, 0.3f, 0.8f}, []() {
                                      // TODO: Settings menu
                                    });
  m_uiManager.addTextElement("menu_settings_text", {0.0f, 25.75f, 0.0f, 0.0f},
                             "SETTINGS", m_font, {0.8f, 0.8f, 0.8f, 1.0f}, 0.12f);

  // Quit Button
  m_uiManager.addInteractiveElement("menu_quit_btn", {0.0f, 30.0f, 15.0f, 3.5f},
                                    {0.5f, 0.2f, 0.2f, 0.8f}, []() {
                                      GLFWwindow* window = glfwGetCurrentContext();
                                      if (window) glfwSetWindowShouldClose(window, GLFW_TRUE);
                                    });
  m_uiManager.addTextElement("menu_quit_text", {0.0f, 30.75f, 0.0f, 0.0f},
                             "QUIT", m_font, {1.0f, 0.8f, 0.8f, 1.0f}, 0.12f);
}

void MenuUI::_updateStartMenu(const Game &game, float virtualWidth) {
  GameState state = game.getState();
  bool isStartMenu = (state == GameState::START_MENU);

  m_uiManager.getElement("start_title")->visible = isStartMenu;
  m_uiManager.getElement("menu_start_btn")->visible = isStartMenu;
  m_uiManager.getElement("menu_start_text")->visible = isStartMenu;
  m_uiManager.getElement("menu_settings_btn")->visible = isStartMenu;
  m_uiManager.getElement("menu_settings_text")->visible = isStartMenu;
  m_uiManager.getElement("menu_quit_btn")->visible = isStartMenu;
  m_uiManager.getElement("menu_quit_text")->visible = isStartMenu;

  if (isStartMenu) {
    float center_x = virtualWidth / 2.0f;
    float time = static_cast<float>(glfwGetTime());

    // Title Animation (Breathing/Scaling)
    if (auto *title = dynamic_cast<TextElement *>(
            m_uiManager.getElement("start_title"))) {
      float pulse = 1.0f + 0.05f * std::sin(time * 2.0f);
      title->scale = 0.35f * pulse;
      title->bounds.x =
          center_x - m_font.getTextWidth(title->text, title->scale) / 2.0f;
      // Also slightly oscillate Y for floating effect
      title->bounds.y = 8.0f + 1.0f * std::sin(time * 1.5f);
    }

    // Centering buttons
    auto centerButton = [&](const std::string& btnId, const std::string& textId) {
        auto* btn = m_uiManager.getElement(btnId);
        auto* txt = dynamic_cast<TextElement*>(m_uiManager.getElement(textId));
        if (btn && txt) {
            btn->bounds.x = center_x - btn->bounds.w / 2.0f;
            txt->bounds.x = center_x - m_font.getTextWidth(txt->text, txt->scale) / 2.0f;
        }
    };

    centerButton("menu_start_btn", "menu_start_text");
    centerButton("menu_settings_btn", "menu_settings_text");
    centerButton("menu_quit_btn", "menu_quit_text");
  }
}
