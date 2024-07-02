#pragma once

// TODO: Abstract everything out into separate classes

#include "glfw.hpp"
#include <iostream>
#include <vector>

struct QueueFamilyIndices {
  int graphicsFamily = -1;
  int presentFamily = -1;

  bool isComplete() { return graphicsFamily != -1 && presentFamily != -1; }
};

struct SwapchainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

class VulkanProgram {
public:
  VulkanProgram() = default;
  ~VulkanProgram();

  void initVulkan(GLFWwindow *window);

private:
  VkInstance _instance;
  VkDebugUtilsMessengerEXT _debugMessenger;
  VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
  VkQueue _graphicsQueue;
  VkQueue _presentQueue;
  VkDevice _device;
  VkSurfaceKHR _surface;
  VkSwapchainKHR _swapchain;
  std::vector<VkImage> _swapchainImages;
  VkFormat _swapchainImageFormat;
  VkExtent2D _swapchainExtent;
  std::vector<VkImageView> _swapchainImageViews;

  const std::vector<const char *> _validationLayers = {
      "VK_LAYER_KHRONOS_validation",
  };
  const std::vector<const char *> _deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  };

  void createInstance();
  void setupDebugMessenger();
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createSurface(GLFWwindow *window);
  void createSwapChain(GLFWwindow *window);
  void createImageViews();
  void createGraphicsPipeline();

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
  SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);

  const bool isDeviceSuitable(VkPhysicalDevice device);
  const bool checkDeviceExtensionSupport(VkPhysicalDevice device);

  const std::vector<const char *> getRequiredExtensions();
  const bool checkValidationLayerSupport();
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities,
                              GLFWwindow *window);

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
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
      std::cout << "validation layer: " << pCallbackData->pMessage << std::endl;
    }

    return VK_FALSE;
  }
};
