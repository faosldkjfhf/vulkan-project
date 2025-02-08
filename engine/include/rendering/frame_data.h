#pragma once

#include "core/deletion_queue.h"
#include "core/descriptor_allocator_growable.h"
#include "pch.h"

namespace bisky {
namespace rendering {

struct FrameData {
  VkCommandPool commandPool;
  VkCommandBuffer mainCommandBuffer;
  VkSemaphore swapchainSemaphore;
  VkSemaphore renderSemaphore;
  VkFence renderFence;

  core::DescriptorAllocatorGrowable frameDescriptors;
  core::DeletionQueue deletionQueue;
};

} // namespace rendering
} // namespace bisky
