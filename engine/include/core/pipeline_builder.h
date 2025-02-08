#pragma once

#include "pch.h"

namespace bisky {
namespace core {

class PipelineBuilder {
public:
  PipelineBuilder();
  ~PipelineBuilder();

  void clear();

  PipelineBuilder &setColorAttachmentFormat(VkFormat format);
  PipelineBuilder &setDepthFormat(VkFormat format);
  PipelineBuilder &disableDepthTest();
  PipelineBuilder &disableBlending();
  PipelineBuilder &setMultisamplingNone();
  PipelineBuilder &setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
  PipelineBuilder &setPolygonMode(VkPolygonMode mode);
  PipelineBuilder &setInputTopology(VkPrimitiveTopology topology);
  PipelineBuilder &setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
  PipelineBuilder &enableDepthTest(bool depthWriteEnable, VkCompareOp op);
  PipelineBuilder &enableBlendingAdditive();
  PipelineBuilder &enableBlendingAlphaBlend();
  VkPipeline build(VkDevice device);

  Vector<VkPipelineShaderStageCreateInfo> shaderStages;
  VkPipelineInputAssemblyStateCreateInfo inputAssembly;
  VkPipelineRasterizationStateCreateInfo rasterizer;
  VkPipelineColorBlendAttachmentState colorBlendAttachment;
  VkPipelineMultisampleStateCreateInfo multisampling;
  VkPipelineLayout layout;
  VkPipelineDepthStencilStateCreateInfo depthStencil;
  VkPipelineRenderingCreateInfo renderInfo;
  VkFormat colorAttachmentFormat;

private:
};

} // namespace core
} // namespace bisky
