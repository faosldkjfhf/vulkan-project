#pragma once

#include "pch.h"
#include <deque>

namespace bisky {
namespace core {

struct DescriptorWriter {
  std::deque<VkDescriptorImageInfo> imageInfos;
  std::deque<VkDescriptorBufferInfo> bufferInfos;
  Vector<VkWriteDescriptorSet> writes;

  void writeImage(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
  void writeBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);

  void clear();
  void updateSet(VkDevice device, VkDescriptorSet set);
};

} // namespace core
} // namespace bisky
