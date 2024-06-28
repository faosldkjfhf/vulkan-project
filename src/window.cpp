#include "window.hpp"
#include <iostream>

static void glfwError(int id, const char *description) {
  std::cout << description << std::endl;
}

Window::Window(const int &w, const int &h) : _width(w), _height(h) {
  glfwSetErrorCallback(&glfwError);

  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  _window =
      glfwCreateWindow(_width, _height, "Vulkan Engine", nullptr, nullptr);
}

Window::~Window() {
  glfwDestroyWindow(_window);
  glfwTerminate();
}
