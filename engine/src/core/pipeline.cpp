#include "core/device.h"
#include "core/window.h"
#include "pch.h"
#include "rendering/renderer.h"

#include "stb_image.h"
#include <vulkan/vulkan_core.h>

#include "core/pipeline.h"

namespace bisky {
namespace core {

Pipeline::Pipeline(Pointer<Window> window, Pointer<Device> device, const char *file, const char *vertEntry,
                   const char *fragEntry, const Pipeline::Config &config)
    : _window(window), _device(device), _config(config) {
  initialize(file, vertEntry, fragEntry);
}

Pipeline::~Pipeline() {}

void Pipeline::initialize(const char *file, const char *vertEntry, const char *fragEntry) {
  // NOTE: Initialize slang global session
  slang::createGlobalSession(_globalSession.writeRef());

  createDescriptorSetLayout();
  createPipelineLayout();

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

  _config.shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

  createGraphicsPipeline();
  createTextureImage();
  createTextureImageView();
  createTextureImageSampler();
  createUniformBuffers();
  createDescriptorPool();
  createDescriptorSets();

  vkDestroyShaderModule(_device->device(), vertShaderModule, nullptr);
  vkDestroyShaderModule(_device->device(), fragShaderModule, nullptr);
}

void Pipeline::cleanup() {
  for (size_t i = 0; i < rendering::MAX_FRAMES_IN_FLIGHT; i++) {
    vmaUnmapMemory(_device->allocator(), _uniformBufferAllocations[i]);
    vmaDestroyBuffer(_device->allocator(), _uniformBuffers[i], _uniformBufferAllocations[i]);
  }

  vkDestroySampler(_device->device(), _textureSampler, nullptr);
  vkDestroyImageView(_device->device(), _textureImageView, nullptr);
  vmaDestroyImage(_device->allocator(), _textureImage, _textureAllocation);

  vkDestroyDescriptorPool(_device->device(), _descriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(_device->device(), _descriptorSetLayout, nullptr);
  vkDestroyPipelineLayout(_device->device(), _pipelineLayout, nullptr);
  vkDestroyPipeline(_device->device(), _graphicsPipeline, nullptr);
}

void Pipeline::bind(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1,
                          &_descriptorSets[imageIndex], 0, nullptr);
}

void Pipeline::createDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding uboLayoutBinding = {};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutBinding samplerBinding = {};
  samplerBinding.binding = 1;
  samplerBinding.descriptorCount = 1;
  samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerBinding};
  VkDescriptorSetLayoutCreateInfo layoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  VK_CHECK(vkCreateDescriptorSetLayout(_device->device(), &layoutInfo, nullptr, &_descriptorSetLayout));
}

void Pipeline::createPipelineLayout() {
  VkPushConstantRange pushConstantRange = {};
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(PushConstants);
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  VK_CHECK(vkCreatePipelineLayout(_device->device(), &pipelineLayoutInfo, nullptr, &_pipelineLayout));
}

void Pipeline::updateUniformBuffer(uint32_t imageIndex, void *data, size_t size) {
  memcpy(_uniformBuffersMapped[imageIndex], data, size);
}

void Pipeline::updateConfig(const Pipeline::Config &config) { _config = config; }

void Pipeline::addDescriptor(VkDescriptorType type, uint32_t count) { _descriptorPoolSizes.push_back({type, count}); }

void Pipeline::createGraphicsPipeline() {
  VkGraphicsPipelineCreateInfo pipelineInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  pipelineInfo.stageCount = static_cast<uint32_t>(_config.shaderStages.size());
  pipelineInfo.pStages = _config.shaderStages.data();
  pipelineInfo.pVertexInputState = &_config.vertexInput;
  pipelineInfo.pInputAssemblyState = &_config.inputAssembly;
  pipelineInfo.pViewportState = &_config.viewportState;
  pipelineInfo.pDynamicState = &_config.dynamicState;
  pipelineInfo.pColorBlendState = &_config.colorBlendState;
  pipelineInfo.pMultisampleState = &_config.multisampling;
  pipelineInfo.pRasterizationState = &_config.rasterizer;
  pipelineInfo.pDepthStencilState = &_config.depthStencil;
  pipelineInfo.renderPass = _config.renderPass;
  pipelineInfo.subpass = _config.subpass;
  pipelineInfo.layout = _pipelineLayout;

  VK_CHECK(vkCreateGraphicsPipelines(_device->device(), nullptr, 1, &pipelineInfo, nullptr, &_graphicsPipeline));
}

