#pragma once

#include "core/device.h"
#include "core/immedate_submit.h"
#include "gpu/gpu_mesh_buffers.h"
#include "pch.h"
#include "utils/init.h"
#include <cstring>
#include <fstream>
#include <slang-com-ptr.h>
#include <span>
#include <vulkan/vulkan_core.h>

namespace bisky {
namespace utils {

#ifdef NDEBUG
static constexpr bool enableValidationLayers = false;
#else
static constexpr bool enableValidationLayers = true;
#endif

const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation", "VK_LAYER_KHRONOS_synchronization2"};

const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME};

inline bool hasStencilComponent(VkFormat format) {
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

inline bool checkValidationLayerSupport() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char *layerName : validationLayers) {
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

inline std::vector<const char *> getRequiredExtensions() {
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#if __APPLE__
  extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

  if (enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }
  extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

  return extensions;
}

inline static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                           VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                           const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                           void *pUserData) {

  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    std::cerr << "[vulkan] " << pCallbackData->pMessage << std::endl;
  }

  return VK_FALSE;
}
inline VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                             const VkAllocationCallbacks *pAllocator,
                                             VkDebugUtilsMessengerEXT *pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

inline void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                          const VkAllocationCallbacks *pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

inline void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
  createInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
}

inline void transitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout) {
  VkImageMemoryBarrier2 imageBarrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
  imageBarrier.pNext = nullptr;

  imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
  imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
  imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
  imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

  imageBarrier.oldLayout = currentLayout;
  imageBarrier.newLayout = newLayout;

  VkImageAspectFlags aspectMask =
      (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
  imageBarrier.subresourceRange = init::imageSubresourceRange(aspectMask);
  imageBarrier.image = image;

  VkDependencyInfo depInfo{};
  depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  depInfo.pNext = nullptr;

  depInfo.imageMemoryBarrierCount = 1;
  depInfo.pImageMemoryBarriers = &imageBarrier;

  vkCmdPipelineBarrier2(cmd, &depInfo);
}

inline void copyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize,
                             VkExtent2D dstSize) {
  VkImageBlit2 blitRegion = {};
  blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
  blitRegion.pNext = nullptr;

  blitRegion.srcOffsets[1].x = srcSize.width;
  blitRegion.srcOffsets[1].y = srcSize.height;
  blitRegion.srcOffsets[1].z = 1;
  blitRegion.dstOffsets[1].x = dstSize.width;
  blitRegion.dstOffsets[1].y = dstSize.height;
  blitRegion.dstOffsets[1].z = 1;

  blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blitRegion.srcSubresource.baseArrayLayer = 0;
  blitRegion.srcSubresource.layerCount = 1;
  blitRegion.srcSubresource.mipLevel = 0;

  blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blitRegion.dstSubresource.baseArrayLayer = 0;
  blitRegion.dstSubresource.layerCount = 1;
  blitRegion.dstSubresource.mipLevel = 0;

  VkBlitImageInfo2 blitInfo = {};
  blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
  blitInfo.dstImage = destination;
  blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  blitInfo.srcImage = source;
  blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  blitInfo.filter = VK_FILTER_LINEAR;
  blitInfo.regionCount = 1;
  blitInfo.pRegions = &blitRegion;

  vkCmdBlitImage2(cmd, &blitInfo);
}

inline bool loadShaderModule(const char *filePath, VkDevice device, VkShaderModule *outModule) {
  std::ifstream file(filePath, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    return false;
  }

  size_t fileSize = (size_t)file.tellg();
  Vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
  file.seekg(0);
  file.read((char *)buffer.data(), fileSize);
  file.close();

  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = buffer.size() * sizeof(uint32_t);
  createInfo.pCode = buffer.data();

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    return false;
  }

  *outModule = shaderModule;
  return true;
}

