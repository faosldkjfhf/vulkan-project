#pragma once

#include "glfw.hpp"

class Window {
public:
  Window(const int &w, const int &h);
  ~Window();

  inline GLFWwindow *getWindow() { return _window; }

private:
  int _width;
  int _height;

  GLFWwindow *_window;
};
