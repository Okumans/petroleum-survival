#pragma once

#include "game/game.hpp"
#include "ui/ui_manager.hpp"

#include <GLFW/glfw3.h>

#ifndef SHADER_PATH
#define SHADER_PATH ASSETS_PATH "/shaders"
#endif

#ifndef ICONS_PATH
#define ICONS_PATH ASSETS_PATH "/icons"
#endif

#define UI_VERTEX_SHADER_PATH SHADER_PATH "/ui.vert.glsl"
#define UI_FRAGMENT_SHADER_PATH SHADER_PATH "/ui.frag.glsl"

struct InputState {
  bool isFirstMouse = false;
  float mouseLastX = 0.0f, mouseLastY = 0.0f;
};

struct AppState {
  int windowWidth = 800, windowHeight = 600;
  InputState inputState;
  bool gameStarted = false;
};

#include <functional>
#include <string>
#include <vector>

struct LoadingTask {
  std::string name;
  std::function<void()> task;
};

class App {
private:
  GLFWwindow *m_window;

  Game m_game;

  AppState m_appState;

  UIManager m_uiManager;
  BitmapFont m_font;

  std::vector<LoadingTask> m_loadingTasks;
  size_t m_currentLoadingTask = 0;

public:
  App(GLFWwindow *window);
  ~App();
  void render(double delta_time);

private:
  // GLFW static callbacks adapters
  static void _glfwKeyCallback(GLFWwindow *window, int key, int scancode,
                               int action, int mods);
  static void _glfwMouseMoveCallback(GLFWwindow *window, double pos_x,
                                     double pos_y);
  static void _glfwMouseButtonCallback(GLFWwindow *window, int button,
                                       int action, int mods);
  static void _glfwScrollCallback(GLFWwindow *window, double offset_x,
                                  double offset_y);
  static void _glfwFramebufferSizeCallback(GLFWwindow *window, int width,
                                           int height);
  // internal event handler
  void _handleKeyCallback(int key, int scancode, int action, int mods);
  void _handleProcessInput(double delta_time);
  void _handleMouseMoveCallback(double pos_x, double pos_y);
  void _handleMouseClickCallback(int button, int action, int mods);
  void _handleScrollCallback(double offset_x, double offset_y);
  void _handleFramebufferSizeCallback(int width, int height);

  void _setupResources();
  void _setupUIElements();
  void _updateUIElements(double delta_time);
};
