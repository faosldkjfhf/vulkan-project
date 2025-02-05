#include "core/descriptors.h"
#include <span>

namespace bisky {
namespace core {

void DescriptorLayoutBuilder::add(uint32_t binding, VkDescriptorType type) {
  VkDescriptorSetLayoutBinding newbind = {};
  newbind.descriptorType = type;
  newbind.binding = binding;
  newbind.descriptorCount = 1;

  bindings.push_back(newbind);
}

void DescriptorLayoutBuilder::clear() { bindings.clear(); }

VkDescriptorSetLayout DescriptorLayoutBuilder::build(VkDevice device, VkShaderStageFlags shaderStages, void *pNext,
                                                     VkDescriptorSetLayoutCreateFlags flags) {
  for (auto &b : bindings) {
    b.stageFlags |= shaderStages;
  }

  VkDescriptorSetLayoutCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.pNext = pNext;

  info.pBindings = bindings.data();
  info.bindingCount = static_cast<uint32_t>(bindings.size());
  info.flags = flags;

  VkDescriptorSetLayout set;
  VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

  return set;
}

void DescriptorAllocator::initPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios) {
  Vector<VkDescriptorPoolSize> poolSizes;
  for (auto &ratio : poolRatios) {
    poolSizes.push_back(
        VkDescriptorPoolSize{.type = ratio.type, .descriptorCount = static_cast<uint32_t>(ratio.ratio * maxSets)});
  }

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = maxSets;
  poolInfo.flags = 0;

  VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool));
}

void DescriptorAllocator::clearDescriptors(VkDevice device) { vkResetDescriptorPool(device, pool, 0); }

void DescriptorAllocator::destroyPool(VkDevice device) { vkDestroyDescriptorPool(device, pool, nullptr); }

VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout) {
  VkDescriptorSetAllocateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  info.descriptorPool = pool;
  info.descriptorSetCount = 1;
  info.pSetLayouts = &layout;

  VkDescriptorSet ds;
  VK_CHECK(vkAllocateDescriptorSets(device, &info, &ds));

  return ds;
}

} // namespace core
} // namespace bisky
