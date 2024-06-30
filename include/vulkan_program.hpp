#pragma once

#include "glfw.hpp"
#include <iostream>
#include <vector>

struct QueueFamilyIndices {
  int graphicsFamily = -1;

  bool isComplete() { return graphicsFamily != -1; }
};

class VulkanProgram {
public:
  VulkanProgram() = default;
  ~VulkanProgram();

  void initVulkan();

private:
  VkInstance _instance;
  VkDebugUtilsMessengerEXT _debugMessenger;
  VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
  VkQueue _graphicsQueue;
  VkDevice _device;
  const std::vector<const char *> _validationLayers = {
      "VK_LAYER_KHRONOS_validation",
  };

  void createInstance();
  void setupDebugMessenger();
  void pickPhysicalDevice();
  void createLogicalDevice();
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

  bool isDeviceSuitable(VkPhysicalDevice device);

  const std::vector<const char *> getRequiredExtensions();
  const bool checkValidationLayerSupport();

  // TODO: Move to a separate utils class
  void populateDebugMessengerCreateInfo(
      VkDebugUtilsMessengerCreateInfoEXT &createInfo);
  VkResult createDebugUtilsMessengerEXT(
      VkInstance instance,
      const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
      const VkAllocationCallbacks *pAllocator,
      VkDebugUtilsMessengerEXT *pDebugMessenger);
  void destroyDebugUtilsMessengerEXT(VkInstance instance,
                                     VkDebugUtilsMessengerEXT debugMessenger,
                                     const VkAllocationCallbacks *pAllocator);

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData) {
    if (messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
      std::cout << messageSeverity << ": " << pCallbackData->pMessage
                << std::endl;
    }

    return VK_FALSE;
  }
};
