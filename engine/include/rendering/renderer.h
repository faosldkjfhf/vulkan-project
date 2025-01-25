#pragma once

#include "core/device.h"
#include "core/window.h"
#include "pch.h"

namespace bisky {

namespace core {

class Window;
class Device;

} // namespace core

namespace rendering {

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

class Renderer {
public:
  Renderer(core::Window &window, core::Device &device);
  ~Renderer();

  void cleanup();
  void cleanupSwapchain();

  VkCommandBuffer beginRenderPass(uint32_t imageIndex);
  void endRenderPass(VkCommandBuffer commandBuffer);
  void draw(VkCommandBuffer commandBuffer, uint32_t count, uint32_t instances = 1);
  void setViewportAndScissor(VkCommandBuffer commandBuffer, VkViewport viewport, VkRect2D scissor);
  bool acquireNextImage(uint32_t *imageIndex);
  void present(uint32_t imageIndex);
  void advanceFrame();

  void waitForFence();
  void resetFence();

  VkSwapchainKHR swapchain() { return _swapchain; }
  VkRenderPass renderPass() { return _renderPass; }
  VkFormat format() { return _format; }
  VkExtent2D extent() { return _extent; }
  bool framebufferResized() { return _framebufferResized; }
  void setFramebufferResized(bool val) { _framebufferResized = val; }

private:
  void initialize();
  void createSwapchain();
  void createImageViews();
  void createRenderPass();
  void createFramebuffers();
  void createCommandBuffers();
  void createSyncObjects();
  void recreate();

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

  core::Window &_window;
  core::Device &_device;

  VkSwapchainKHR _swapchain;
  VkRenderPass _renderPass;
  std::vector<VkImage> _images;
  std::vector<VkImageView> _imageViews;
  std::vector<VkFramebuffer> _framebuffers;
  VkFormat _format;
  VkExtent2D _extent;

  std::vector<VkCommandBuffer> _commandBuffers;
  std::vector<VkSemaphore> _imageAvailableSemaphores;
  std::vector<VkSemaphore> _renderFinishedSemaphores;
  std::vector<VkFence> _inFlightFences;
  uint32_t _currentFrame = 0;
  bool _framebufferResized = false;
};

} // namespace rendering
} // namespace bisky
