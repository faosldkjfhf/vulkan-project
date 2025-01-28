#pragma once

#include "pch.h"

namespace bisky {
namespace core {

class Device;

class Model {
public:
  Model(Device &device, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);
  Model(Device &device, const char *objPath, const char *texturePath = nullptr);
  ~Model();

  void cleanup();

  void bind(VkCommandBuffer commandBuffer);
  void draw(VkCommandBuffer commandBuffer) const;

private:
  void createVertexBuffer(const std::vector<Vertex> &vertices);
  void createIndexBuffer(const std::vector<uint32_t> &indices);
  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryAllocateFlags properties, VkBuffer &buffer,
                    VmaAllocation &allocation);
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

  Device &_device;
  VkBuffer _vertexBuffer;
  VmaAllocation _vertexAllocation;
  VkBuffer _indexBuffer;
  VmaAllocation _indexAllocation;
  uint32_t _indexCount;

  std::vector<Vertex> _vertices;
  std::vector<uint32_t> _indices;
};

} // namespace core
} // namespace bisky
