#pragma once

#include "gpu/gpu_buffer.h"

namespace bisky {

struct GPUMeshBuffers {
  GPUBuffer indexBuffer;
  GPUBuffer vertexBuffer;
  VkDeviceAddress vertexBufferAddress;

  void cleanup(VmaAllocator allocator);
};

} // namespace bisky
