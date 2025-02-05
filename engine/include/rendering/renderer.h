#pragma once

#include "core/compute_pipeline.h"
#include "core/deletion_queue.h"
#include "core/descriptors.h"
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

  VkCommandBuffer beginRenderPass();
  void endRenderPass(VkCommandBuffer commandBuffer);
  void clear(VkCommandBuffer commandBuffer, uint32_t imageIndex);
  void draw(VkCommandBuffer commandBuffer, Pointer<core::ComputePipeline> computePipeline, uint32_t imageIndex);
  void drawImgui(VkCommandBuffer commandBuffer, VkImageView target);
  void setViewportAndScissor(VkCommandBuffer commandBuffer, VkViewport viewport, VkRect2D scissor);
  bool acquireNextImage(uint32_t *imageIndex);
  void present(uint32_t imageIndex);

  void waitForFence();
  void resetFence();

  VkSwapchainKHR swapchain() { return _swapchain; }
  VkRenderPass renderPass() { return _renderPass; }
  const VkFormat &format() { return _format; }
  const VkExtent2D &extent() { return _extent; }
  bool framebufferResized() { return _framebufferResized; }
  void setFramebufferResized(bool val) { _framebufferResized = val; }
  float aspectRatio() { return _extent.width / (float)_extent.height; }
  VkCommandBuffer currentCommandBuffer() { return _frames[_currentFrame].mainCommandBuffer; }
  VkImageView currentImageView() { return _imageViews[_currentFrame]; }
  VkImage currentImage() { return _images[_currentFrame]; }
  FrameData &getCurrentFrame() { return _frames[_currentFrame]; }
  const VkDescriptorSetLayout &drawImageLayout() { return _drawImageDescriptorLayout; }
  const VkDescriptorSet &drawImageDescriptors() { return _drawImageDescriptors; }

private:
  void initialize();
  void initializeImgui();
  void createSwapchain();
  void createImageViews();
  void createRenderPass();
  void createDepthResources();
  void createFramebuffers();
  void initializeCommands();
  void initializeSyncStructures();
  void initializeDescriptors();
  void recreate();

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

  Pointer<core::Window> _window;
  Pointer<core::Device> _device;

  VkSwapchainKHR _swapchain;
  VkSwapchainKHR _oldSwapchain;
  VkRenderPass _renderPass;
  Vector<VkImage> _images;
  Vector<VkImageView> _imageViews;
  Vector<VkFramebuffer> _framebuffers;
  VkFormat _format;
  VkExtent2D _extent;

  VkImage _depthImage;
  VmaAllocation _depthAllocation;
  VkImageView _depthImageView;

  core::DescriptorAllocator _globalDescriptorAllocator;

  VkDescriptorSet _drawImageDescriptors;
  VkDescriptorSetLayout _drawImageDescriptorLayout;
  AllocatedImage _drawImage;
  VkExtent2D _drawExtent;

  VkFence _immFence;
  VkCommandBuffer _immCommandBuffer;
  VkCommandPool _immCommandPool;
  VkDescriptorPool _imguiPool;

  FrameData _frames[FRAME_OVERLAP];
  uint32_t _currentFrame = 0;
  bool _framebufferResized = false;

  core::DeletionQueue _deletionQueue;
};

} // namespace rendering
} // namespace bisky
