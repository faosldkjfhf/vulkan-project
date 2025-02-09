#pragma once

#include "core/deletion_queue.h"
#include "pch.h"

namespace bisky {
namespace core {

class Device;

class ImmediateSubmit {
public:
  ImmediateSubmit(Pointer<Device> device);
  ~ImmediateSubmit();

  void cleanup();
  void submit(std::function<void(VkCommandBuffer cmd)> &&function);

private:
  void initialize();

  Pointer<Device> _device;

  VkFence _fence;
  VkCommandBuffer _commandBuffer;
  VkCommandPool _commandPool;
  DeletionQueue _deletionQueue;
};

} // namespace core
} // namespace bisky
