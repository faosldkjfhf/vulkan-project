#pragma once

#include "vulkan_program.hpp"
#include "window.hpp"
#include <memory>

class Engine {
public:
  Engine();
  ~Engine();

  void run();

  void input();
  void update();
  void render();
  void loop();

private:
  std::shared_ptr<Window> _window;
  std::shared_ptr<VulkanProgram> _vulkanProgram;
};
