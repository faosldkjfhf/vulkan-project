#pragma once

#include "pch.h"

namespace bisky {
namespace core {

class PipelineBuilder {
public:
  PipelineBuilder();
  ~PipelineBuilder();

  void clear();

  void setColorAttachmentFormat(VkFormat format);
  void setDepthFormat(VkFormat format);
  void disableDepthTest();
  void disableBlending();
  void setMultisamplingNone();
  void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
  void setPolygonMode(VkPolygonMode mode);
  void setInputTopology(VkPrimitiveTopology topology);
  void setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
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
