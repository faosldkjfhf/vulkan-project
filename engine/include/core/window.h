#pragma once

#include "icallbacks.h"
#include "pch.h"

namespace bisky {
namespace core {

class Window {
public:
  Window(int width, int height, const char *title, ICallbacks *callbacks);
  ~Window();

  GLFWwindow *window() { return _window; }

  bool shouldClose();
  void setShouldClose();

  void cleanup();

private:
  GLFWwindow *_window;
};

} // namespace core
} // namespace bisky
