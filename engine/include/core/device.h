#pragma once

#include "core/deletion_queue.h"
#include "pch.h"

namespace bisky {
namespace core {

class Window;

class Device {

public:
  Device(Pointer<Window> window);
  ~Device();

  void cleanup();

  SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);
  VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                               VkFormatFeatureFlags features);
  VkFormat findDepthFormat();

  VkDevice device() { return _device; }
  VkPhysicalDevice physicalDevice() { return _physicalDevice; }
  VkInstance instance() { return _instance; }
  QueueFamilyIndices indices() { return _indices; }
  uint32_t queueFamily() { return _indices.queueFamily.value(); }
  const VkQueue &queue() { return _queue; }
  VkSurfaceKHR surface() { return _surface; }
  VmaAllocator allocator() { return _allocator; }

  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryAllocateFlags properties, VkBuffer &buffer,
                    VmaAllocation &allocation);
  VkCommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(VkCommandBuffer commandBuffer);
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
  VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
  void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling,
                   VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image,
                   VmaAllocation &imageAllocation);
  void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
                             uint32_t mipLevels = 1);

private:
  void initialize();
  void createInstance();
  void setupDebugMessenger();
  void createWindowSurface();
  void pickPhysicalDevice();
  void createLogicalDevice();

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
  bool isDeviceSuitable(VkPhysicalDevice device);
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);

  Pointer<Window> _window;

  VkInstance _instance;
  VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
  VkDevice _device;
  VkSurfaceKHR _surface;
  VkDebugUtilsMessengerEXT _debugMessenger;
  QueueFamilyIndices _indices;
  VkQueue _queue;
  VmaAllocator _allocator;

  DeletionQueue _deletionQueue;
};

} // namespace core
} // namespace bisky
