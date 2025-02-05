#include "engine.h"

#include "core/compute_pipeline.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "rendering/renderer.h"
#include "utils/utils.h"

namespace bisky {

Engine::Engine() {
  initialize();
  initializeImgui();
}

Engine::~Engine() { cleanup(); }

void Engine::initialize() {
  _window = std::make_shared<core::Window>(800, 800, "Bisky Engine", this);
  _device = std::make_shared<core::Device>(_window);
  _renderer = std::make_shared<rendering::Renderer>(_window, _device);
  _computePipeline = std::make_shared<core::ComputePipeline>(_window, _device, _renderer);
}

void Engine::initializeImgui() {}

void Engine::createDefaultScene() {
  core::Model::Builder builder;
  builder.vertices = {
      {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
      {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
      {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
      {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
  };
  builder.indices = {0, 1, 2, 2, 3, 0};

  _models.emplace_back(std::make_shared<core::Model>(_device, builder));
}

void Engine::cleanup() {
  for (auto model : _models) {
    model->cleanup();
  }

  _computePipeline->cleanup();
  _renderer->cleanup();
  _device->cleanup();
  _window->cleanup();
}

void Engine::run() {
  while (!_window->shouldClose()) {
    input();
    update();
    render();
  }

  vkDeviceWaitIdle(_device->device());
}

void Engine::input() { glfwPollEvents(); }

void Engine::update() {}

void Engine::render() {
  // reset fences and wait for next fence
  _renderer->waitForFence();

  // try to acquire the next image
  uint32_t imageIndex;
  if (_renderer->acquireNextImage(&imageIndex)) {
    _renderer->resetFence();
  } else {
    return;
  }

  // begin the render pass
  VkCommandBuffer commandBuffer = _renderer->beginRenderPass();

  // draw the image to the swapchain
  _renderer->draw(commandBuffer, _computePipeline, imageIndex);

  // end command buffer and render pass
  _renderer->endRenderPass(commandBuffer);

  // submit to present queue
  _renderer->present(imageIndex);
}

void Engine::onKey(int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    _window->setShouldClose();
  }
}

void Engine::onResize(int width, int height) { _renderer->setFramebufferResized(true); }

void Engine::onClick(int button, int action, int mods) {}

void Engine::onMouseMove(double xpos, double ypos) {}

} // namespace bisky
