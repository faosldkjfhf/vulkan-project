#include "gpu/gpu_buffer.h"

namespace bisky {

GPUBuffer GPUBuffer::Builder::build(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
  GPUBuffer buffer;

  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = allocSize;
  bufferInfo.usage = usage;

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = memoryUsage;
  allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

  VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.info));
  return buffer;
}

void GPUBuffer::cleanup(VmaAllocator allocator) { vmaDestroyBuffer(allocator, buffer, allocation); }

} // namespace bisky
