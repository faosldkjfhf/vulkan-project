#include "core/device.h"
#include "core/texture.h"

#include "core/model.h"

namespace bisky {
namespace core {

Model::Model(Pointer<Device> device, const Model::Builder &builder) : _device(device) {
  createVertexBuffer(builder.vertices);
  createIndexBuffer(builder.indices);
}

Model::Model(Pointer<Device> device, const char *objPath, const char *texturePath) : _device(device) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objPath)) {
    throw std::runtime_error(warn + err);
  }

  std::unordered_map<Vertex, uint32_t> uniqueVertices = {};
  for (const auto &shape : shapes) {
    for (const auto &index : shape.mesh.indices) {
      Vertex vertex = {};

      vertex.position = {attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],
                         attrib.vertices[3 * index.vertex_index + 2]};

      vertex.uv = {attrib.texcoords[2 * index.texcoord_index + 0],
                   1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

      vertex.color = {1.0f, 1.0f, 1.0f};

      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] = static_cast<uint32_t>(_vertices.size());
        _vertices.push_back(vertex);
      }

      _indices.push_back(uniqueVertices[vertex]);
    }
  }

  createVertexBuffer(_vertices);
  createIndexBuffer(_indices);
}

Model::~Model() {}

void Model::cleanup() {
  vmaDestroyBuffer(_device->allocator(), _vertexBuffer, _vertexAllocation);
  vmaDestroyBuffer(_device->allocator(), _indexBuffer, _indexAllocation);
}

void Model::addTexture(const char *texture) { _texture = std::make_shared<core::Texture>(_device, texture); }

void Model::bind(VkCommandBuffer commandBuffer) const {
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

  _device->createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                        stagingBuffer, stagingAllocation);
  vmaCopyMemoryToAllocation(_device->allocator(), vertices.data(), stagingAllocation, 0, size);
  _device->createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _vertexBuffer, _vertexAllocation);
  _device->copyBuffer(stagingBuffer, _vertexBuffer, size);

  vmaDestroyBuffer(_device->allocator(), stagingBuffer, stagingAllocation);
}

void Model::createIndexBuffer(const std::vector<uint32_t> &indices) {
  _indexCount = static_cast<uint32_t>(indices.size());
  VkBuffer stagingBuffer;
  VmaAllocation stagingAllocation;
  VkDeviceSize size = sizeof(indices[0]) * indices.size();

  _device->createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                        stagingBuffer, stagingAllocation);
  vmaCopyMemoryToAllocation(_device->allocator(), indices.data(), stagingAllocation, 0, size);
  _device->createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _indexBuffer, _indexAllocation);
  _device->copyBuffer(stagingBuffer, _indexBuffer, size);

  vmaDestroyBuffer(_device->allocator(), stagingBuffer, stagingAllocation);
}

} // namespace core
} // namespace bisky
