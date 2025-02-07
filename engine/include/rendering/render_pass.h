#pragma once

#include "pch.h"

namespace bisky {
namespace rendering {

struct AttachmentInfo {
  std::string name;
  float size_x = 1.0f;
  float size_y = 1.0f;
  VkFormat format = VK_FORMAT_UNDEFINED;
  uint32_t samples = 1;
  uint32_t layers = 1;
  uint32_t levels = 1;
  bool persistent = false;
};

class RenderPass {
public:
  RenderPass();
  ~RenderPass();

  void addColorOutput(std::string name, AttachmentInfo attachment);
  void addAttachmentInput(std::string name);

  Vector<AttachmentInfo> attachments() { return _outputs; }

private:
  Vector<AttachmentInfo> _outputs;
  Vector<std::string> _inputs;
};

} // namespace rendering
} // namespace bisky
