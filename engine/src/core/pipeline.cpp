#include "core/device.h"
#include "core/window.h"
#include "pch.h"
#include "rendering/renderer.h"

#include "core/pipeline.h"

namespace bisky {
namespace core {

Pipeline::Pipeline(Window &window, Device &device, rendering::Renderer &renderer)
    : _window(window), _device(device), _renderer(renderer) {
  initialize();
}

Pipeline::~Pipeline() {}

void Pipeline::initialize() {
  // NOTE: Initialize slang global session
  slang::createGlobalSession(_globalSession.writeRef());

  createDescriptorSetLayout();
  createGraphicsPipeline("../resources/shaders/render/shader.slang", "vertMain", "fragMain");
  createUniformBuffers();
  createDescriptorPool();
  createDescriptorSets();
}

void Pipeline::cleanup() {
  for (size_t i = 0; i < rendering::MAX_FRAMES_IN_FLIGHT; i++) {
    vmaUnmapMemory(_device.allocator(), _uniformBufferAllocations[i]);
    vmaDestroyBuffer(_device.allocator(), _uniformBuffers[i], _uniformBufferAllocations[i]);
  }

  vkDestroyDescriptorPool(_device.device(), _descriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(_device.device(), _descriptorSetLayout, nullptr);
  vkDestroyPipeline(_device.device(), _graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(_device.device(), _pipelineLayout, nullptr);
}

void Pipeline::bind(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1,
                          &_descriptorSets[imageIndex], 0, nullptr);
}

void Pipeline::setDynamicState(std::vector<VkDynamicState> dynamicStates) {
  _pipelineInfo.dynamicState = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
  _pipelineInfo.dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  _pipelineInfo.dynamicState.pDynamicStates = dynamicStates.data();
}
void Pipeline::setVertexInputState(std::vector<VkVertexInputBindingDescription> bindings,
                                   std::vector<VkVertexInputAttributeDescription> attributes) {
  _pipelineInfo.vertexInputInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  _pipelineInfo.vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
  _pipelineInfo.vertexInputInfo.pVertexBindingDescriptions = bindings.data();
  _pipelineInfo.vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
  _pipelineInfo.vertexInputInfo.pVertexAttributeDescriptions = attributes.data();
}

void Pipeline::setInputAssemblyState(VkPrimitiveTopology primitiveTopology, VkBool32 primitiveRestartEnable) {}

void Pipeline::setViewportState(uint32_t viewportCount, uint32_t scissorCount) {}

void Pipeline::setRasterizer(VkPolygonMode polygonMode, VkCullModeFlagBits cullMode, VkFrontFace frontFace,
                             float lineWidth, VkBool32 depthBiasEnable) {}

void Pipeline::setPipelineLayout(std::vector<VkDescriptorSetLayout> setLayouts,
                                 std::vector<VkPushConstantRange> pushConstantRanges) {}

void Pipeline::createDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding uboLayoutBinding = {};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutCreateInfo layoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &uboLayoutBinding;

  if (vkCreateDescriptorSetLayout(_device.device(), &layoutInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout");
  }
}

void Pipeline::updateUniformBuffer(uint32_t imageIndex, void *data, size_t size) {
  memcpy(_uniformBuffersMapped[imageIndex], data, size);
}

void Pipeline::createGraphicsPipeline(const char *file, const char *vertEntry, const char *fragEntry) {
  // NOTE: Start a slang session
  slang::SessionDesc sessionDesc = {};
  slang::TargetDesc targetDesc = {};
  targetDesc.format = SLANG_SPIRV;
  targetDesc.profile = _globalSession->findProfile("spirv_1_5");
  targetDesc.flags = 0;

  sessionDesc.targets = &targetDesc;
  sessionDesc.targetCount = 1;

  std::vector<slang::CompilerOptionEntry> options;
  options.push_back(
      {slang::CompilerOptionName::EmitSpirvDirectly, {slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}});
  sessionDesc.compilerOptionEntries = options.data();
  sessionDesc.compilerOptionEntryCount = options.size();

  Slang::ComPtr<slang::ISession> session;
  if (!SLANG_SUCCEEDED(_globalSession->createSession(sessionDesc, session.writeRef()))) {
    throw std::runtime_error("failed to create slang session");
  }

  slang::IModule *slangModule = nullptr;
  {
    Slang::ComPtr<slang::IBlob> diagnosticsBlob;
    slangModule = session->loadModule(file, diagnosticsBlob.writeRef());

    if (diagnosticsBlob) {
      fprintf(stdout, "%s\n", (const char *)diagnosticsBlob->getBufferPointer());
    }

    if (!slangModule) {
      throw std::runtime_error("failed to compile slang module");
    }
  }

  VkShaderModule vertShaderModule = createShaderModule(session, slangModule, vertEntry);
  VkShaderModule fragShaderModule = createShaderModule(session, slangModule, fragEntry);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

  std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  auto bindingDescription = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)_renderer.extent().width;
  viewport.height = (float)_renderer.extent().height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = _renderer.extent();

  VkPipelineViewportStateCreateInfo viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f; // Optional
  rasterizer.depthBiasClamp = 0.0f;          // Optional
  rasterizer.depthBiasSlopeFactor = 0.0f;    // Optional

  VkPipelineMultisampleStateCreateInfo multisampling = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f;          // Optional
  multisampling.pSampleMask = nullptr;            // Optional
  multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
  multisampling.alphaToOneEnable = VK_FALSE;      // Optional

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // Optional

  VkPipelineColorBlendStateCreateInfo colorBlending = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f; // Optional
  colorBlending.blendConstants[1] = 0.0f; // Optional
  colorBlending.blendConstants[2] = 0.0f; // Optional
  colorBlending.blendConstants[3] = 0.0f; // Optional

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = nullptr;

  if (vkCreatePipelineLayout(_device.device(), &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout");
  }

  VkGraphicsPipelineCreateInfo pipelineInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  pipelineInfo.pStages = shaderStages.data();
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = nullptr; // Optional
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = _pipelineLayout;
  pipelineInfo.renderPass = _renderer.renderPass();
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
  pipelineInfo.basePipelineIndex = -1;              // Optional

  if (vkCreateGraphicsPipelines(_device.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_graphicsPipeline) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics pipeline");
  }

  vkDestroyShaderModule(_device.device(), vertShaderModule, nullptr);
  vkDestroyShaderModule(_device.device(), fragShaderModule, nullptr);
}

void Pipeline::createUniformBuffers() {
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);

  _uniformBuffers.resize(rendering::MAX_FRAMES_IN_FLIGHT);
  _uniformBufferAllocations.resize(rendering::MAX_FRAMES_IN_FLIGHT);
  _uniformBuffersMapped.resize(rendering::MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < rendering::MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                 _uniformBuffers[i], _uniformBufferAllocations[i]);

    vmaMapMemory(_device.allocator(), _uniformBufferAllocations[i], &_uniformBuffersMapped[i]);
  }
}

void Pipeline::createDescriptorPool() {
  VkDescriptorPoolSize poolSize = {};
  poolSize.descriptorCount = static_cast<uint32_t>(rendering::MAX_FRAMES_IN_FLIGHT);
  poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

  VkDescriptorPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.poolSizeCount = 1;
  poolInfo.maxSets = static_cast<uint32_t>(rendering::MAX_FRAMES_IN_FLIGHT);

  if (vkCreateDescriptorPool(_device.device(), &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool");
  }
}

void Pipeline::createDescriptorSets() {
  std::vector<VkDescriptorSetLayout> layouts(rendering::MAX_FRAMES_IN_FLIGHT, _descriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  allocInfo.descriptorPool = _descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(rendering::MAX_FRAMES_IN_FLIGHT);
  allocInfo.pSetLayouts = layouts.data();

  _descriptorSets.resize(rendering::MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateDescriptorSets(_device.device(), &allocInfo, _descriptorSets.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i < rendering::MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = _uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet descriptorWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptorWrite.dstSet = _descriptorSets[i];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;
    descriptorWrite.pImageInfo = nullptr;       // Optional
    descriptorWrite.pTexelBufferView = nullptr; // Optional

    vkUpdateDescriptorSets(_device.device(), 1, &descriptorWrite, 0, nullptr);
  }
}

VkShaderModule Pipeline::createShaderModule(Slang::ComPtr<slang::ISession> session, slang::IModule *module,
                                            const char *entryPoint) {
  Slang::ComPtr<slang::IEntryPoint> stageEntryPoint;
  module->findEntryPointByName(entryPoint, stageEntryPoint.writeRef());

  std::vector<slang::IComponentType *> componentTypes;
  componentTypes.push_back(module);
  componentTypes.push_back(stageEntryPoint);

  Slang::ComPtr<slang::IComponentType> composedProgram;
  {
    Slang::ComPtr<slang::IBlob> diagnosticsBlob;
    SlangResult result = session->createCompositeComponentType(componentTypes.data(), componentTypes.size(),
                                                               composedProgram.writeRef(), diagnosticsBlob.writeRef());

    if (diagnosticsBlob) {
      fprintf(stdout, "%s\n", (const char *)diagnosticsBlob->getBufferPointer());
    }

    if (!SLANG_SUCCEEDED(result)) {
      throw std::runtime_error("failed to compose program");
    }
  }

  Slang::ComPtr<slang::IBlob> spirvCode;
  {
    Slang::ComPtr<slang::IBlob> diagnosticsBlob;
    SlangResult result = composedProgram->getEntryPointCode(0, 0, spirvCode.writeRef(), diagnosticsBlob.writeRef());

    if (diagnosticsBlob) {
      fprintf(stdout, "%s\n", (const char *)diagnosticsBlob->getBufferPointer());
    }

    if (!SLANG_SUCCEEDED(result)) {
      throw std::runtime_error("failed to get vertex entry point");
    }
  }

  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = spirvCode->getBufferSize();
  createInfo.pCode = static_cast<const uint32_t *>(spirvCode->getBufferPointer());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(_device.device(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module");
  }

  return shaderModule;
}

void Pipeline::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryAllocateFlags properties,
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

void Pipeline::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
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
