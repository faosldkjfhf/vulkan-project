#include "gpu/gpu_mesh_buffers.h"

namespace bisky {

void GPUMeshBuffers::cleanup(VmaAllocator allocator) {
  vertexBuffer.cleanup(allocator);
  indexBuffer.cleanup(allocator);
}

} // namespace bisky
