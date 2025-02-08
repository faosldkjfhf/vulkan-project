#include "core/compute_pipeline.h"
#include "core/descriptor_allocator_growable.h"
#include "core/descriptor_writer.h"
#include "core/descriptors.h"
#include "core/device.h"
#include "core/immedate_submit.h"
#include "core/window.h"
#include "gpu/gpu_buffer.h"
#include "gpu/gpu_mesh_buffers.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "pch.h"
#include "utils/init.h"
#include "utils/utils.h"

#include <algorithm>
#include <limits>
#include <vulkan/vulkan_core.h>

#include "rendering/renderer.h"

namespace bisky {
namespace rendering {

Renderer::Renderer(Pointer<core::Window> window, Pointer<core::Device> device, VkSwapchainKHR oldSwapchain)
    : _window(window), _device(device), _oldSwapchain(oldSwapchain) {
  initialize();
}

Renderer::~Renderer() {}

void Renderer::initialize() {
  _immediateSubmit = std::make_shared<core::ImmediateSubmit>(_device);

  createSwapchain();
  createImageViews();
  initializeCommands();
  initializeSyncStructures();
  initializeDescriptors();
  initializeDefaultData();
  initializeImgui();
}

void Renderer::initializeDefaultData() {
  uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
  _whiteImage = createImage((void *)&white, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

  uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1.0f));
  _greyImage = createImage((void *)&grey, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

  uint32_t black = glm::packUnorm4x8(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
  _blackImage = createImage((void *)&black, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

  uint32_t magenta = glm::packUnorm4x8(glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
  std::array<uint32_t, 16 * 16> pixels;
  for (int x = 0; x < 16; x++) {
    for (int y = 0; y < 16; y++) {
      pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
    }
  }
  _errorCheckerboardImage =
      createImage(pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

  VkSamplerCreateInfo sample = {};
  sample.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sample.magFilter = VK_FILTER_NEAREST;
  sample.minFilter = VK_FILTER_NEAREST;
  VK_CHECK(vkCreateSampler(_device->device(), &sample, nullptr, &_defaultSamplerNearest));

  sample.magFilter = VK_FILTER_LINEAR;
  sample.minFilter = VK_FILTER_LINEAR;
  VK_CHECK(vkCreateSampler(_device->device(), &sample, nullptr, &_defaultSamplerLinear));

  _deletionQueue.push_back([&]() {
    vkDestroySampler(_device->device(), _defaultSamplerLinear, nullptr);
    vkDestroySampler(_device->device(), _defaultSamplerNearest, nullptr);
    _whiteImage.cleanup(_device->device(), _device->allocator());
    _blackImage.cleanup(_device->device(), _device->allocator());
    _greyImage.cleanup(_device->device(), _device->allocator());
    _errorCheckerboardImage.cleanup(_device->device(), _device->allocator());
  });
}

void Renderer::initializeImgui() {
  VkDescriptorPoolSize poolSizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};
  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.maxSets = 1000;
  poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
  poolInfo.pPoolSizes = poolSizes;

  VK_CHECK(vkCreateDescriptorPool(_device->device(), &poolInfo, nullptr, &_imguiPool));

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  ImGui_ImplGlfw_InitForVulkan(_window->window(), true);

  ImGui_ImplVulkan_InitInfo initInfo = {};
  initInfo.Device = _device->device();
  initInfo.Instance = _device->instance();
  initInfo.PhysicalDevice = _device->physicalDevice();
  initInfo.Queue = _device->queue();
  initInfo.DescriptorPool = _imguiPool;
  initInfo.MinImageCount = 3;
  initInfo.ImageCount = 3;
  initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  initInfo.UseDynamicRendering = true;

  initInfo.PipelineRenderingCreateInfo = {};
  initInfo.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
  initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &_format;

  ImGui_ImplVulkan_Init(&initInfo);
  ImGui_ImplVulkan_CreateFontsTexture();
}

void Renderer::cleanup() {
  for (auto &frame : _frames) {
    vkDestroySemaphore(_device->device(), frame.renderSemaphore, nullptr);
    vkDestroySemaphore(_device->device(), frame.swapchainSemaphore, nullptr);
    vkDestroyFence(_device->device(), frame.renderFence, nullptr);
    vkDestroyCommandPool(_device->device(), frame.commandPool, nullptr);

    frame.deletionQueue.flush();
  }

  _immediateSubmit->cleanup();

  ImGui_ImplVulkan_Shutdown();
  vkDestroyDescriptorPool(_device->device(), _imguiPool, nullptr);
  vkDestroyDescriptorSetLayout(_device->device(), _singleImageDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(_device->device(), _gpuSceneDescriptorLayout, nullptr);

  _deletionQueue.flush();
}

VkSurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
  for (const auto &availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

VkPresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
  for (const auto &availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(_window->window(), &width, &height);
    VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

void Renderer::createSwapchain() {
  SwapchainSupportDetails details = _device->querySwapchainSupport(_device->physicalDevice());

  VkSurfaceFormatKHR format = chooseSwapSurfaceFormat(details.formats);
  VkPresentModeKHR presentMode = chooseSwapPresentMode(details.presentModes);
  VkExtent2D extent = chooseSwapExtent(details.capabilities);

  uint32_t imageCount = details.capabilities.minImageCount + 1;
  if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount) {
    imageCount = details.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  createInfo.surface = _device->surface();
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = format.format;
  createInfo.imageColorSpace = format.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  createInfo.preTransform = details.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = _oldSwapchain;

  VK_CHECK(vkCreateSwapchainKHR(_device->device(), &createInfo, nullptr, &_swapchain));

  vkGetSwapchainImagesKHR(_device->device(), _swapchain, &imageCount, nullptr);
  _images.resize(imageCount);
  vkGetSwapchainImagesKHR(_device->device(), _swapchain, &imageCount, _images.data());

  _format = format.format;
  _extent = extent;

  _deletionQueue.push_back([&]() { vkDestroySwapchainKHR(_device->device(), _swapchain, nullptr); });

  VkExtent3D drawImageExtent = {_extent.width, _extent.height, 1};

  _drawImage.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  _drawImage.extent = drawImageExtent;

  VkImageUsageFlags drawImageUsages = {};
  drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
  drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  VkImageCreateInfo imgInfo = init::imageCreateInfo(_drawImage.format, drawImageUsages, drawImageExtent);
  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK(
      vmaCreateImage(_device->allocator(), &imgInfo, &allocInfo, &_drawImage.image, &_drawImage.allocation, nullptr));

  VkImageViewCreateInfo viewInfo =
      init::imageViewCreateInfo(_drawImage.format, _drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
  VK_CHECK(vkCreateImageView(_device->device(), &viewInfo, nullptr, &_drawImage.imageView));

  _deletionQueue.push_back([&]() {
    vkDestroyImageView(_device->device(), _drawImage.imageView, nullptr);
    vmaDestroyImage(_device->allocator(), _drawImage.image, _drawImage.allocation);
  });

  _depthImage.format = VK_FORMAT_D32_SFLOAT;
  _depthImage.extent = _drawImage.extent;
  VkImageUsageFlags depthImageUsages = {};
  depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

  VkImageCreateInfo depthImageInfo = init::imageCreateInfo(_depthImage.format, depthImageUsages, _depthImage.extent);
  vmaCreateImage(_device->allocator(), &depthImageInfo, &allocInfo, &_depthImage.image, &_depthImage.allocation,
                 nullptr);
  VkImageViewCreateInfo depthViewInfo =
      init::imageViewCreateInfo(_depthImage.format, _depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);
  VK_CHECK(vkCreateImageView(_device->device(), &depthViewInfo, nullptr, &_depthImage.imageView));

  _deletionQueue.push_back([&]() {
    vkDestroyImageView(_device->device(), _depthImage.imageView, nullptr);
    vmaDestroyImage(_device->allocator(), _depthImage.image, _depthImage.allocation);
  });
}

void Renderer::createImageViews() {
  _imageViews.resize(_images.size());
  for (size_t i = 0; i < _images.size(); i++) {
    _imageViews[i] = _device->createImageView(_images[i], _format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
  }

  for (auto &imageView : _imageViews) {
    _deletionQueue.push_back([&]() { vkDestroyImageView(_device->device(), imageView, nullptr); });
  }
}

// void Renderer::createRenderPass() {
// VkAttachmentDescription colorAttachment = {};
// colorAttachment.format = _format;
// colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
// colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
// colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
// colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
// colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
// colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
// colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
//
// VkAttachmentDescription depthAttachment = {};
// depthAttachment.format = _device->findDepthFormat();
// depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
// depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
// depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
// depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
// depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
// depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
// depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
// VkAttachmentReference depthAttachmentRef = {};
// depthAttachmentRef.attachment = 1;
// depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
// VkSubpassDependency dependency = {};
// dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
// dependency.dstSubpass = 0;
// dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
// VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT; dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
// dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
// VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT; dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
// VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
//
// VkAttachmentReference colorAttachmentRef = {};
// colorAttachmentRef.attachment = 0;
// colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
// VkSubpassDescription subpass = {};
// subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
// subpass.colorAttachmentCount = 1;
// subpass.pColorAttachments = &colorAttachmentRef;
// subpass.pDepthStencilAttachment = &depthAttachmentRef;
//
// std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
// VkRenderPassCreateInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
// renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
// renderPassInfo.pAttachments = attachments.data();
// renderPassInfo.subpassCount = 1;
// renderPassInfo.pSubpasses = &subpass;
// renderPassInfo.dependencyCount = 1;
// renderPassInfo.pDependencies = &dependency;
//
// VK_CHECK(vkCreateRenderPass(_device->device(), &renderPassInfo, nullptr, &_renderPass));
// }

// void Renderer::createDepthResources() {
// VkFormat depthFormat = _device->findDepthFormat();
//
// _device->createImage(_extent.width, _extent.height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL,
//                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _depthImage,
//                      _depthAllocation);
//
// _depthImageView = _device->createImageView(_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
//
// _device->transitionImageLayout(_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
//                                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
//
// _deletionQueue.push_back([&]() { vkDestroyImageView(_device->device(), _depthImageView, nullptr); });
// _deletionQueue.push_back([&]() { vmaDestroyImage(_device->allocator(), _depthImage, _depthAllocation); });
// }

// void Renderer::createFramebuffers() {
// _framebuffers.resize(_imageViews.size());
// for (size_t i = 0; i < _imageViews.size(); i++) {
//   std::array<VkImageView, 2> attachments = {_imageViews[i], _depthImageView};
//
//   VkFramebufferCreateInfo framebufferInfo{};
//   framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
//   framebufferInfo.renderPass = _renderPass;
//   framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
//   framebufferInfo.pAttachments = attachments.data();
//   framebufferInfo.width = _extent.width;
//   framebufferInfo.height = _extent.height;
//   framebufferInfo.layers = 1;
//
//   VK_CHECK(vkCreateFramebuffer(_device->device(), &framebufferInfo, nullptr, &_framebuffers[i]));
// }
//
// for (auto &framebuffer : _framebuffers) {
//   _deletionQueue.push_back([&]() { vkDestroyFramebuffer(_device->device(), framebuffer, nullptr); });
// }
// }

void Renderer::initializeCommands() {
  VkCommandPoolCreateInfo commandPoolInfo =
      init::commandPoolCreateInfo(_device->queueFamily(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  for (uint32_t i = 0; i < FRAME_OVERLAP; i++) {
    VK_CHECK(vkCreateCommandPool(_device->device(), &commandPoolInfo, nullptr, &_frames[i].commandPool));
    VkCommandBufferAllocateInfo allocInfo = init::commandBufferAllocateInfo(_frames[i].commandPool);
    VK_CHECK(vkAllocateCommandBuffers(_device->device(), &allocInfo, &_frames[i].mainCommandBuffer));
  }
}

AllocatedImage Renderer::createImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped) {
  AllocatedImage image = {};
  image.format = format;
  image.extent = size;

  VkImageCreateInfo imgInfo = init::imageCreateInfo(format, usage, size);
  if (mipmapped) {
    imgInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
  }

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK(vmaCreateImage(_device->allocator(), &imgInfo, &allocInfo, &image.image, &image.allocation, nullptr));

  VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
  if (format == VK_FORMAT_D32_SFLOAT) {
    aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
  }

  VkImageViewCreateInfo viewInfo = init::imageViewCreateInfo(format, image.image, aspectFlag);
  viewInfo.subresourceRange.levelCount = imgInfo.mipLevels;
  VK_CHECK(vkCreateImageView(_device->device(), &viewInfo, nullptr, &image.imageView));

  return image;
}

AllocatedImage Renderer::createImage(void *data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage,
                                     bool mipmapped) {
  size_t dataSize = size.depth * size.width * size.height * 4;

  GPUBuffer::Builder builder;
  GPUBuffer uploadBuffer =
      builder.build(_device->allocator(), dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  memcpy(uploadBuffer.info.pMappedData, data, dataSize);

  AllocatedImage image =
      createImage(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

  _immediateSubmit->submit([&](VkCommandBuffer cmd) {
    utils::transitionImage(cmd, image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy copyRegion = {};
    copyRegion.bufferOffset = 0;
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;

    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageExtent = size;

    vkCmdCopyBufferToImage(cmd, uploadBuffer.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    utils::transitionImage(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  });

  uploadBuffer.cleanup(_device->allocator());

  return image;
}

void Renderer::initializeSyncStructures() {
  VkFenceCreateInfo fenceInfo = init::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
  VkSemaphoreCreateInfo semaphoreInfo = init::semaphoreCreateInfo();

  for (uint32_t i = 0; i < FRAME_OVERLAP; i++) {
    VK_CHECK(vkCreateSemaphore(_device->device(), &semaphoreInfo, nullptr, &_frames[i].renderSemaphore));
    VK_CHECK(vkCreateSemaphore(_device->device(), &semaphoreInfo, nullptr, &_frames[i].swapchainSemaphore));
    VK_CHECK(vkCreateFence(_device->device(), &fenceInfo, nullptr, &_frames[i].renderFence));
  }
}

void Renderer::initializeDescriptors() {
  Vector<core::DescriptorAllocator::PoolSizeRatio> sizes = {{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}};

  _globalDescriptorAllocator.initPool(_device->device(), 10, sizes);

  {
    core::DescriptorLayoutBuilder builder;
    _drawImageDescriptorLayout =
        builder.add(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE).build(_device->device(), VK_SHADER_STAGE_COMPUTE_BIT);
    _drawImageDescriptors = _globalDescriptorAllocator.allocate(_device->device(), _drawImageDescriptorLayout);
  }
  {
    core::DescriptorLayoutBuilder builder;
    _gpuSceneDescriptorLayout =
        builder.add(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            .build(_device->device(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
  }
  {
    core::DescriptorLayoutBuilder builder;
    _singleImageDescriptorLayout = builder.add(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                                       .build(_device->device(), VK_SHADER_STAGE_FRAGMENT_BIT);
  }

  core::DescriptorWriter writer;
  writer.writeImage(0, _drawImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
  writer.updateSet(_device->device(), _drawImageDescriptors);

  for (int i = 0; i < FRAME_OVERLAP; i++) {
    Vector<core::DescriptorAllocatorGrowable::PoolSizeRatio> frameSizes = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
    };

    _frames[i].frameDescriptors = core::DescriptorAllocatorGrowable{};
    _frames[i].frameDescriptors.init(_device->device(), 1000, frameSizes);

    _deletionQueue.push_back([&, i]() { _frames[i].frameDescriptors.destroyPools(_device->device()); });
  }

  _deletionQueue.push_back([&]() {
    _globalDescriptorAllocator.destroyPool(_device->device());
    vkDestroyDescriptorSetLayout(_device->device(), _drawImageDescriptorLayout, nullptr);
  });
}

VkCommandBuffer Renderer::beginRenderPass() {
  VkCommandBuffer commandBuffer = currentCommandBuffer();
  VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));

  VkCommandBufferBeginInfo beginInfo = init::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
  VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

  return commandBuffer;
}

void Renderer::endRenderPass(VkCommandBuffer commandBuffer) {
  VK_CHECK(vkEndCommandBuffer(commandBuffer));

  VkCommandBufferSubmitInfo cmdInfo = init::commandBufferSubmitInfo(commandBuffer);
  VkSemaphoreSubmitInfo signalInfo =
      init::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, getCurrentFrame().renderSemaphore);
  VkSemaphoreSubmitInfo waitInfo =
      init::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, getCurrentFrame().swapchainSemaphore);

  VkSubmitInfo2 submitInfo = init::submitInfo(&cmdInfo, &signalInfo, &waitInfo);

  VK_CHECK(vkQueueSubmit2(_device->queue(), 1, &submitInfo, getCurrentFrame().renderFence));
}

void Renderer::clear(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
  // transition our swapchain image to a general layout
  utils::transitionImage(commandBuffer, _images[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

  // clear the swapchain image
  VkClearColorValue clearValue = {};
  clearValue = {{0.0f, 1.0f, 0.0f, 1.0f}};
  VkImageSubresourceRange clearRange = init::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
  vkCmdClearColorImage(commandBuffer, _images[imageIndex], VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
}

void Renderer::draw(VkCommandBuffer commandBuffer, ComputeEffect &effect, VkPipelineLayout layout,
                    VkPipeline graphicsPipeline, Vector<Pointer<MeshAsset>> meshes, uint32_t imageIndex) {

  _drawExtent.height = std::min(_extent.height, _drawImage.extent.height) * _renderScale;
  _drawExtent.width = std::min(_extent.width, _drawImage.extent.width) * _renderScale;

  utils::transitionImage(commandBuffer, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

  // transition our swapchain image to a general layout
  utils::transitionImage(commandBuffer, _images[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

  // clear the swapchain image
  VkClearColorValue clearValue = {};
  clearValue = {{0.0f, 1.0f, 0.0f, 1.0f}};
  VkImageSubresourceRange clearRange = init::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

  // bind the compute pipeline for drawing
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, effect.layout, 0, 1, &_drawImageDescriptors, 0,
                          nullptr);
  vkCmdPushConstants(commandBuffer, effect.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(effect.data), &effect.data);
  vkCmdDispatch(commandBuffer, std::ceil(_extent.width / 16), std::ceil(_extent.height / 16), 1);

  utils::transitionImage(commandBuffer, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  utils::transitionImage(commandBuffer, _depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

  drawGeometry(commandBuffer, layout, graphicsPipeline, meshes);

  utils::transitionImage(commandBuffer, _drawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  utils::transitionImage(commandBuffer, _images[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  // execute a copy from the draw image onto the swapchain
  utils::copyImageToImage(commandBuffer, _drawImage.image, _images[imageIndex], _drawExtent, _extent);

  // set swapchain to color attachment optimal
  utils::transitionImage(commandBuffer, _images[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  // draw imgui
  drawImgui(commandBuffer, _imageViews[imageIndex]);

  // set swapchain to present
  utils::transitionImage(commandBuffer, _images[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                         VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

void Renderer::drawGeometry(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkPipeline pipeline,
                            Vector<Pointer<MeshAsset>> meshes) {
  VkRenderingAttachmentInfo colorAttachment =
      init::attachmentInfo(_drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
  VkRenderingAttachmentInfo depthAttachment =
      init::depthAttachmentInfo(_depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
  VkRenderingInfo renderInfo = init::renderingInfo(_drawExtent, &colorAttachment, &depthAttachment);
  vkCmdBeginRendering(commandBuffer, &renderInfo);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  VkViewport viewport = {};
  viewport.x = 0;
  viewport.y = 0;
  viewport.width = _drawExtent.width;
  viewport.height = _drawExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent.width = _drawExtent.width;
  scissor.extent.height = _drawExtent.height;

  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  VkDescriptorSet imageSet =
      getCurrentFrame().frameDescriptors.allocate(_device->device(), _singleImageDescriptorLayout);
  {
    core::DescriptorWriter writer;
    writer.writeImage(0, _errorCheckerboardImage.imageView, _defaultSamplerNearest,
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.updateSet(_device->device(), imageSet);
  }

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &imageSet, 0, nullptr);

  glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat4 projection =
      glm::perspective(glm::radians(70.0f), (float)_drawExtent.width / _drawExtent.height, 100.0f, 0.1f);
  projection[1][1] *= -1;

  // GPUBuffer::Builder builder;
  // GPUBuffer gpuSceneDataBuffer = builder.build(_device->allocator(), sizeof(GPUSceneData),
  //                                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
  // getCurrentFrame().deletionQueue.push_back([&]() { gpuSceneDataBuffer.cleanup(_device->allocator()); });

  // GPUSceneData *sceneUniformData = (GPUSceneData *)gpuSceneDataBuffer.info.pMappedData;
  // *sceneUniformData = _sceneData;
  // VkDescriptorSet globalDescriptor =
  //     getCurrentFrame().frameDescriptors.allocate(_device->device(), _gpuSceneDescriptorLayout);
  //
  // core::DescriptorWriter writer;
  // writer.writeBuffer(0, gpuSceneDataBuffer.buffer, sizeof(gpuSceneDataBuffer), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
  // writer.updateSet(_device->device(), globalDescriptor);

  GPUPushConstants pushConstants = {};
  pushConstants.worldMatrix = projection * view;
  pushConstants.vertexBuffer = meshes[2]->meshBuffers.vertexBufferAddress;

  vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);
  vkCmdBindIndexBuffer(commandBuffer, meshes[2]->meshBuffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(commandBuffer, meshes[2]->surfaces[0].count, 1, meshes[2]->surfaces[0].startIndex, 0, 0);

  vkCmdEndRendering(commandBuffer);
}

void Renderer::drawImgui(VkCommandBuffer commandBuffer, VkImageView target) {
  VkRenderingAttachmentInfo colorAttachment = init::attachmentInfo(target, nullptr);
  VkRenderingInfo renderInfo = init::renderingInfo(_extent, &colorAttachment, nullptr);

  vkCmdBeginRendering(commandBuffer, &renderInfo);

  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

  vkCmdEndRendering(commandBuffer);
}

void Renderer::setViewportAndScissor(VkCommandBuffer commandBuffer, VkViewport viewport, VkRect2D scissor) {
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

bool Renderer::acquireNextImage(uint32_t *imageIndex) {
  VkResult result = vkAcquireNextImageKHR(_device->device(), _swapchain, UINT64_MAX,
                                          getCurrentFrame().swapchainSemaphore, VK_NULL_HANDLE, imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreate();
    return false;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire next swapchain image");
  }

  return true;
}

void Renderer::present(uint32_t imageIndex) {
  VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &getCurrentFrame().renderSemaphore;

  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &_swapchain;
  presentInfo.pImageIndices = &imageIndex;

  VkResult result = vkQueuePresentKHR(_device->queue(), &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized) {
    _framebufferResized = false;
    recreate();
    return;
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swapchain image");
  }

  _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::waitForFence() {
  vkWaitForFences(_device->device(), 1, &getCurrentFrame().renderFence, VK_TRUE, UINT64_MAX);
}

void Renderer::resetFence() { vkResetFences(_device->device(), 1, &getCurrentFrame().renderFence); }

void Renderer::recreate() {
  int width = 0, height = 0;
  glfwGetFramebufferSize(_window->window(), &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(_window->window(), &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(_device->device());

  _deletionQueue.flush();

  createSwapchain();
  createImageViews();
  initializeDescriptors();
}

} // namespace rendering
} // namespace bisky
