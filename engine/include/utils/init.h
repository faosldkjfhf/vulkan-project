#pragma once

#include "pch.h"
#include <slang-com-ptr.h>
#include <slang.h>
#include <vulkan/vulkan_core.h>

namespace init {

inline VkRenderingAttachmentInfo attachmentInfo(VkImageView view, VkClearValue *clear,
                                                VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
  VkRenderingAttachmentInfo colorAttachment = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
  colorAttachment.imageLayout = layout;
  colorAttachment.imageView = view;
  colorAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  if (clear) {
    colorAttachment.clearValue = *clear;
  }

  return colorAttachment;
}

inline VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0) {
  VkCommandPoolCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  info.queueFamilyIndex = queueFamilyIndex;
  info.flags = flags;
  info.pNext = nullptr;

  return info;
}

inline VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool commandPool, uint32_t count = 1) {
  VkCommandBufferAllocateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  info.commandBufferCount = count;
  info.commandPool = commandPool;
  info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  info.pNext = nullptr;

  return info;
}

inline VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0) {
  VkFenceCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  info.flags = flags;
  info.pNext = nullptr;

  return info;
}

inline VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0) {
  VkSemaphoreCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  info.flags = flags;
  info.pNext = nullptr;

  return info;
}

inline VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0) {
  VkCommandBufferBeginInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  info.flags = flags;
  info.pInheritanceInfo = nullptr;
  info.pNext = nullptr;

  return info;
}

inline VkImageSubresourceRange imageSubresourceRange(VkImageAspectFlags aspectMask) {
  VkImageSubresourceRange info = {};
  info.aspectMask = aspectMask;
  info.baseMipLevel = 0;
  info.levelCount = VK_REMAINING_MIP_LEVELS;
  info.baseArrayLayer = 0;
  info.layerCount = VK_REMAINING_ARRAY_LAYERS;

  return info;
}

inline VkSemaphoreSubmitInfo semaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore) {
  VkSemaphoreSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  submitInfo.semaphore = semaphore;
  submitInfo.stageMask = stageMask;
  submitInfo.deviceIndex = 0;
  submitInfo.value = 1;
  submitInfo.pNext = nullptr;

  return submitInfo;
}

inline VkCommandBufferSubmitInfo commandBufferSubmitInfo(VkCommandBuffer cmd) {
  VkCommandBufferSubmitInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  info.commandBuffer = cmd;
  info.deviceMask = 0;
  info.pNext = nullptr;

  return info;
}

inline VkSubmitInfo2 submitInfo(VkCommandBufferSubmitInfo *cmd, VkSemaphoreSubmitInfo *signalSemaphoreInfo,
                                VkSemaphoreSubmitInfo *waitSemaphoreInfo) {
  VkSubmitInfo2 submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  submitInfo.pNext = nullptr;

  submitInfo.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
  submitInfo.pWaitSemaphoreInfos = waitSemaphoreInfo;

  submitInfo.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
  submitInfo.pSignalSemaphoreInfos = signalSemaphoreInfo;

  submitInfo.commandBufferInfoCount = 1;
  submitInfo.pCommandBufferInfos = cmd;

  return submitInfo;
}

inline VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags flags, VkExtent3D extent) {
  VkImageCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  info.pNext = nullptr;

  info.imageType = VK_IMAGE_TYPE_2D;
  info.format = format;
  info.extent = extent;

  info.mipLevels = 1;
  info.arrayLayers = 1;
  info.samples = VK_SAMPLE_COUNT_1_BIT;

  info.tiling = VK_IMAGE_TILING_OPTIMAL;
  info.usage = flags;

  return info;
}

inline VkImageViewCreateInfo imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags flags) {
  VkImageViewCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  info.pNext = nullptr;

  info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  info.image = image;
  info.format = format;
  info.subresourceRange.baseMipLevel = 0;
  info.subresourceRange.levelCount = 1;
  info.subresourceRange.baseArrayLayer = 0;
  info.subresourceRange.layerCount = 1;
  info.subresourceRange.aspectMask = flags;

  return info;
}

inline VkRenderingInfo renderingInfo(VkExtent2D extent, VkRenderingAttachmentInfo *colorAttachment,
                                     VkRenderingAttachmentInfo *depthAttachment) {
  VkRenderingInfo renderInfo = {};
  renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderInfo.pNext = nullptr;

  renderInfo.renderArea = VkRect2D{VkOffset2D{0, 0}, extent};
  renderInfo.layerCount = 1;
  renderInfo.colorAttachmentCount = 1;
  renderInfo.pColorAttachments = colorAttachment;
  renderInfo.pDepthAttachment = depthAttachment;
  renderInfo.pStencilAttachment = nullptr;

  return renderInfo;
}

inline VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule module,
                                                                     const char *entry = "main") {
  VkPipelineShaderStageCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  info.stage = stage;
  info.module = module;
  info.pName = entry;

  return info;
}

inline VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo() {
  VkPipelineLayoutCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  info.flags = 0;
  info.setLayoutCount = 0;
  info.pSetLayouts = nullptr;
  info.pushConstantRangeCount = 0;
  info.pPushConstantRanges = nullptr;

  return info;
}

inline Slang::ComPtr<slang::ISession> createSession(Slang::ComPtr<slang::IGlobalSession> globalSession) {
  slang::SessionDesc sessionDesc = {};
  slang::TargetDesc targetDesc = {};
  targetDesc.format = SLANG_SPIRV;
  targetDesc.profile = globalSession->findProfile("spirv_1_5");
  targetDesc.flags = 0;

  sessionDesc.targets = &targetDesc;
  sessionDesc.targetCount = 1;

  std::vector<slang::CompilerOptionEntry> options;
  options.push_back(
      {slang::CompilerOptionName::EmitSpirvDirectly, {slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}});
  sessionDesc.compilerOptionEntries = options.data();
  sessionDesc.compilerOptionEntryCount = options.size();

  Slang::ComPtr<slang::ISession> session;
  if (!SLANG_SUCCEEDED(globalSession->createSession(sessionDesc, session.writeRef()))) {
    throw std::runtime_error("failed to create slang session");
  }

  return session;
}

inline VkRenderingAttachmentInfo depthAttachmentInfo(VkImageView imageView, VkImageLayout layout) {
  VkRenderingAttachmentInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  info.imageView = imageView;
  info.imageLayout = layout;
  info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  // using reverse - 0 is far, 1 is near
  info.clearValue.depthStencil.depth = 0.0f;

  return info;
}

}; // namespace init
