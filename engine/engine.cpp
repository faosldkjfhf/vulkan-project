#include "engine.h"

namespace bisky {

Engine::Engine() { initialize(); }

Engine::~Engine() { cleanup(); }

void Engine::initialize() {}

void Engine::cleanup() {
  _pipeline.cleanup();
  _renderer.cleanup();
  _device.cleanup();
  _window.cleanup();
}

void Engine::run() {
  while (!_window.shouldClose()) {
    input();
    update();
    render();
  }
}

void Engine::input() { glfwPollEvents(); }

void Engine::update() {}

void Engine::render() {}

void Engine::onKey(int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    _window.setShouldClose();
  }
}

void Engine::onResize(int width, int height) {}

void Engine::onClick(int button, int action, int mods) {}

void Engine::onMouseMove(double xpos, double ypos) {}
} // namespace bisky
