#include "engine.hpp"

Engine::Engine()
    : _window(std::make_shared<Window>(640, 480)),
      _vulkanProgram(std::make_shared<VulkanProgram>()) {
  _vulkanProgram->initVulkan(_window->getWindow());
}

Engine::~Engine() {}

void Engine::run() { loop(); }

void Engine::loop() {
  while (!glfwWindowShouldClose(_window->getWindow())) {
    input();
    update();
    render();
  }
}

void Engine::input() { glfwPollEvents(); }

void Engine::update() {}

void Engine::render() {}
