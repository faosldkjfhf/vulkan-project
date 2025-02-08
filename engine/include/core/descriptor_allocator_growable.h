#pragma once

#include "pch.h"
#include <span>

namespace bisky {
namespace core {

class DescriptorAllocatorGrowable {
public:
  struct PoolSizeRatio {
    VkDescriptorType type;
    float ratio;
  };

  void init(VkDevice device, uint32_t initialSets, std::span<PoolSizeRatio> poolRatios);
  void clearPools(VkDevice device);
  void destroyPools(VkDevice device);

  VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout, void *pNext = nullptr);

private:
  VkDescriptorPool getPool(VkDevice device);
  VkDescriptorPool createPool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios);

  Vector<PoolSizeRatio> _ratios;
  Vector<VkDescriptorPool> _fullPools;
  Vector<VkDescriptorPool> _readyPools;
  uint32_t _setsPerPool;
};

} // namespace core
} // namespace bisky
