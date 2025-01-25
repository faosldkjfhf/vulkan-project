#include "core/device.h"
#include "core/window.h"
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

  createGraphicsPipeline("../resources/shaders/render/shader.slang", "vertMain", "fragMain");
  createVertexBuffer();
  createIndexBuffer();
}

void Pipeline::cleanup() {
  vmaDestroyBuffer(_device.allocator(), _indexBuffer, _indexAllocation);
  vmaDestroyBuffer(_device.allocator(), _vertexBuffer, _vertexAllocation);
  vkDestroyPipeline(_device.device(), _graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(_device.device(), _layout, nullptr);
}

void Pipeline::bind(VkCommandBuffer commandBuffer) {
  VkBuffer vertexBuffers[] = {_vertexBuffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(commandBuffer, _indexBuffer, 0, VK_INDEX_TYPE_UINT16);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);
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
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
  pipelineLayoutInfo.setLayoutCount = 0;            // Optional
  pipelineLayoutInfo.pSetLayouts = nullptr;         // Optional
  pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
  pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

  if (vkCreatePipelineLayout(_device.device(), &pipelineLayoutInfo, nullptr, &_layout) != VK_SUCCESS) {
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
  pipelineInfo.layout = _layout;
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

void Pipeline::createVertexBuffer() {
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

void Pipeline::createIndexBuffer() {
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
