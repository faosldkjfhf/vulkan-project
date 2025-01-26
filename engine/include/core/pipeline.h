#pragma once

#include "pch.h"

#include <slang-com-ptr.h>
#include <slang.h>

namespace bisky {

namespace rendering {
class Renderer;
}

namespace core {

const std::vector<Vertex> vertices = {{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                                      {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
                                      {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                                      {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}};

const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};

class Window;
class Device;

struct PipelineInfo {
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  VkPipelineDynamicStateCreateInfo dynamicState;
  VkPipelineVertexInputStateCreateInfo vertexInputInfo;
  VkPipelineInputAssemblyStateCreateInfo inputAssembly;
  VkPipelineViewportStateCreateInfo viewportState;
  VkPipelineViewportStateCreateInfo rasterizer;
  VkPipelineMultisampleStateCreateInfo multisampling;
  VkPipelineColorBlendAttachmentState colorBlendAttachment;
  VkPipelineColorBlendStateCreateInfo colorBlending;
  VkPipelineLayoutCreateInfo pipelineLayoutInfo;
};

class Pipeline {
public:
  Pipeline(Window &window, Device &device, rendering::Renderer &renderer);
  ~Pipeline();

  VkPipelineLayout layout() { return _pipelineLayout; }
  VkPipeline pipeline() { return _graphicsPipeline; }
  size_t numIndices() { return indices.size(); }

  void cleanup();
  void bind(VkCommandBuffer commandBuffer, uint32_t imageIndex);

  void setDynamicState(std::vector<VkDynamicState> dynamicStates);
  void setVertexInputState(std::vector<VkVertexInputBindingDescription> bindings,
                           std::vector<VkVertexInputAttributeDescription> attributes);
  void setInputAssemblyState(VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                             VkBool32 primitiveRestartEnable = VK_FALSE);
  void setViewportState(uint32_t viewportCount = 1, uint32_t scissorCount = 1);
  void setRasterizer(VkPolygonMode polygonMode, VkCullModeFlagBits cullMode, VkFrontFace frontFace,
                     float lineWidth = 1.0f, VkBool32 depthBiasEnable = VK_FALSE);
  void setPipelineLayout(std::vector<VkDescriptorSetLayout> setLayouts,
                         std::vector<VkPushConstantRange> pushConstantRanges);

  void updateUniformBuffer(uint32_t imageIndex, void *data, size_t size);

private:
  void initialize();
  void createDescriptorSetLayout();
  void createGraphicsPipeline(const char *file, const char *vertEntry, const char *fragEntry);
  void createVertexBuffer();
  void createIndexBuffer();
  void createUniformBuffers();
  void createDescriptorPool();
  void createDescriptorSets();

  VkShaderModule createShaderModule(Slang::ComPtr<slang::ISession> session, slang::IModule *module,
                                    const char *entryPoint);
  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryAllocateFlags properties, VkBuffer &buffer,
                    VmaAllocation &allocation);
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

  Window &_window;
  Device &_device;
  rendering::Renderer &_renderer;

  VkPipelineLayout _pipelineLayout;
  VkPipeline _graphicsPipeline;
  PipelineInfo _pipelineInfo;

  VkDescriptorSetLayout _descriptorSetLayout;
  VkDescriptorPool _descriptorPool;
  std::vector<VkDescriptorSet> _descriptorSets;

  // vertex and index buffers
  VkBuffer _vertexBuffer;
  VmaAllocation _vertexAllocation;
  VkBuffer _indexBuffer;
  VmaAllocation _indexAllocation;

  // uniform buffers
  std::vector<VkBuffer> _uniformBuffers;
  std::vector<VmaAllocation> _uniformBufferAllocations;
  std::vector<void *> _uniformBuffersMapped;

  // slang global session for compiling
  Slang::ComPtr<slang::IGlobalSession> _globalSession;
};

} // namespace core
} // namespace bisky
