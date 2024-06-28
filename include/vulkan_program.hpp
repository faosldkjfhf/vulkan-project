#pragma once

#include "glfw.hpp"
#include <vector>

class VulkanProgram {
public:
  VulkanProgram() = default;
  ~VulkanProgram();

  void initVulkan();

private:
  VkInstance _instance;
  const std::vector<const char *> _validationLayers = {
      "VK_LAYER_KHRONOS_validation",
  };

  void createInstance();
  const bool checkValidationLayerSupport();
};
