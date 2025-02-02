#include "core/device.h"
#include "core/pipeline.h"
#include "core/window.h"
#include "pch.h"

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
  createSwapchain();
  createImageViews();
  createRenderPass();
  createDepthResources();
  createFramebuffers();
  createCommandBuffers();
  createSyncObjects();
}

void Renderer::cleanup() {
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(_device->device(), _imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(_device->device(), _renderFinishedSemaphores[i], nullptr);
    vkDestroyFence(_device->device(), _inFlightFences[i], nullptr);
  }

  vkDestroyRenderPass(_device->device(), _renderPass, nullptr);

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
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  createInfo.preTransform = details.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = _oldSwapchain;

  if (vkCreateSwapchainKHR(_device->device(), &createInfo, nullptr, &_swapchain) != VK_SUCCESS) {
    throw std::runtime_error("failed to create swapchain");
  }

  vkGetSwapchainImagesKHR(_device->device(), _swapchain, &imageCount, nullptr);
  _images.resize(imageCount);
  vkGetSwapchainImagesKHR(_device->device(), _swapchain, &imageCount, _images.data());

  _format = format.format;
  _extent = extent;

  _deletionQueue.push_back([&]() { vkDestroySwapchainKHR(_device->device(), _swapchain, nullptr); });
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
  depthAttachment.format = _device->findDepthFormat();
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

  VkSubpassDependency dependency = {};
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

  if (vkCreateRenderPass(_device->device(), &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS) {
    throw std::runtime_error("failed to create render pass");
  }
}

void Renderer::createDepthResources() {
  VkFormat depthFormat = _device->findDepthFormat();

  _device->createImage(_extent.width, _extent.height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                       VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _depthImage,
                       _depthAllocation);

  _depthImageView = _device->createImageView(_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

  _device->transitionImageLayout(_depthImage, depthFormat, 1, VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

  _deletionQueue.push_back([&]() { vkDestroyImageView(_device->device(), _depthImageView, nullptr); });
  _deletionQueue.push_back([&]() { vmaDestroyImage(_device->allocator(), _depthImage, _depthAllocation); });
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

    if (vkCreateFramebuffer(_device->device(), &framebufferInfo, nullptr, &_framebuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffer");
    }
  }

  for (auto &framebuffer : _framebuffers) {
    _deletionQueue.push_back([&]() { vkDestroyFramebuffer(_device->device(), framebuffer, nullptr); });
  }
}

void Renderer::createCommandBuffers() {
  _commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());
  allocInfo.commandPool = _device->commandPool();
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  if (vkAllocateCommandBuffers(_device->device(), &allocInfo, _commandBuffers.data()) != VK_SUCCESS) {
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
    if (vkCreateSemaphore(_device->device(), &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(_device->device(), &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(_device->device(), &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create semaphores");
    }
  }
}

VkCommandBuffer Renderer::beginRenderPass(uint32_t imageIndex) {
  VkCommandBuffer buffer = currentCommandBuffer();
  vkResetCommandBuffer(buffer, 0);

  VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = nullptr;

  if (vkBeginCommandBuffer(buffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin command buffer");
  }

  VkRenderPassBeginInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
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

  if (vkQueueSubmit(_device->queue(), 1, &submitInfo, _inFlightFences[_currentFrame]) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer");
  }
}

void Renderer::draw(VkCommandBuffer commandBuffer) {
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

void Renderer::setViewportAndScissor(VkCommandBuffer commandBuffer, VkViewport viewport, VkRect2D scissor) {
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

bool Renderer::acquireNextImage(uint32_t *imageIndex) {
  VkResult result = vkAcquireNextImageKHR(_device->device(), _swapchain, UINT64_MAX,
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

  VkResult result = vkQueuePresentKHR(_device->queue(), &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized) {
    _framebufferResized = false;
    recreate();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swapchain image");
  }
}

void Renderer::advanceFrame() { _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT; }

void Renderer::waitForFence() {
  vkWaitForFences(_device->device(), 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);
}

void Renderer::resetFence() { vkResetFences(_device->device(), 1, &_inFlightFences[_currentFrame]); }

void Renderer::recreate() {
  int width = 0, height = 0;
  glfwGetFramebufferSize(_window->window(), &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(_window->window(), &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(_device->device());

  _deletionQueue.flush();

  _oldSwapchain = _swapchain;
  createSwapchain();
  createImageViews();
  createDepthResources();
  createFramebuffers();
}

} // namespace rendering
} // namespace bisky
