#pragma once

#include "pch.h"

namespace bisky {

class GPUBuffer {
public:
  GPUBuffer() = default;
  ~GPUBuffer() = default;

  struct Builder {
    VmaAllocator allocator;

    GPUBuffer build(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
  };

  void cleanup(VmaAllocator allocator);

  VkBuffer buffer;
  VmaAllocation allocation;
  VmaAllocationInfo info;
};

} // namespace bisky
