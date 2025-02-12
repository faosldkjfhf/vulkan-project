#pragma once

#include "core/compute_pipeline.h"
#include "core/descriptor_allocator_growable.h"
#include "core/descriptor_writer.h"
#include "core/device.h"
#include "core/mesh_loader.h"
#include "core/window.h"
#include "gpu/gpu_mesh_buffers.h"
#include "pch.h"
#include "rendering/renderer.h"
#include <slang-com-ptr.h>

#include "gpu/gpu_object.h"

namespace bisky {

struct GLTFMetallicObject {
  MaterialPipeline opaquePipeline;
  MaterialPipeline transparentPipeline;
  VkDescriptorSetLayout materialLayout;

  struct MaterialConstants {
    glm::vec4 colorFactors;
    glm::vec4 metalRoughFactors;
    glm::vec4 extra[14];
  };

  struct MaterialResources {
    AllocatedImage colorImage;
    VkSampler colorSampler;
    AllocatedImage metalRoughImage;
    VkSampler metalRoughSampler;
    VkBuffer dataBuffer;
    uint32_t dataOffset;
  };

  core::DescriptorWriter writer;

  void build(Engine *engine);
  void clear(VkDevice device);

  MaterialInstance writeMaterial(VkDevice device, MaterialPass pass, const MaterialResources &resources,
                                 core::DescriptorAllocatorGrowable &descriptorAllocator);
};

class Engine : public ICallbacks {
public:
  Engine();
  ~Engine();

  void run();

private:
  void input();
  void update();
  void render();

  void initialize();
  void initializeSlang();
  void cleanup();

  virtual void onKey(int key, int scancode, int action, int mods) override;
  virtual void onResize(int width, int height) override;
  virtual void onClick(int button, int action, int mods) override;
  virtual void onMouseMove(double xpos, double ypos) override;

  Pointer<core::Window> _window;
  Pointer<core::Device> _device;
  Pointer<rendering::Renderer> _renderer;

  Vector<ComputeEffect> _backgroundEffects;
  int _currentBackgroundEffect = 0;

  VkPipelineLayout _meshPipelineLayout;
  VkPipeline _meshPipeline;

  Vector<Pointer<MeshAsset>> _testMeshes;

  Slang::ComPtr<slang::IGlobalSession> _globalSession;
  Slang::ComPtr<slang::ISession> _session;
};

} // namespace bisky
