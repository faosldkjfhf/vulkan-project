#pragma once

#include "core/deletion_queue.h"
#include "core/device.h"
#include "core/model.h"
#include "core/pipeline.h"
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
  Renderer(Pointer<core::Window> window, Pointer<core::Device> device, VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
  ~Renderer();

  void cleanup();

  VkCommandBuffer beginRenderPass(uint32_t imageIndex);
  void endRenderPass(VkCommandBuffer commandBuffer);
  void draw(VkCommandBuffer commandBuffer);
  void setViewportAndScissor(VkCommandBuffer commandBuffer, VkViewport viewport, VkRect2D scissor);
  bool acquireNextImage(uint32_t *imageIndex);
  void present(uint32_t imageIndex);
  void advanceFrame();

  void waitForFence();
  void resetFence();

  VkSwapchainKHR swapchain() { return _swapchain; }
  VkRenderPass renderPass() { return _renderPass; }
  const VkFormat &format() { return _format; }
  const VkExtent2D &extent() { return _extent; }
  bool framebufferResized() { return _framebufferResized; }
  void setFramebufferResized(bool val) { _framebufferResized = val; }
  uint32_t currentFrame() { return _currentFrame; }
  float aspectRatio() { return _extent.width / (float)_extent.height; }
  VkCommandBuffer currentCommandBuffer() { return _commandBuffers[_currentFrame]; }
  VkImageView currentImageView() { return _imageViews[_currentFrame]; }
  VkImage currentImage() { return _images[_currentFrame]; }

private:
  void initialize();
  void initializeImgui();
  void createSwapchain();
  void createImageViews();
  void createRenderPass();
  void createDepthResources();
  void createFramebuffers();
  void createCommandBuffers();
  void createSyncObjects();
  void recreate();

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

  Pointer<core::Window> _window;
  Pointer<core::Device> _device;

  VkSwapchainKHR _swapchain;
  VkSwapchainKHR _oldSwapchain;
  VkRenderPass _renderPass;
  std::vector<VkImage> _images;
  std::vector<VkImageView> _imageViews;
  std::vector<VkFramebuffer> _framebuffers;
  VkFormat _format;
  VkExtent2D _extent;

  VkImage _depthImage;
  VmaAllocation _depthAllocation;
  VkImageView _depthImageView;

  std::vector<VkCommandBuffer> _commandBuffers;
  std::vector<VkSemaphore> _imageAvailableSemaphores;
  std::vector<VkSemaphore> _renderFinishedSemaphores;
  std::vector<VkFence> _inFlightFences;
  uint32_t _currentFrame = 0;
  bool _framebufferResized = false;

  core::DeletionQueue _deletionQueue;
};

} // namespace rendering
} // namespace bisky
