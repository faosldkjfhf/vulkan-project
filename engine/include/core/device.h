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
  VkQueue queue() { return _queue; }
  VkSurfaceKHR surface() { return _surface; }
  VkCommandPool commandPool() { return _commandPool; }
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
  void transitionImageLayout(VkImage image, VkFormat format, uint32_t mipLevels, VkImageLayout oldLayout,
                             VkImageLayout newLayout);

private:
  void initialize();
  void createInstance();
  void setupDebugMessenger();
  void createWindowSurface();
  void pickPhysicalDevice();
  void createLogicalDevice();

  // NOTE: Possible to create 2 command pools
  // one for short-term allocations, one for long-term
  void createCommandPool();

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
  bool isDeviceSuitable(VkPhysicalDevice device);
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);

  Pointer<Window> _window;

  VkInstance _instance;
  VkPhysicalDevice _physicalDevice;
  VkDevice _device;
  VkSurfaceKHR _surface;
  VkDebugUtilsMessengerEXT _debugMessenger;
  QueueFamilyIndices _indices;
  VkQueue _queue;
  VkCommandPool _commandPool;
  VmaAllocator _allocator;

  DeletionQueue _deletionQueue;
};

} // namespace core
} // namespace bisky
