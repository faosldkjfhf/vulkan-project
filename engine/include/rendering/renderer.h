#pragma once

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
  Renderer(core::Window &window, core::Device &device);
  ~Renderer();

  void cleanup();
  void cleanupSwapchain();

  void addModel(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);
  void addModel(const char *objPath);

  void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                   VkMemoryPropertyFlags properties, VkImage &image, VmaAllocation &imageAllocation);
  void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

  VkCommandBuffer beginRenderPass(uint32_t imageIndex);
  void endRenderPass(VkCommandBuffer commandBuffer);
  VkCommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(VkCommandBuffer commandBuffer);
  void draw(core::Pipeline &pipeline, VkCommandBuffer commandBuffer);
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
  uint32_t currentFrame() { return _currentFrame; }
  float aspectRatio() { return _extent.width / (float)_extent.height; }

private:
  void initialize();
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

  bool hasStencilComponent(VkFormat format);

  core::Window &_window;
  core::Device &_device;

  VkSwapchainKHR _swapchain;
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

  std::vector<core::Model> _models;
};

} // namespace rendering
} // namespace bisky
