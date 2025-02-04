#pragma once

#include "pch.h"
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

}; // namespace init
