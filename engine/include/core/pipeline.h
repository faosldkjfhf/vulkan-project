#pragma once

#include "pch.h"

#include <slang-com-ptr.h>
#include <slang.h>

namespace bisky {
namespace core {

class Window;
class Device;

class Pipeline {
public:
  struct Config {
    VkPipelineViewportStateCreateInfo viewportState;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    VkPipelineDynamicStateCreateInfo dynamicState;
    VkPipelineVertexInputStateCreateInfo vertexInput;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo colorBlendState;
    VkPipelineDepthStencilStateCreateInfo depthStencil;
    VkPipelineLayout pipelineLayout = nullptr;
    VkRenderPass renderPass = nullptr;
    uint32_t subpass = 0;
  };

  Pipeline(Pointer<Window> window, Pointer<Device> device, const char *file, const char *vertEntry,
           const char *fragEntry, const Pipeline::Config &config);
  ~Pipeline();

  void cleanup();
  void bind(VkCommandBuffer commandBuffer, uint32_t imageIndex);

  void updateUniformBuffer(uint32_t imageIndex, void *data, size_t size);
  void updateConfig(const Pipeline::Config &config);

  VkPipelineLayout layout() { return _pipelineLayout; }

private:
  void initialize(const char *file, const char *vertEntry, const char *fragEntry);
  void createDescriptorSetLayout();
  void createPipelineLayout();
  void createGraphicsPipeline();

  void createTextureImage();
  void createTextureImageView();
  void createTextureImageSampler();
  void generateMipmaps(VkImage image, VkFormat format, int32_t width, int32_t height, uint32_t mipLevels);

  // TODO: move to model?
  void createUniformBuffers();
  void createDescriptorPool();
  void createDescriptorSets();

  VkShaderModule createShaderModule(Slang::ComPtr<slang::ISession> session, slang::IModule *module,
                                    const char *entryPoint);

  Pointer<Window> _window;
  Pointer<Device> _device;

  VkPipelineLayout _pipelineLayout;
  VkPipeline _graphicsPipeline;
  Pipeline::Config _config;

  VkDescriptorSetLayout _descriptorSetLayout;
  VkDescriptorPool _descriptorPool;
  std::vector<VkDescriptorSet> _descriptorSets;

  // uniform buffers
  Vector<VkBuffer> _uniformBuffers;
  Vector<VmaAllocation> _uniformBufferAllocations;
  Vector<void *> _uniformBuffersMapped;

  uint32_t _mipLevels;
  VkImage _textureImage;
  VmaAllocation _textureAllocation;
  VkImageView _textureImageView;
  VkSampler _textureSampler;

  // slang global session for compiling
  Slang::ComPtr<slang::IGlobalSession> _globalSession;
};

} // namespace core
} // namespace bisky
