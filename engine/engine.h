#pragma once

#include "core/device.h"
#include "core/pipeline.h"
#include "core/window.h"
#include "icallbacks.h"
#include "pch.h"
#include "rendering/renderer.h"

namespace bisky {

class Engine : public ICallbacks {
public:
  Engine();
  ~Engine();

  void run();

private:
  void input();
  void update();
  void render();

  void createDefaultScene();

  void initialize();
  void cleanup();

  virtual void onKey(int key, int scancode, int action, int mods) override;
  virtual void onResize(int width, int height) override;
  virtual void onClick(int button, int action, int mods) override;
  virtual void onMouseMove(double xpos, double ypos) override;

  Pointer<core::Window> _window;
  Pointer<core::Device> _device;
  Pointer<rendering::Renderer> _renderer;
  Pointer<core::Pipeline> _pipeline;

  Vector<Pointer<core::Model>> _models;
};

} // namespace bisky
