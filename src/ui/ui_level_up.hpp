#pragma once

#include "ui/ui_manager.hpp"

class Game;
class BitmapFont;

class LevelUI {
private:
  UIManager &m_uiManager;
  const BitmapFont &m_font;

public:
  LevelUI(UIManager &ui_manager, const BitmapFont &font);
  ~LevelUI();

  void setup(Game &game);
  void update(const Game &game, float virtualWidth);

private:
  void _setupHUD();
  void _setupLevelUpOverlay(Game &game);
  void _updateHUD(const Game &game, float virtualWidth);
  void _updateLevelUpOverlay(const Game &game, float virtualWidth);
};
