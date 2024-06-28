#pragma once

#include "glfw.hpp"

class Window {
public:
  Window(const int &w, const int &h);
  ~Window();

  GLFWwindow *getWindow();

private:
  int _width;
  int _height;

  GLFWwindow *_window;
};
