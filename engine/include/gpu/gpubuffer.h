#pragma once

#include "pch.h"

namespace bisky {

class GPUBuffer {
public:
  struct Buffer {
    VkBuffer buffer;
    VmaAllocation allocation;
  };

  GPUBuffer();
  ~GPUBuffer();

private:
};

} // namespace bisky
