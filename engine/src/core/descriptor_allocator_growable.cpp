#include "core/descriptor_allocator_growable.h"

namespace bisky {
namespace core {

void DescriptorAllocatorGrowable::init(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios) {
  _ratios.clear();

  for (auto r : poolRatios) {
    _ratios.push_back(r);
  }

  VkDescriptorPool pool = createPool(device, maxSets, poolRatios);
  _setsPerPool = maxSets * 1.5;
  _readyPools.push_back(pool);
}

void DescriptorAllocatorGrowable::clearPools(VkDevice device) {
  for (auto p : _readyPools) {
    vkResetDescriptorPool(device, p, 0);
  }

  for (auto p : _fullPools) {
    vkResetDescriptorPool(device, p, 0);
    _readyPools.push_back(p);
  }

  _fullPools.clear();
}

void DescriptorAllocatorGrowable::destroyPools(VkDevice device) {
  for (auto p : _readyPools) {
    vkDestroyDescriptorPool(device, p, nullptr);
  }
  _readyPools.clear();

  for (auto p : _fullPools) {
    vkDestroyDescriptorPool(device, p, nullptr);
  }
  _fullPools.clear();
}

VkDescriptorSet DescriptorAllocatorGrowable::allocate(VkDevice device, VkDescriptorSetLayout layout, void *pNext) {
  VkDescriptorPool pool = getPool(device);

  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.pNext = pNext;
  allocInfo.descriptorPool = pool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &layout;

  VkDescriptorSet set;
  VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &set);

  if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
    _fullPools.push_back(pool);
    pool = getPool(device);
    allocInfo.descriptorPool = pool;

    VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &set));
  }

  _readyPools.push_back(pool);
  return set;
}

VkDescriptorPool DescriptorAllocatorGrowable::getPool(VkDevice device) {
  VkDescriptorPool pool;

  if (_readyPools.size() != 0) {
    pool = _readyPools.back();
    _readyPools.pop_back();
  }

  else {
    pool = createPool(device, _setsPerPool, _ratios);
    _setsPerPool = _setsPerPool * 1.5;
    if (_setsPerPool > 4092) {
      _setsPerPool = 4092;
    }
  }

  return pool;
}

VkDescriptorPool DescriptorAllocatorGrowable::createPool(VkDevice device, uint32_t setCount,
                                                         std::span<PoolSizeRatio> poolRatios) {
  Vector<VkDescriptorPoolSize> sizes;
  for (PoolSizeRatio ratio : poolRatios) {
    sizes.push_back(VkDescriptorPoolSize{.type = ratio.type, .descriptorCount = uint32_t(ratio.ratio * setCount)});
  }

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags = 0;
  poolInfo.maxSets = setCount;
  poolInfo.poolSizeCount = static_cast<uint32_t>(sizes.size());
  poolInfo.pPoolSizes = sizes.data();

  VkDescriptorPool pool;
  VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool));

  return pool;
}

} // namespace core
} // namespace bisky
