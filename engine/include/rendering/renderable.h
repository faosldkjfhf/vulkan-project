#pragma once

#include "gpu/gpu_object.h"
#include "pch.h"

namespace bisky {
namespace rendering {

struct RenderContext {
  Vector<GPUObject> objects;
};

class IRenderable {
  virtual void draw(const glm::mat4 &topMatrix, RenderContext &ctx) = 0;
};

} // namespace rendering
} // namespace bisky
