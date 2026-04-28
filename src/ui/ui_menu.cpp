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
  _setupCreditsMenu();
}

void MenuUI::update(const Game &game, float virtualWidth) {
  _updateStartMenu(game, virtualWidth);
  _updateCreditsMenu(virtualWidth);
}

void MenuUI::_setupStartMenu(Game &game) {
  m_uiManager.addTextElement("start_title", {0.0f, 10.0f, 0.0f, 0.0f},
                             "VAMPIRE SURVIVOR", m_font,
                             {1.0f, 0.2f, 0.2f, 1.0f}, 0.35f);

  // Start Button
  m_uiManager.addInteractiveElement(
      "menu_start_btn", {0.0f, 20.0f, 15.0f, 3.5f}, {0.2f, 0.2f, 0.8f, 0.8f},
      [&game]() { game.startGame(); });
  m_uiManager.addTextElement("menu_start_text", {0.0f, 20.75f, 0.0f, 0.0f},
                             "START GAME", m_font, {1.0f, 1.0f, 1.0f, 1.0f},
                             0.12f);

  // Credits Button
  m_uiManager.addInteractiveElement(
      "menu_credits_btn", {0.0f, 25.0f, 15.0f, 3.5f}, {0.3f, 0.3f, 0.3f, 0.8f},
      [this]() { m_state = MenuState::CREDITS; });
  m_uiManager.addTextElement("menu_credits_text", {0.0f, 25.75f, 0.0f, 0.0f},
                             "CREDITS", m_font, {0.8f, 0.8f, 0.8f, 1.0f},
                             0.12f);

  // Quit Button
  m_uiManager.addInteractiveElement(
      "menu_quit_btn", {0.0f, 30.0f, 15.0f, 3.5f}, {0.5f, 0.2f, 0.2f, 0.8f},
      []() {
        GLFWwindow *window = glfwGetCurrentContext();
        if (window)
          glfwSetWindowShouldClose(window, GLFW_TRUE);
      });
  m_uiManager.addTextElement("menu_quit_text", {0.0f, 30.75f, 0.0f, 0.0f},
                             "QUIT", m_font, {1.0f, 0.8f, 0.8f, 1.0f}, 0.12f);
}

void MenuUI::_setupCreditsMenu() {
  m_uiManager.addTextElement("credits_title", {0.0f, 5.0f, 0.0f, 0.0f},
                             "CREDITS", m_font, {1.0f, 0.8f, 0.0f, 1.0f},
                             0.25f);

  std::string creditsText =
      "Vampire Survivor Clone\n\n"
      "Lead Developer: Jeerabhat Supapinit\n"
      "Second in command developer: Jane\n"
      "Special Thanks to Antigravity, Gemini CLI and Codex\n"
      "and for tuning the gameplay balance.\n\n"
      "Built with love for high-performance C++ gaming.\n"
      "Enjoy the chaos!";

  m_uiManager.addTextBoxElement("credits_box", {0.0f, 10.0f, 30.0f, 20.0f},
                                creditsText, m_font, {1.0f, 1.0f, 1.0f, 1.0f},
                                0.08f);

  m_uiManager.addInteractiveElement(
      "credits_back_btn", {0.0f, 32.0f, 10.0f, 3.0f}, {0.4f, 0.4f, 0.4f, 1.0f},
      [this]() { m_state = MenuState::MAIN; });
  m_uiManager.addTextElement("credits_back_text", {0.0f, 32.6f, 0.0f, 0.0f},
                             "BACK", m_font, {1.0f, 1.0f, 1.0f, 1.0f}, 0.1f);
}

void MenuUI::_updateStartMenu(const Game &game, float virtualWidth) {
  GameState gameState = game.getState();
  bool showMain =
      (gameState == GameState::START_MENU && m_state == MenuState::MAIN);

  m_uiManager.getElement("start_title")->visible = showMain;
  m_uiManager.getElement("menu_start_btn")->visible = showMain;
  m_uiManager.getElement("menu_start_text")->visible = showMain;
  m_uiManager.getElement("menu_credits_btn")->visible = showMain;
  m_uiManager.getElement("menu_credits_text")->visible = showMain;
  m_uiManager.getElement("menu_quit_btn")->visible = showMain;
  m_uiManager.getElement("menu_quit_text")->visible = showMain;

  if (showMain) {
    float center_x = virtualWidth / 2.0f;
    float time = static_cast<float>(glfwGetTime());

    if (auto *title = dynamic_cast<TextElement *>(
            m_uiManager.getElement("start_title"))) {
      float pulse = 1.0f + 0.05f * std::sin(time * 2.0f);
      title->scale = 0.35f * pulse;
      title->bounds.x =
          center_x - m_font.getTextWidth(title->text, title->scale) / 2.0f;
      title->bounds.y = 8.0f + 1.0f * std::sin(time * 1.5f);
    }

    auto centerButton = [&](const std::string &btnId,
                            const std::string &textId) {
      auto *btn = m_uiManager.getElement(btnId);
      auto *txt = dynamic_cast<TextElement *>(m_uiManager.getElement(textId));
      if (btn && txt) {
        btn->bounds.x = center_x - btn->bounds.w / 2.0f;
        txt->bounds.x =
            center_x - m_font.getTextWidth(txt->text, txt->scale) / 2.0f;
      }
    };

    centerButton("menu_start_btn", "menu_start_text");
    centerButton("menu_credits_btn", "menu_credits_text");
    centerButton("menu_quit_btn", "menu_quit_text");
  }
}

void MenuUI::_updateCreditsMenu(float virtualWidth) {
  bool showCredits = (m_state == MenuState::CREDITS);

  m_uiManager.getElement("credits_title")->visible = showCredits;
  m_uiManager.getElement("credits_box")->visible = showCredits;
  m_uiManager.getElement("credits_back_btn")->visible = showCredits;
  m_uiManager.getElement("credits_back_text")->visible = showCredits;

  if (showCredits) {
    float center_x = virtualWidth / 2.0f;

    if (auto *title = dynamic_cast<TextElement *>(
            m_uiManager.getElement("credits_title"))) {
      title->bounds.x =
          center_x - m_font.getTextWidth(title->text, title->scale) / 2.0f;
    }

    if (auto *box = dynamic_cast<TextBoxElement *>(
            m_uiManager.getElement("credits_box"))) {
      box->bounds.x = center_x - box->bounds.w / 2.0f;
    }

    auto *btn = m_uiManager.getElement("credits_back_btn");
    auto *txt = dynamic_cast<TextElement *>(
        m_uiManager.getElement("credits_back_text"));
    if (btn && txt) {
      btn->bounds.x = center_x - btn->bounds.w / 2.0f;
      txt->bounds.x =
          center_x - m_font.getTextWidth(txt->text, txt->scale) / 2.0f;
    }
  }
}
