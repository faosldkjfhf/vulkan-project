#include "core/device.h"
#include "core/window.h"
#include "rendering/renderer.h"
#include "utils/utils.h"

#include "core/compute_pipeline.h"

namespace bisky {
namespace core {

ComputePipeline::ComputePipeline(Pointer<Window> window, Pointer<Device> device, Pointer<rendering::Renderer> renderer)
    : _device(device), _window(window), _renderer(renderer) {
  slang::createGlobalSession(_globalSession.writeRef());
  initPipelines();
}

ComputePipeline::~ComputePipeline() {}

void ComputePipeline::cleanup() { _deletionQueue.flush(); }

void ComputePipeline::bind(VkCommandBuffer cmd) {
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipelineLayout, 0, 1,
                          &_renderer->drawImageDescriptors(), 0, nullptr);

  ComputePushConstants pc;
  pc.data1 = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
  pc.data2 = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

  vkCmdPushConstants(cmd, _pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &pc);
}

void ComputePipeline::executeDispatch(VkCommandBuffer cmd, uint32_t x, uint32_t y, uint32_t z) {
  vkCmdDispatch(cmd, x, y, z);
}

void ComputePipeline::initPipelines() { initBackgroundPipelines(); }

void ComputePipeline::initBackgroundPipelines() {
  VkPushConstantRange pushConstant = {};
  pushConstant.offset = 0;
  pushConstant.size = sizeof(ComputePushConstants);
  pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  VkPipelineLayoutCreateInfo computeLayout = {};
  computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  computeLayout.setLayoutCount = 1;
  computeLayout.pSetLayouts = &_renderer->drawImageLayout();
  computeLayout.pushConstantRangeCount = 1;
  computeLayout.pPushConstantRanges = &pushConstant;

  VK_CHECK(vkCreatePipelineLayout(_device->device(), &computeLayout, nullptr, &_pipelineLayout));
  _deletionQueue.push_back([&]() { vkDestroyPipelineLayout(_device->device(), _pipelineLayout, nullptr); });

  VkShaderModule computeShaderModule;
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
    slangModule = session->loadModule("../resources/shaders/compute/shader.slang", diagnosticsBlob.writeRef());

    if (diagnosticsBlob) {
      fprintf(stdout, "%s\n", (const char *)diagnosticsBlob->getBufferPointer());
    }

    if (!slangModule) {
      throw std::runtime_error("failed to compile slang module");
    }
  }
  if (!utils::loadShaderModule(session, slangModule, _device->device(), "computeMain", &computeShaderModule)) {
    fmt::println("error building shader module");
  }

  VkPipelineShaderStageCreateInfo stageInfo = {};
  stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  stageInfo.module = computeShaderModule;
  stageInfo.pName = "main";

  VkComputePipelineCreateInfo computePipelineCreateInfo = {};
  computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineCreateInfo.layout = _pipelineLayout;
  computePipelineCreateInfo.stage = stageInfo;

  VK_CHECK(
      vkCreateComputePipelines(_device->device(), VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &_pipeline));
  _deletionQueue.push_back([&]() { vkDestroyPipeline(_device->device(), _pipeline, nullptr); });

  vkDestroyShaderModule(_device->device(), computeShaderModule, nullptr);
}

} // namespace core
} // namespace bisky
