#pragma once

#include "gpu/gpu_buffer.h"
#include "pch.h"

namespace bisky {

struct GPUMeshBuffers {
  GPUBuffer indexBuffer;
  GPUBuffer vertexBuffer;
  VkDeviceAddress vertexBufferAddress;

  void cleanup(VmaAllocator allocator);
};

} // namespace bisky