inline bool loadShaderModule(Slang::ComPtr<slang::ISession> session, slang::IModule *module, VkDevice device,
                             const char *entryPoint, VkShaderModule *outModule) {
  Slang::ComPtr<slang::IEntryPoint> stageEntryPoint;
  module->findEntryPointByName(entryPoint, stageEntryPoint.writeRef());

  std::vector<slang::IComponentType *> componentTypes;
  componentTypes.push_back(module);
  componentTypes.push_back(stageEntryPoint);

  Slang::ComPtr<slang::IComponentType> composedProgram;
  {
    Slang::ComPtr<slang::IBlob> diagnosticsBlob;
    SlangResult result = session->createCompositeComponentType(componentTypes.data(), componentTypes.size(),
                                                               composedProgram.writeRef(), diagnosticsBlob.writeRef());

    if (diagnosticsBlob) {
      fprintf(stdout, "%s\n", (const char *)diagnosticsBlob->getBufferPointer());
    }

    if (!SLANG_SUCCEEDED(result)) {
      return false;
    }
  }

  Slang::ComPtr<slang::IBlob> spirvCode;
  {
    Slang::ComPtr<slang::IBlob> diagnosticsBlob;
    SlangResult result = composedProgram->getEntryPointCode(0, 0, spirvCode.writeRef(), diagnosticsBlob.writeRef());

    if (diagnosticsBlob) {
      fprintf(stdout, "%s\n", (const char *)diagnosticsBlob->getBufferPointer());
    }

    if (!SLANG_SUCCEEDED(result)) {
      return false;
    }
  }

  VkShaderModule shaderModule;
  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = spirvCode->getBufferSize();
  createInfo.pCode = static_cast<const uint32_t *>(spirvCode->getBufferPointer());

  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    return false;
  }

  *outModule = shaderModule;
  return true;
}

inline slang::IModule *createSlangModule(Slang::ComPtr<slang::ISession> session, const char *file) {
  slang::IModule *module;
  Slang::ComPtr<slang::IBlob> diagnosticsBlob;
  module = session->loadModule(file, diagnosticsBlob.writeRef());

  if (diagnosticsBlob) {
    fprintf(stdout, "%s\n", (const char *)diagnosticsBlob->getBufferPointer());
  }

  return module;
}

inline GPUMeshBuffers uploadMesh(Pointer<core::Device> device, Pointer<core::ImmediateSubmit> immediateSubmit,
                                 std::span<uint32_t> indices, std::span<Vertex> vertices) {
  const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
  const size_t indexBufferSize = indices.size() * sizeof(uint32_t);
  GPUMeshBuffers buffers;

  GPUBuffer::Builder builder = {};
  builder.allocator = device->allocator();

  buffers.vertexBuffer = builder.build(vertexBufferSize,
                                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                           VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                       VMA_MEMORY_USAGE_GPU_ONLY);

  VkBufferDeviceAddressInfo deviceAddressInfo = {};
  deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  deviceAddressInfo.buffer = buffers.vertexBuffer.buffer;
  buffers.vertexBufferAddress = vkGetBufferDeviceAddress(device->device(), &deviceAddressInfo);

  buffers.indexBuffer = builder.build(
      indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

  GPUBuffer staging =
      builder.build(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

  void *data = staging.info.pMappedData;
  memcpy(data, vertices.data(), vertexBufferSize);
  memcpy((char *)data + vertexBufferSize, indices.data(), indexBufferSize);

  immediateSubmit->submit([&](VkCommandBuffer cmd) {
    VkBufferCopy vertexCopy = {};
    vertexCopy.size = vertexBufferSize;
    vertexCopy.srcOffset = 0;
    vertexCopy.dstOffset = 0;
    vkCmdCopyBuffer(cmd, staging.buffer, buffers.vertexBuffer.buffer, 1, &vertexCopy);

    VkBufferCopy indexCopy = {};
    indexCopy.size = indexBufferSize;
    indexCopy.srcOffset = vertexBufferSize;
    indexCopy.dstOffset = 0;
    vkCmdCopyBuffer(cmd, staging.buffer, buffers.indexBuffer.buffer, 1, &indexCopy);
  });

  staging.cleanup(device->allocator());

  return buffers;
}

} // namespace utils
} // namespace bisky
