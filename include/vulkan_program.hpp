#pragma once

#include "glfw.hpp"

class VulkanProgram {
public:
  VulkanProgram() = default;
  ~VulkanProgram();

  void initVulkan();
  void createInstance();

private:
  VkInstance _instance;
};
