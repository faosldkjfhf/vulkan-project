#pragma once

#include "iforce.h"
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vk_enum_string_helper.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <fmt/core.h>
#include <tiny_obj_loader.h>
#include <vk_mem_alloc.h>

#define VK_CHECK(x)                                                                                                    \
  do {                                                                                                                 \
    VkResult err = x;                                                                                                  \
    if (err) {                                                                                                         \
      fmt::print("[ERROR]: {}", string_VkResult(err));                                                                 \
      abort();                                                                                                         \
    }                                                                                                                  \
  } while (0)

namespace bisky {
template <typename T> using Vector = std::vector<T>;
template <typename T> using Pointer = std::shared_ptr<T>;
} // namespace bisky

struct QueueFamilyIndices {
  std::optional<uint32_t> queueFamily;

  bool isComplete() { return queueFamily.has_value(); }
};

struct FrameData {
  VkCommandPool commandPool;
  VkCommandBuffer mainCommandBuffer;
};

constexpr uint32_t FRAME_OVERLAP = 2;

struct SwapchainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex {
  glm::vec3 position;
  glm::vec3 color;
  glm::vec2 uv;

  bool operator==(const Vertex &other) const {
    return position == other.position && color == other.color && uv == other.uv;
  }

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, uv);

    return attributeDescriptions;
  }
};

struct UniformBufferObject {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 projection;
};

struct PushConstants {
  alignas(16) glm::vec3 offset;
  alignas(16) glm::vec3 color;
};

namespace std {

template <> struct hash<Vertex> {
  size_t operator()(Vertex const &vertex) const {
    return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
           (hash<glm::vec2>()(vertex.uv) << 1);
  }
};

} // namespace std
