#include "core/device.h"

#include "core/immedate_submit.h"
#include "utils/init.h"

namespace bisky {
namespace core {

ImmediateSubmit::ImmediateSubmit(Pointer<Device> device) : _device(device) { initialize(); }

ImmediateSubmit::~ImmediateSubmit() {}

void ImmediateSubmit::cleanup() { _deletionQueue.flush(); }

void ImmediateSubmit::initialize() {
  // create command pool
  VkCommandPoolCreateInfo commandPoolInfo =
      init::commandPoolCreateInfo(_device->queueFamily(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
  VK_CHECK(vkCreateCommandPool(_device->device(), &commandPoolInfo, nullptr, &_commandPool));
  _deletionQueue.push_back([&]() { vkDestroyCommandPool(_device->device(), _commandPool, nullptr); });

  // create command buffer
  VkCommandBufferAllocateInfo allocInfo = init::commandBufferAllocateInfo(_commandPool);
  VK_CHECK(vkAllocateCommandBuffers(_device->device(), &allocInfo, &_commandBuffer));

  // create fence
  VkFenceCreateInfo fenceInfo = init::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
  VK_CHECK(vkCreateFence(_device->device(), &fenceInfo, nullptr, &_fence));
  _deletionQueue.push_back([&]() { vkDestroyFence(_device->device(), _fence, nullptr); });
}

void ImmediateSubmit::submit(std::function<void(VkCommandBuffer cmd)> &&function) {
  VK_CHECK(vkResetFences(_device->device(), 1, &_fence));
  VK_CHECK(vkResetCommandBuffer(_commandBuffer, 0));

  VkCommandBuffer cmd = _commandBuffer;

  VkCommandBufferBeginInfo cmdBeginInfo = init::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
  VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

  function(cmd);

  VK_CHECK(vkEndCommandBuffer(cmd));

  VkCommandBufferSubmitInfo cmdInfo = init::commandBufferSubmitInfo(cmd);
  VkSubmitInfo2 submitInfo = init::submitInfo(&cmdInfo, nullptr, nullptr);
  VK_CHECK(vkQueueSubmit2(_device->queue(), 1, &submitInfo, _fence));
  VK_CHECK(vkWaitForFences(_device->device(), 1, &_fence, true, UINT64_MAX));
}

} // namespace core
} // namespace bisky
