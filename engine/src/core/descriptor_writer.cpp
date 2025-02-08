#include "core/descriptor_writer.h"
#include <vulkan/vulkan_core.h>

namespace bisky {
namespace core {

void DescriptorWriter::writeImage(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout,
                                  VkDescriptorType type) {
  VkDescriptorImageInfo &info = imageInfos.emplace_back(VkDescriptorImageInfo{

      .sampler = sampler,
      .imageView = image,
      .imageLayout = layout,
  });

  VkWriteDescriptorSet write = {};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstBinding = binding;
  write.dstSet = VK_NULL_HANDLE;
  write.descriptorCount = 1;
  write.descriptorType = type;
  write.pImageInfo = &info;

  writes.push_back(write);
}

void DescriptorWriter::writeBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type) {
  VkDescriptorBufferInfo &info =
      bufferInfos.emplace_back(VkDescriptorBufferInfo{.buffer = buffer, .offset = offset, .range = size});
  VkWriteDescriptorSet write = {};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstBinding = binding;
  write.dstSet = VK_NULL_HANDLE;
  write.descriptorCount = 1;
  write.descriptorType = type;
  write.pBufferInfo = &info;

  writes.push_back(write);
}

void DescriptorWriter::clear() {
  imageInfos.clear();
  writes.clear();
  bufferInfos.clear();
}

void DescriptorWriter::updateSet(VkDevice device, VkDescriptorSet set) {
  for (VkWriteDescriptorSet &write : writes) {
    write.dstSet = set;
  }

  vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

} // namespace core
} // namespace bisky
