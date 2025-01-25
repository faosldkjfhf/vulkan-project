#include "core/window.h"

namespace bisky {
namespace core {

Window::Window(int width, int height, const char *title, Callbacks *callbacks) {
  if (!glfwInit()) {
    throw std::runtime_error("failed to init glfw");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  _window = glfwCreateWindow(width, height, title, NULL, NULL);
  if (!_window) {
    glfwTerminate();
    throw std::runtime_error("failed to create window");
  }

  glfwSetWindowUserPointer(_window, callbacks);
  glfwSetKeyCallback(_window, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
    reinterpret_cast<Callbacks *>(glfwGetWindowUserPointer(window))->onKey(key, scancode, action, mods);
  });

  glfwSetFramebufferSizeCallback(_window, [](GLFWwindow *window, int width, int height) {
    reinterpret_cast<Callbacks *>(glfwGetWindowUserPointer(window))->onResize(width, height);
  });
}

Window::~Window() {}

bool Window::shouldClose() { return glfwWindowShouldClose(_window); }

void Window::setShouldClose() { glfwSetWindowShouldClose(_window, true); }

void Window::cleanup() {
  glfwDestroyWindow(_window);
  glfwTerminate();
}

} // namespace core
} // namespace bisky
