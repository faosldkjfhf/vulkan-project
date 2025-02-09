#pragma once

#include "pch.h"

namespace bisky {

enum class MaterialPass : uint8_t { COLOR, TRANSPARENT, OTHER };

struct MaterialPipeline {
  VkPipeline pipeline;
  VkPipelineLayout layout;
};

struct MaterialInstance {
  Pointer<MaterialPipeline> pipeline;
  VkDescriptorSet materialSet;
  MaterialPass passType;
};

struct GPUObject {
  uint32_t indexCount;
  uint32_t firstIndex;

  Pointer<MaterialInstance> material;

  VkBuffer indexBuffer;
  VkDeviceAddress vertexBufferAddress;
};

} // namespace bisky
