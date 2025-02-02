#include "engine.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "rendering/renderer.h"

namespace bisky {

Engine::Engine() { initialize(); }

Engine::~Engine() { cleanup(); }

void Engine::initialize() {
  _window = std::make_shared<core::Window>(800, 600, "Bisky Engine", this);
  _device = std::make_shared<core::Device>(_window);
  _renderer = std::make_shared<rendering::Renderer>(_window, _device);

  core::Pipeline::Config config = {};
  config.inputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  config.inputAssembly.primitiveRestartEnable = VK_FALSE;
  config.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  uint32_t count = rendering::MAX_FRAMES_IN_FLIGHT;
  config.descriptorTypes = {
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count},
  };

  config.viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  config.viewportState.viewportCount = 1;
  config.viewportState.scissorCount = 1;

  std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT};
  config.dynamicState = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
  config.dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  config.dynamicState.pDynamicStates = dynamicStates.data();

  config.rasterizer = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  config.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  config.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  config.rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  config.rasterizer.lineWidth = 1.0f;
  config.rasterizer.rasterizerDiscardEnable = VK_FALSE;
  config.rasterizer.depthBiasEnable = VK_FALSE;
  config.rasterizer.depthClampEnable = VK_FALSE;

  config.multisampling = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  config.multisampling.sampleShadingEnable = VK_FALSE;
  config.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  config.colorBlendAttachment = {};
  config.colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  config.colorBlendAttachment.blendEnable = VK_FALSE;

  config.colorBlendState = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  config.colorBlendState.attachmentCount = 1;
  config.colorBlendState.pAttachments = &config.colorBlendAttachment;
  config.colorBlendState.logicOpEnable = VK_FALSE;

  auto bindingDescription = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  config.vertexInput = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  config.vertexInput.vertexBindingDescriptionCount = 1;
  config.vertexInput.pVertexBindingDescriptions = &bindingDescription;
  config.vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
  config.vertexInput.pVertexAttributeDescriptions = attributeDescriptions.data();

  config.depthStencil = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
  config.depthStencil.depthTestEnable = VK_TRUE;
  config.depthStencil.depthWriteEnable = VK_TRUE;
  config.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  config.depthStencil.depthBoundsTestEnable = VK_FALSE;
  config.depthStencil.minDepthBounds = 0.0f;
  config.depthStencil.maxDepthBounds = 1.0f;
  config.depthStencil.stencilTestEnable = VK_FALSE;
  config.depthStencil.front = {};
  config.depthStencil.back = {};

  config.pipelineLayout = nullptr;
  config.renderPass = _renderer->renderPass();
  config.subpass = 0;

  _pipeline = std ::make_shared<core::Pipeline>(_window, _device, "../resources/shaders/render/test.slang", "vertMain",
                                                "fragMain", config);

  createDefaultScene();

  // IMGUI_CHECKVERSION();
  // ImGui::CreateContext();
  // ImGuiIO &io = ImGui::GetIO();
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  //
  // ImGui_ImplGlfw_InitForVulkan(_window->window(), true);
  // ImGui_ImplVulkan_InitInfo initInfo = {};
  // initInfo.Instance = _device->instance();
  // initInfo.PhysicalDevice = _device->physicalDevice();
  // initInfo.Device = _device->device();
  // initInfo.QueueFamily = _device->indices().queueFamily.value();
  // initInfo.Queue = _device->queue();
  // initInfo.PipelineCache = VK_NULL_HANDLE;
  // initInfo.DescriptorPool = _pipeline->descriptorPool();
  // initInfo.Subpass = 0;
  // initInfo.MinImageCount = 2;
  // initInfo.ImageCount = rendering::MAX_FRAMES_IN_FLIGHT;
  // initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  // initInfo.Allocator = VK_NULL_HANDLE;
  // initInfo.CheckVkResultFn = nullptr;
  // initInfo.RenderPass = _renderer->renderPass();
  // ImGui_ImplVulkan_Init(&initInfo);
  //
  // VkCommandBuffer commandBuffer = _device->beginSingleTimeCommands();
  // ImGui_ImplVulkan_CreateFontsTexture();
  // _device->endSingleTimeCommands(commandBuffer);
  //
  // ImGui_ImplVulkan_DestroyFontsTexture();
}

void Engine::createDefaultScene() {
  core::Model::Builder builder;
  builder.vertices = {
      {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
      {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
      {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
      {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
  };
  builder.indices = {0, 1, 2, 2, 3, 0};

  _models.emplace_back(std::make_shared<core::Model>(_device, builder));
}

void Engine::cleanup() {
  for (auto model : _models) {
    model->cleanup();
  }

  _pipeline->cleanup();
  _renderer->cleanup();
  _device->cleanup();
  _window->cleanup();
}

void Engine::run() {
  while (!_window->shouldClose()) {
    input();
    update();
    render();
  }

  vkDeviceWaitIdle(_device->device());
}

void Engine::input() { glfwPollEvents(); }

void Engine::update() {
  UniformBufferObject ubo;
  ubo.model = glm::mat4(1.0f);
  ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.projection = glm::perspective(glm::radians(45.0f), _renderer->aspectRatio(), 0.1f, 10.0f);
  ubo.projection[1][1] *= -1;
  _pipeline->updateUniformBuffer(_renderer->currentFrame(), (void *)&ubo, sizeof(ubo));
}

void Engine::render() {
  // reset fences and wait for next fence
  _renderer->waitForFence();

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(_renderer->extent().width);
  viewport.height = static_cast<float>(_renderer->extent().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = _renderer->extent();

  // try to acquire the next image
  uint32_t imageIndex;
  if (_renderer->acquireNextImage(&imageIndex)) {
    _renderer->resetFence();
  } else {
    return;
  }

  // begin the render pass
  VkCommandBuffer commandBuffer = _renderer->beginRenderPass(imageIndex);

  // submit commands
  _pipeline->bind(commandBuffer, _renderer->currentFrame());
  _renderer->setViewportAndScissor(commandBuffer, viewport, scissor);

  for (auto model : _models) {
    model->bind(commandBuffer);
    model->draw(commandBuffer);
  }

  // end command buffer and render pass
  _renderer->endRenderPass(commandBuffer);

  // submit to present queue
  _renderer->present(imageIndex);
  _renderer->advanceFrame();
}

void Engine::onKey(int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    _window->setShouldClose();
  }
}

void Engine::onResize(int width, int height) { _renderer->setFramebufferResized(true); }

void Engine::onClick(int button, int action, int mods) {}

void Engine::onMouseMove(double xpos, double ypos) {}

} // namespace bisky
