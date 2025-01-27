#include "core/device.h"

#include "core/model.h"

namespace bisky {
namespace core {

Model::Model(Device &device, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices)
    : _device(device) {
  createVertexBuffer(vertices);
  createIndexBuffer(indices);
}

Model::~Model() {}

void Model::cleanup() {
  vmaDestroyBuffer(_device.allocator(), _indexBuffer, _indexAllocation);
  vmaDestroyBuffer(_device.allocator(), _vertexBuffer, _vertexAllocation);
}

void Model::bind(VkCommandBuffer commandBuffer) {
  VkBuffer vertexBuffers[] = {_vertexBuffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(commandBuffer, _indexBuffer, 0, VK_INDEX_TYPE_UINT32);
}

void Model::draw(VkCommandBuffer commandBuffer) const { vkCmdDrawIndexed(commandBuffer, _indexCount, 1, 0, 0, 0); }

void Model::createVertexBuffer(const std::vector<Vertex> &vertices) {
  VkBuffer stagingBuffer;
  VmaAllocation stagingAllocation;
  VkDeviceSize size = sizeof(vertices[0]) * vertices.size();

  createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
               stagingBuffer, stagingAllocation);
  vmaCopyMemoryToAllocation(_device.allocator(), vertices.data(), stagingAllocation, 0, size);
  createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _vertexBuffer, _vertexAllocation);
  copyBuffer(stagingBuffer, _vertexBuffer, size);

  vmaDestroyBuffer(_device.allocator(), stagingBuffer, stagingAllocation);
}

void Model::createIndexBuffer(const std::vector<uint32_t> &indices) {
  _indexCount = static_cast<uint32_t>(indices.size());
  VkBuffer stagingBuffer;
  VmaAllocation stagingAllocation;
  VkDeviceSize size = sizeof(indices[0]) * indices.size();

  createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
               stagingBuffer, stagingAllocation);
  vmaCopyMemoryToAllocation(_device.allocator(), indices.data(), stagingAllocation, 0, size);
  createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _indexBuffer, _indexAllocation);
  copyBuffer(stagingBuffer, _indexBuffer, size);

  vmaDestroyBuffer(_device.allocator(), stagingBuffer, stagingAllocation);
}

void Model::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryAllocateFlags properties,
                         VkBuffer &buffer, VmaAllocation &allocation) {
  VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.usage = usage;
  bufferInfo.size = size;

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocInfo.flags = properties;

  if (vmaCreateBuffer(_device.allocator(), &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS) {
    throw std::runtime_error("failed to create buffer");
  }
}

void Model::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = _device.commandPool();
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(_device.device(), &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0; // Optional
  copyRegion.dstOffset = 0; // Optional
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(_device.queue(), 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(_device.queue());
  vkFreeCommandBuffers(_device.device(), _device.commandPool(), 1, &commandBuffer);
}

} // namespace core
} // namespace bisky
