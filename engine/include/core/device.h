#pragma once

#include "pch.h"

namespace bisky {
namespace core {

class Window;

class Device {

public:
  Device(Window &window);
  ~Device();

  void cleanup();

  SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);

  VkDevice device() { return _device; }
  VkPhysicalDevice physicalDevice() { return _physicalDevice; }
  VkInstance instance() { return _instance; }
  QueueFamilyIndices indices() { return _indices; }
  VkQueue queue() { return _queue; }
  VkSurfaceKHR surface() { return _surface; }

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

  Window &_window;
  VkInstance _instance;
  VkPhysicalDevice _physicalDevice;
  VkDevice _device;
  VkSurfaceKHR _surface;
  VkDebugUtilsMessengerEXT _debugMessenger;
  QueueFamilyIndices _indices;
  VkQueue _queue;
};

} // namespace core
} // namespace bisky
