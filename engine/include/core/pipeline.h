#pragma once

#include "pch.h"

// #include <slang-com-ptr.h>
// #include <slang.h>

namespace bisky {

namespace rendering {

class Renderer;

}

namespace core {

class Window;
class Device;

class Pipeline {
public:
  Pipeline(Window &window, Device &device, rendering::Renderer &renderer);
  ~Pipeline();

  void cleanup();

private:
  void initialize();
  void createGraphicsPipeline(const char *file, const char *vertEntry, const char *fragEntry);

  VkShaderModule createShaderModule(Slang::ComPtr<slang::ISession> session, slang::IModule *module,
                                    const char *entryPoint);

  Window &_window;
  Device &_device;
  rendering::Renderer &_renderer;

  VkPipelineLayout _layout;
  VkPipeline _graphicsPipeline;

  Slang::ComPtr<slang::IGlobalSession> _globalSession;
};

} // namespace core
} // namespace bisky
