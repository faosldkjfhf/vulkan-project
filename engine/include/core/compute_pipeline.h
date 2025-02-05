#pragma once
#include "core/deletion_queue.h"
#include "pch.h"
#include <slang-com-ptr.h>

namespace bisky {

namespace rendering {
class Renderer;
}

namespace core {

class Window;
class Device;

class ComputePipeline {
public:
  ComputePipeline(Pointer<Window> window, Pointer<Device> device, Pointer<rendering::Renderer> renderer);
  ~ComputePipeline();

  void cleanup();

  void bind(VkCommandBuffer cmd);
  void executeDispatch(VkCommandBuffer cmd, uint32_t x, uint32_t y, uint32_t z);

private:
  void initPipelines();
  void initBackgroundPipelines();

  Pointer<Window> _window;
  Pointer<Device> _device;
  Pointer<rendering::Renderer> _renderer;
  Slang::ComPtr<slang::IGlobalSession> _globalSession;

  VkPipeline _pipeline;
  VkPipelineLayout _pipelineLayout;
  DeletionQueue _deletionQueue;
};

} // namespace core
} // namespace bisky
