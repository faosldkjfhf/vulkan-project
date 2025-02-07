#pragma once

#include "pch.h"

namespace bisky {

class GPUBuffer {
public:
  struct Builder {
    VmaAllocator allocator;

    GPUBuffer build(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
  };

  friend class GPUMeshBuffers;

  void cleanup(VmaAllocator allocator);

  VkBuffer buffer;
  VmaAllocation allocation;
  VmaAllocationInfo info;

private:
  GPUBuffer() = default;
};

} // namespace bisky
