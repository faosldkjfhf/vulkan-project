#pragma once

#include "pch.h"

#include <slang-com-ptr.h>
#include <slang.h>

namespace bisky {

namespace rendering {
class Renderer;
}

namespace core {

const std::vector<Vertex> vertices = {{{0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                                      {{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
                                      {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}};

class Window;
class Device;

class Pipeline {
public:
  Pipeline(Window &window, Device &device, rendering::Renderer &renderer);
  ~Pipeline();

  VkPipelineLayout layout() { return _layout; }
  VkPipeline pipeline() { return _graphicsPipeline; }

  void cleanup();
  void bind(VkCommandBuffer commandBuffer);

private:
  void initialize();
  void createGraphicsPipeline(const char *file, const char *vertEntry, const char *fragEntry);
  void createVertexBuffer();
  void createIndexBuffer();

  VkShaderModule createShaderModule(Slang::ComPtr<slang::ISession> session, slang::IModule *module,
                                    const char *entryPoint);
  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryAllocateFlags properties, VkBuffer &buffer,
                    VmaAllocation &allocation);

  Window &_window;
  Device &_device;
  rendering::Renderer &_renderer;

  VkPipelineLayout _layout;
  VkPipeline _graphicsPipeline;

  VkBuffer _vertexBuffer;
  VmaAllocation _vertexAllocation;

  Slang::ComPtr<slang::IGlobalSession> _globalSession;
};

} // namespace core
} // namespace bisky
