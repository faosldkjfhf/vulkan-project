#include "core/device.h"
#include "core/pipeline.h"
#include "core/window.h"
#include "pch.h"

#include <algorithm>
#include <limits>
#include <ranges>
#include <vulkan/vulkan_core.h>

#include "rendering/renderer.h"

namespace bisky {
namespace rendering {

Renderer::Renderer(core::Window &window, core::Device &device) : _window(window), _device(device) { initialize(); }

Renderer::~Renderer() {}

void Renderer::initialize() {
  createSwapchain();
  createImageViews();
  createRenderPass();
  createDepthResources();
  createFramebuffers();
  createCommandBuffers();
  createSyncObjects();
}

void Renderer::cleanup() {
  for (auto model : _models) {
    model.cleanup();
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(_device.device(), _imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(_device.device(), _renderFinishedSemaphores[i], nullptr);
    vkDestroyFence(_device.device(), _inFlightFences[i], nullptr);
  }

  vkDestroyRenderPass(_device.device(), _renderPass, nullptr);

  cleanupSwapchain();
}

void Renderer::cleanupSwapchain() {
  vmaDestroyImage(_device.allocator(), _depthImage, _depthAllocation);
  vkDestroyImageView(_device.device(), _depthImageView, nullptr);

  for (auto framebuffer : _framebuffers) {
    vkDestroyFramebuffer(_device.device(), framebuffer, nullptr);
  }

  for (auto imageView : _imageViews) {
    vkDestroyImageView(_device.device(), imageView, nullptr);
  }

  vkDestroySwapchainKHR(_device.device(), _swapchain, nullptr);
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
    glfwGetFramebufferSize(_window.window(), &width, &height);
    VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

void Renderer::createSwapchain() {
  SwapchainSupportDetails details = _device.querySwapchainSupport(_device.physicalDevice());

  VkSurfaceFormatKHR format = chooseSwapSurfaceFormat(details.formats);
  VkPresentModeKHR presentMode = chooseSwapPresentMode(details.presentModes);
  VkExtent2D extent = chooseSwapExtent(details.capabilities);

  uint32_t imageCount = details.capabilities.minImageCount + 1;
  if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount) {
    imageCount = details.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  createInfo.surface = _device.surface();
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = format.format;
  createInfo.imageColorSpace = format.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  createInfo.preTransform = details.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(_device.device(), &createInfo, nullptr, &_swapchain) != VK_SUCCESS) {
    throw std::runtime_error("failed to create swapchain");
  }

  vkGetSwapchainImagesKHR(_device.device(), _swapchain, &imageCount, nullptr);
  _images.resize(imageCount);
  vkGetSwapchainImagesKHR(_device.device(), _swapchain, &imageCount, _images.data());

  _format = format.format;
  _extent = extent;
}

void Renderer::createImageViews() {
  _imageViews.resize(_images.size());
  for (size_t i = 0; i < _images.size(); i++) {
    VkImageViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    createInfo.image = _images[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = _format;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(_device.device(), &createInfo, nullptr, &_imageViews[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create image views");
    }
  }
}

void Renderer::createRenderPass() {
  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = _format;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentDescription depthAttachment = {};
  depthAttachment.format = _device.findDepthFormat();
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef = {};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;

  std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
  VkRenderPassCreateInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(_device.device(), &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS) {
    throw std::runtime_error("failed to create render pass");
  }
}

void Renderer::createDepthResources() {
  VkFormat depthFormat = _device.findDepthFormat();

  createImage(_extent.width, _extent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _depthImage,
              _depthAllocation);

  VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  viewInfo.image = _depthImage;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = depthFormat;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(_device.device(), &viewInfo, nullptr, &_depthImageView) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image view");
  }

  transitionImageLayout(_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void Renderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                           VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image,
                           VmaAllocation &imageAllocation) {
  VkImageCreateInfo imageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.flags = 0;

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocInfo.flags = properties;
  allocInfo.priority = 1.0f;

  if (vmaCreateImage(_device.allocator(), &imageInfo, &allocInfo, &image, &imageAllocation, nullptr) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture image");
  }
}

void Renderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    if (hasStencilComponent(format)) {
      barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  } else {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } else {
    throw std::runtime_error("unsupported layout transition");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

  endSingleTimeCommands(commandBuffer);
}

void Renderer::createFramebuffers() {
  _framebuffers.resize(_imageViews.size());
  for (size_t i = 0; i < _imageViews.size(); i++) {
    std::array<VkImageView, 2> attachments = {_imageViews[i], _depthImageView};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = _renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = _extent.width;
    framebufferInfo.height = _extent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(_device.device(), &framebufferInfo, nullptr, &_framebuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffer");
    }
  }
}

void Renderer::createCommandBuffers() {
  _commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());
  allocInfo.commandPool = _device.commandPool();
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  if (vkAllocateCommandBuffers(_device.device(), &allocInfo, _commandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffer");
  }
}

void Renderer::createSyncObjects() {
  _imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  _renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  _inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

  VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(_device.device(), &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(_device.device(), &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(_device.device(), &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create semaphores");
    }
  }
}

void Renderer::addModel(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices) {
  _models.emplace_back(_device, vertices, indices);
}

void Renderer::addModel(const char *objPath) { _models.emplace_back(_device, objPath); }

VkCommandBuffer Renderer::beginRenderPass(uint32_t imageIndex) {
  VkCommandBuffer buffer = _commandBuffers[_currentFrame];
  vkResetCommandBuffer(buffer, 0);

  VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = nullptr;

  if (vkBeginCommandBuffer(buffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin command buffer");
  }

  VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  renderPassInfo.renderPass = _renderPass;
  renderPassInfo.framebuffer = _framebuffers[imageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = _extent;

  std::array<VkClearValue, 2> clearValues = {};
  clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};

  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  return buffer;
}

void Renderer::endRenderPass(VkCommandBuffer commandBuffer) {
  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to end command buffer");
  }

  VkSemaphore waitSemaphores[] = {_imageAvailableSemaphores[_currentFrame]};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSemaphore signalSemaphores[] = {_renderFinishedSemaphores[_currentFrame]};

  VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  if (vkQueueSubmit(_device.queue(), 1, &submitInfo, _inFlightFences[_currentFrame]) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer");
  }
}

VkCommandBuffer Renderer::beginSingleTimeCommands() {
  VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = _device.commandPool();
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(_device.device(), &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void Renderer::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(_device.queue(), 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(_device.queue());

  vkFreeCommandBuffers(_device.device(), _device.commandPool(), 1, &commandBuffer);
}

void Renderer::draw(core::Pipeline &pipeline, VkCommandBuffer commandBuffer) {
  for (auto model : _models) {
    model.bind(commandBuffer);
    model.draw(commandBuffer);

    // for (int i = 0; i < 4; i++) {
    //   PushConstants push = {};
    //   push.offset = {0.0f, -0.4f + i * 0.25f, 0.0f};
    //   push.color = {0.0f, 0.0f, 0.2f + i * 0.2f};
    //   vkCmdPushConstants(commandBuffer, pipeline.layout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
    //   0,
    //                      sizeof(push), &push);
    // model.draw(commandBuffer);
    // }
  }
}

void Renderer::setViewportAndScissor(VkCommandBuffer commandBuffer, VkViewport viewport, VkRect2D scissor) {
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

bool Renderer::acquireNextImage(uint32_t *imageIndex) {
  VkResult result = vkAcquireNextImageKHR(_device.device(), _swapchain, UINT64_MAX,
                                          _imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreate();
    return false;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire next swapchain image");
  }

  return true;
}

void Renderer::present(uint32_t imageIndex) {
  VkSemaphore signalSemaphores[] = {_renderFinishedSemaphores[_currentFrame]};
  VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapchains[] = {_swapchain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapchains;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = nullptr; // Optional

  VkResult result = vkQueuePresentKHR(_device.queue(), &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized) {
    _framebufferResized = false;
    recreate();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swapchain image");
  }
}

void Renderer::advanceFrame() { _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT; }

void Renderer::waitForFence() {
  vkWaitForFences(_device.device(), 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);
}

void Renderer::resetFence() { vkResetFences(_device.device(), 1, &_inFlightFences[_currentFrame]); }

void Renderer::recreate() {
  int width = 0, height = 0;
  glfwGetFramebufferSize(_window.window(), &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(_window.window(), &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(_device.device());

  cleanupSwapchain();

  createSwapchain();
  createImageViews();
  createDepthResources();
  createFramebuffers();
}

bool Renderer::hasStencilComponent(VkFormat format) {
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

} // namespace rendering
} // namespace bisky
