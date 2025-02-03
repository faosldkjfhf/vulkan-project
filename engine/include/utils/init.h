#pragma once

#include "pch.h"

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

}; // namespace init
