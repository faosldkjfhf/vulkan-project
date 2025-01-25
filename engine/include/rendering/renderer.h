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

class Renderer {
public:
  Renderer(core::Window &window, core::Device &device);
  ~Renderer();

  void cleanup();

  VkSwapchainKHR swapchain() { return _swapchain; }
  VkRenderPass renderPass() { return _renderPass; }

private:
  void initialize();
  void createSwapchain();
  void createImageViews();
  void createRenderPass();

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

  core::Window &_window;
  core::Device &_device;

  VkSwapchainKHR _swapchain;
  VkRenderPass _renderPass;
  std::vector<VkImage> _images;
  std::vector<VkImageView> _imageViews;
  VkFormat _format;
  VkExtent2D _extent;
};

} // namespace rendering
} // namespace bisky
