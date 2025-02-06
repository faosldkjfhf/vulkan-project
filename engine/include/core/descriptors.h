#pragma once

#include <span>

namespace bisky {
namespace core {

struct DescriptorLayoutBuilder {
  std::vector<VkDescriptorSetLayoutBinding> bindings;
  void add(uint32_t binding, VkDescriptorType type);
  void clear();
  VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, void *pNext = nullptr,
                              VkDescriptorSetLayoutCreateFlags flags = 0);
};

struct DescriptorAllocator {
  struct PoolSizeRatio {
    VkDescriptorType type;
    float ratio;
  };

  VkDescriptorPool pool;

  void initPool(VkDevice device, uint32_t maxSets, std::span<DescriptorAllocator::PoolSizeRatio> poolRatios);
  void clearDescriptors(VkDevice device);
  void destroyPool(VkDevice device);
  VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);
};

} // namespace core
} // namespace bisky
