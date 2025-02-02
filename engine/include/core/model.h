#pragma once

#include "core/texture.h"
#include "pch.h"

namespace bisky {
namespace core {

class Device;

class Model {
public:
  struct Builder {
    Vector<Vertex> vertices;
    Vector<uint32_t> indices;
  };

  Model(Pointer<Device> device, const Model::Builder &builder);
  Model(Pointer<Device> device, const char *objPath, const char *texturePath = nullptr);
  ~Model();

  void addTexture(const char *texture);

  void cleanup();

  void bind(VkCommandBuffer commandBuffer) const;
  void draw(VkCommandBuffer commandBuffer) const;

private:
  void createVertexBuffer(const std::vector<Vertex> &vertices);
  void createIndexBuffer(const std::vector<uint32_t> &indices);

  Pointer<Device> _device;

  VkBuffer _vertexBuffer;
  VmaAllocation _vertexAllocation;
  VkBuffer _indexBuffer;
  VmaAllocation _indexAllocation;
  uint32_t _indexCount;

  Pointer<core::Texture> _texture;
  Vector<Vertex> _vertices;
  Vector<uint32_t> _indices;
};

} // namespace core
} // namespace bisky
