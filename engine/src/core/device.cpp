#include "core/window.h"

#include "core/device.h"
#include "pch.h"
#include "utils/utils.h"
#include <vulkan/vulkan_core.h>

namespace bisky {
namespace core {

Device::Device(Window &window) : _window(window) { initialize(); }

Device::~Device() {}

void Device::cleanup() {
  vmaDestroyAllocator(_allocator);
  vkDestroyCommandPool(_device, _commandPool, nullptr);
  vkDestroyDevice(_device, nullptr);

  if (utils::enableValidationLayers)
    utils::destroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);

  vkDestroySurfaceKHR(_instance, _surface, nullptr);
  vkDestroyInstance(_instance, nullptr);
}

void Device::initialize() {
  createInstance();
  setupDebugMessenger();
  createWindowSurface();
  pickPhysicalDevice();
  createLogicalDevice();
  createCommandPool();

  VmaVulkanFunctions vulkanFunctions = {};
  vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
  vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

  VmaAllocatorCreateInfo allocatorCreateInfo = {};
  allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
  allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_4;
  allocatorCreateInfo.physicalDevice = _physicalDevice;
  allocatorCreateInfo.device = _device;
  allocatorCreateInfo.instance = _instance;
  allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

  if (vmaCreateAllocator(&allocatorCreateInfo, &_allocator) != VK_SUCCESS) {
    throw std::runtime_error("failed to create allocator");
  }
}

void Device::createInstance() {
  if (utils::enableValidationLayers && !utils::checkValidationLayerSupport()) {
    throw std::runtime_error("validation layers requested, but none found");
  }

  VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.pApplicationName = "Simulator";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "Bisky Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_4;

  VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  createInfo.pApplicationInfo = &appInfo;

  auto extensions = utils::getRequiredExtensions();
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debugInfo;
  utils::populateDebugMessengerCreateInfo(debugInfo);

  if (utils::enableValidationLayers) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(utils::validationLayers.size());
    createInfo.ppEnabledLayerNames = utils::validationLayers.data();
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugInfo;
  } else {
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
  }

#if __APPLE__
  createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

  if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
    throw std::runtime_error("failed to create instance");
  }
}

void Device::setupDebugMessenger() {
  if (!utils::enableValidationLayers)
    return;

  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  utils::populateDebugMessengerCreateInfo(createInfo);

  if (utils::createDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &_debugMessenger) != VK_SUCCESS) {
    throw std::runtime_error("failed to setup debug messenger");
  }
}

void Device::createWindowSurface() {
  if (glfwCreateWindowSurface(_instance, _window.window(), nullptr, &_surface) != VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface");
  }
}

void Device::pickPhysicalDevice() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

  for (const auto &device : devices) {
    if (isDeviceSuitable(device)) {
      _physicalDevice = device;
      break;
    }
  }

  if (_physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
}

void Device::createLogicalDevice() {
  _indices = findQueueFamilies(_physicalDevice);

  float queuePriorities = 1.0f;
  VkDeviceQueueCreateInfo queueCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  queueCreateInfo.queueCount = 1;
  queueCreateInfo.queueFamilyIndex = _indices.queueFamily.value();
  queueCreateInfo.pQueuePriorities = &queuePriorities;

  VkPhysicalDeviceFeatures deviceFeatures = {};

  std::vector<const char *> extensions(utils::deviceExtensions);

#if __APPLE__
  extensions.push_back("VK_KHR_portability_subset");
#endif

  VkDeviceCreateInfo createInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  createInfo.queueCreateInfoCount = 1;
  createInfo.pQueueCreateInfos = &queueCreateInfo;
  createInfo.pEnabledFeatures = &deviceFeatures;
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS) {
    throw std::runtime_error("failed to create device");
  }

  vkGetDeviceQueue(_device, _indices.queueFamily.value(), 0, &_queue);
}

QueueFamilyIndices Device::findQueueFamilies(VkPhysicalDevice device) {
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

  int i = 0;
  for (const auto &queueFamily : queueFamilies) {
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentSupport);

    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupport) {
      indices.queueFamily = i;
    }

    if (indices.isComplete()) {
      break;
    }

    i++;
  }

  return indices;
}

bool Device::isDeviceSuitable(VkPhysicalDevice device) {
  QueueFamilyIndices indices = findQueueFamilies(device);

  bool extensionsSupported = checkDeviceExtensionSupport(device);

  bool swapchainAdequate = false;
  if (extensionsSupported) {
    SwapchainSupportDetails swapChainSupport = querySwapchainSupport(device);
    swapchainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  return indices.isComplete() && extensionsSupported && swapchainAdequate;
}

bool Device::checkDeviceExtensionSupport(VkPhysicalDevice device) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

  std::set<std::string> requiredExtensions(utils::deviceExtensions.begin(), utils::deviceExtensions.end());

  for (const auto &extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

SwapchainSupportDetails Device::querySwapchainSupport(VkPhysicalDevice device) {
  SwapchainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);

  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, nullptr);

  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, details.presentModes.data());
  }

  return details;
}

void Device::createCommandPool() {
  VkCommandPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = _indices.queueFamily.value();

  if (vkCreateCommandPool(_device, &poolInfo, nullptr, &_commandPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool");
  }
}

} // namespace core
} // namespace bisky
