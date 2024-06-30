#include "vulkan_program.hpp"
#include <GLFW/glfw3.h>
#include <cstring>
#include <iostream>

VulkanProgram::~VulkanProgram() {
  vkDestroyDevice(_device, nullptr);
  destroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
  vkDestroyInstance(_instance, nullptr);
}

void VulkanProgram::initVulkan() {
  createInstance();
  setupDebugMessenger();
  pickPhysicalDevice();
  createLogicalDevice();
}

void VulkanProgram::createLogicalDevice() {
  QueueFamilyIndices indices = findQueueFamilies(_physicalDevice);

  VkDeviceQueueCreateInfo queueCreateInfo = {};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
  queueCreateInfo.queueCount = 1;

  float queuePriority = 1.0f;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  VkPhysicalDeviceFeatures deviceFeatures = {};

  VkDeviceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = &queueCreateInfo;
  createInfo.queueCreateInfoCount = 1;
  createInfo.pEnabledFeatures = &deviceFeatures;
  createInfo.enabledLayerCount = _validationLayers.size();
  createInfo.ppEnabledLayerNames = _validationLayers.data();

  if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create logical device");
  }

  vkGetDeviceQueue(_device, indices.graphicsFamily, 0, &_graphicsQueue);

  std::cout << "DEBUG: logical device created" << std::endl;
}

QueueFamilyIndices VulkanProgram::findQueueFamilies(VkPhysicalDevice device) {
  QueueFamilyIndices indices;

  unsigned int queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, 0);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies.data());

  int i = 0;
  for (const auto &queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
    }

    if (indices.isComplete()) {
      break;
    }

    i++;
  }

  if (indices.isComplete()) {
    std::cout << "DEBUG: queue family found" << std::endl;
  }

  return indices;
}

void VulkanProgram::populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
  createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
  createInfo.pUserData = nullptr;
}

void VulkanProgram::setupDebugMessenger() {
  VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
  populateDebugMessengerCreateInfo(createInfo);

  if (createDebugUtilsMessengerEXT(_instance, &createInfo, nullptr,
                                   &_debugMessenger) != VK_SUCCESS) {
    throw std::runtime_error("Couldn't create debug messenger");
  }

  std::cout << "DEBUG: debug setup successfully" << std::endl;
}

void VulkanProgram::createInstance() {
  if (!checkValidationLayerSupport()) {
    throw std::runtime_error("Validation layer not found");
  }

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Vulkan Program";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  std::vector<const char *> extensions = getRequiredExtensions();

  VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
  populateDebugMessengerCreateInfo(debugInfo);

  createInfo.enabledExtensionCount = extensions.size();
  createInfo.ppEnabledExtensionNames = extensions.data();
  createInfo.enabledLayerCount = _validationLayers.size();
  createInfo.ppEnabledLayerNames = _validationLayers.data();
  createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugInfo;

  if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create Vulkan instance");
  }

  std::cout << "DEBUG: vulkan instance created" << std::endl;
}

void VulkanProgram::pickPhysicalDevice() {
  unsigned int deviceCount = 0;
  vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw std::runtime_error("Failed to find GPUs with Vulkan support");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

  for (const auto &device : devices) {
    if (isDeviceSuitable(device)) {
      _physicalDevice = device;
      break;
    }
  }

  if (_physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("Failed to find a suitable GPU");
  }

  std::cout << "DEBUG: physical device found" << std::endl;
}

bool VulkanProgram::isDeviceSuitable(VkPhysicalDevice device) {
  VkPhysicalDeviceProperties deviceProperties = {};
  vkGetPhysicalDeviceProperties(device, &deviceProperties);

  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

  // TODO: Ensure certain features/properties, right now doesn't matter

  QueueFamilyIndices indices = findQueueFamilies(device);

  return indices.isComplete();
}

const std::vector<const char *> VulkanProgram::getRequiredExtensions() {
  unsigned int glfwExtensionCount = 0;
  const char **glfwExtensions;

  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char *> extensions(glfwExtensions,
                                       glfwExtensions + glfwExtensionCount);
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  return extensions;
}

const bool VulkanProgram::checkValidationLayerSupport() {
  unsigned int layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char *layerName : _validationLayers) {
    bool layerFound = false;

    for (const auto &layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }

    if (!layerFound) {
      return false;
    }
  }

  return true;
}

VkResult VulkanProgram::createDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void VulkanProgram::destroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks *pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}
