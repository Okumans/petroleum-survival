#pragma once

#include "game/upgrade.hpp"
#include "ui/ui_manager.hpp"
#include <functional>
#include <vector>

class Game;

class LevelUpUI {
private:
  UIManager &m_uiManager;
  std::vector<Upgrade> m_upgrades;
  int m_selectedIndex = -1;
  Game *m_game = nullptr;

  // Callback when an upgrade is selected and applied
  std::function<void()> m_onUpgradeApplied;

public:
  LevelUpUI(UIManager &ui_manager);
  ~LevelUpUI();

  void show(const std::vector<Upgrade> &upgrades, Game *game);
  void hide();
  void selectUpgrade(int index);
  void applySelection();
  void cancel();

  [[nodiscard]] bool isVisible() const { return m_selectedIndex >= -1; }

private:
  void _createUpgradeButtons();
  void _createBackground();
};
