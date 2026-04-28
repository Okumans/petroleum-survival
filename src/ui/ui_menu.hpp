#pragma once

#include "ui/ui_manager.hpp"

class Game;
class BitmapFont;

class MenuUI {
private:
  UIManager &m_uiManager;
  const BitmapFont &m_font;

public:
  MenuUI(UIManager &ui_manager, const BitmapFont &font);
  ~MenuUI();

  void setup(Game &game);
  void update(const Game &game, float virtualWidth);

private:
  void _setupStartMenu(Game &game);
  void _updateStartMenu(const Game &game, float virtualWidth);
};