void Pipeline::createUniformBuffers() {
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);

  _uniformBuffers.resize(rendering::MAX_FRAMES_IN_FLIGHT);
  _uniformBufferAllocations.resize(rendering::MAX_FRAMES_IN_FLIGHT);
  _uniformBuffersMapped.resize(rendering::MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < rendering::MAX_FRAMES_IN_FLIGHT; i++) {
    _device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                          VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, _uniformBuffers[i],
                          _uniformBufferAllocations[i]);

    vmaMapMemory(_device->allocator(), _uniformBufferAllocations[i], &_uniformBuffersMapped[i]);
  }
}

void Pipeline::createTextureImage() {
  int width, height, channels;
  stbi_uc *pixels = stbi_load("../resources/textures/viking_room.png", &width, &height, &channels, STBI_rgb_alpha);
  VkDeviceSize imageSize = width * height * 4;
  _mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

  if (!pixels) {
    throw std::runtime_error("failed to load texture");
  }

  VkBuffer stagingBuffer;
  VmaAllocation stagingAllocation;
  _device->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer, stagingAllocation);
  vmaCopyMemoryToAllocation(_device->allocator(), pixels, stagingAllocation, 0, imageSize);
  stbi_image_free(pixels);

  _device->createImage(width, height, _mipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                       VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _textureImage, _textureAllocation);

  _device->transitionImageLayout(_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, _mipLevels);
  _device->copyBufferToImage(stagingBuffer, _textureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
  vmaDestroyBuffer(_device->allocator(), stagingBuffer, stagingAllocation);

  generateMipmaps(_textureImage, VK_FORMAT_R8G8B8A8_SRGB, width, height, _mipLevels);
}

void Pipeline::createTextureImageView() {
  _textureImageView = _device->createImageView(_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void Pipeline::createTextureImageSampler() {
  VkPhysicalDeviceProperties properties = {};
  vkGetPhysicalDeviceProperties(_device->physicalDevice(), &properties);

  VkSamplerCreateInfo samplerInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

  VK_CHECK(vkCreateSampler(_device->device(), &samplerInfo, nullptr, &_textureSampler));
}

void Pipeline::generateMipmaps(VkImage image, VkFormat format, int32_t width, int32_t height, uint32_t mipLevels) {
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(_device->physicalDevice(), format, &formatProperties);

  if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    throw std::runtime_error("texture image format does not support linear blitting");
  }

  VkCommandBuffer commandBuffer = _device->beginSingleTimeCommands();

  VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  barrier.image = image;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  int mipWidth = width;
  int mipHeight = height;

  for (uint32_t i = 1; i < mipLevels; i++) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &barrier);

    VkImageBlit blit = {};
    blit.srcOffsets[0] = {0, 0, 0};
    blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = {0, 0, 0};
    blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &barrier);

    if (mipWidth > 1)
      mipWidth /= 2;
    if (mipHeight > 1)
      mipHeight /= 2;
  }

  barrier.subresourceRange.baseMipLevel = mipLevels - 1;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);

  _device->endSingleTimeCommands(commandBuffer);
}

void Pipeline::createDescriptorPool() {
  // std::array<VkDescriptorPoolSize, 2> poolSizes = {};
  // poolSizes[0].descriptorCount = static_cast<uint32_t>(rendering::MAX_FRAMES_IN_FLIGHT);
  // poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  // poolSizes[1].descriptorCount = static_cast<uint32_t>(rendering::MAX_FRAMES_IN_FLIGHT);
  // poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

  VkDescriptorPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  poolInfo.pPoolSizes = _config.descriptorTypes.data();
  poolInfo.poolSizeCount = static_cast<uint32_t>(_config.descriptorTypes.size());
  poolInfo.maxSets = static_cast<uint32_t>(rendering::MAX_FRAMES_IN_FLIGHT);

  VK_CHECK(vkCreateDescriptorPool(_device->device(), &poolInfo, nullptr, &_descriptorPool));
}

void Pipeline::createDescriptorSets() {
  std::vector<VkDescriptorSetLayout> layouts(rendering::MAX_FRAMES_IN_FLIGHT, _descriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  allocInfo.descriptorPool = _descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(rendering::MAX_FRAMES_IN_FLIGHT);
  allocInfo.pSetLayouts = layouts.data();

  _descriptorSets.resize(rendering::MAX_FRAMES_IN_FLIGHT);
  VK_CHECK(vkAllocateDescriptorSets(_device->device(), &allocInfo, _descriptorSets.data()));

  for (size_t i = 0; i < rendering::MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = _uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.sampler = _textureSampler;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = _textureImageView;

    std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = _descriptorSets[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = _descriptorSets[i];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(_device->device(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(),
                           0, nullptr);
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
  if (vkCreateShaderModule(_device->device(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module");
  }

  return shaderModule;
}

} // namespace core
} // namespace bisky
