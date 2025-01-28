#include "engine.h"
#include "pch.h"

namespace bisky {

Engine::Engine() { initialize(); }

Engine::~Engine() { cleanup(); }

void Engine::initialize() { _renderer.addModel(core::vertices, core::indices); }

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

  vkDeviceWaitIdle(_device.device());
}

void Engine::input() { glfwPollEvents(); }

void Engine::update() {
  UniformBufferObject ubo;
  ubo.model = glm::mat4(1.0f);
  ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.projection = glm::perspective(glm::radians(45.0f), _renderer.aspectRatio(), 0.1f, 10.0f);
  ubo.projection[1][1] *= -1;
  _pipeline.updateUniformBuffer(_renderer.currentFrame(), (void *)&ubo, sizeof(ubo));
}

void Engine::render() {
  // reset fences and wait for next fence
  _renderer.waitForFence();

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(_renderer.extent().width);
  viewport.height = static_cast<float>(_renderer.extent().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = _renderer.extent();

  // try to acquire the next image
  uint32_t imageIndex;
  if (_renderer.acquireNextImage(&imageIndex)) {
    _renderer.resetFence();
  } else {
    return;
  }

  // begin the render pass
  VkCommandBuffer commandBuffer = _renderer.beginRenderPass(imageIndex);

  // submit commands
  _pipeline.bind(commandBuffer, _renderer.currentFrame());
  _renderer.setViewportAndScissor(commandBuffer, viewport, scissor);
  _renderer.draw(_pipeline, commandBuffer);

  // end command buffer and render pass
  _renderer.endRenderPass(commandBuffer);

  // submit to present queue
  _renderer.present(imageIndex);
  _renderer.advanceFrame();
}

void Engine::onKey(int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    _window.setShouldClose();
  }
}

void Engine::onResize(int width, int height) { _renderer.setFramebufferResized(true); }

void Engine::onClick(int button, int action, int mods) {}

void Engine::onMouseMove(double xpos, double ypos) {}

} // namespace bisky
