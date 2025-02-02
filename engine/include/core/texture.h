#pragma once

#include "pch.h"

namespace bisky {

namespace rendering {
class Renderer;
}

namespace core {

class Device;

class Texture {
public:
  Texture(Pointer<Device> device, const char *path);
  ~Texture();

  void createTextureImage(const char *path);
  void createTextureImageView();
  void createTextureSampler();
  void generateMipmaps(VkImage image, VkFormat format, int32_t width, int32_t height, uint32_t mipLevels);

private:
  Pointer<Device> _device;

  uint32_t _mipLevels = 0;
  VkImage _textureImage;
  VmaAllocation _textureAllocation;
  VkImageView _textureImageView;
  VkSampler _textureSampler;
};

} // namespace core
} // namespace bisky
