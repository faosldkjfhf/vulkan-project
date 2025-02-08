#pragma once

#include "pch.h"

namespace bisky {

struct GPUSceneData {
  glm::mat4 view;
  glm::mat4 proj;
  glm::mat4 viewproj;
  glm::vec4 ambientColor;
  glm::vec4 sunlightDirection;
  glm::vec4 sunlightColor;
};

} // namespace bisky
