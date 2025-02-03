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

}; // namespace init
