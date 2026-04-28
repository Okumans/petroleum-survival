#include <glad/gl.h>

#include <GLFW/glfw3.h>
#include <exception>
#include <glm/glm.hpp>
#include <iostream>

#include "app.hpp"

#ifndef ASSETS_PATH
#define ASSETS_PATH "./assets"
#endif

#ifndef SHADER_PATH
#define SHADER_PATH ASSETS_PATH "/shaders"
#endif

void process_input(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, 1);
}

GLFWwindow *initialize_window(int width, int height, const char *title) {
  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  glfwWindowHint(GLFW_DEPTH_BITS, 24);

  GLFWwindow *window = glfwCreateWindow(width, height, title, NULL, NULL);
  if (!window) {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  glfwMakeContextCurrent(window);

  if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialize GLAD" << std::endl;
    exit(EXIT_FAILURE);
  }
  return window;
}

int main() {
  GLFWwindow *window = initialize_window(800, 600, "vampire survivor");

  try {
    App application(window);

    // Reset time after loading to avoid a huge first-frame delta
    double last_frame_time = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
      double current_frame_time = glfwGetTime();
      double delta_frame_time = current_frame_time - last_frame_time;
      last_frame_time = current_frame_time;

      process_input(window);

      // We don't clear here because App/Game handle their own clearing
      // with state-specific colors (e.g. loading screen vs game world)
      application.render(delta_frame_time);

      glfwSwapBuffers(window);
      glfwPollEvents();
    }
  } catch (const std::exception &e) {
    std::cerr << "Application Exception: " << e.what() << std::endl;
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
