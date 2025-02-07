#pragma once

#include "pch.h"
#include "render_pass.h"

namespace bisky {
namespace rendering {

class RenderGraph {
public:
  struct Builder {
    RenderPass &addPass(const char *name, VkPipelineStageFlagBits stages);
    RenderGraph build();
  };

private:
  std::unordered_map<const char *, RenderPass> _renderPasses;
};

} // namespace rendering
} // namespace bisky
