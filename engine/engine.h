#pragma once

#include "callbacks.h"
#include "core/device.h"
#include "core/pipeline.h"
#include "core/window.h"
#include "pch.h"
#include "rendering/renderer.h"

namespace bisky {

class Engine : public Callbacks {
public:
  Engine();
  ~Engine();

  void run();

private:
  void input();
  void update();
  void render();

  void initialize();
  void cleanup();

  virtual void onKey(int key, int scancode, int action, int mods) override;
  virtual void onResize(int width, int height) override;
  virtual void onClick(int button, int action, int mods) override;
  virtual void onMouseMove(double xpos, double ypos) override;

  core::Window _window{800, 600, "Bisky Engine", this};
  core::Device _device{_window};
  rendering::Renderer _renderer{_window, _device};
  core::Pipeline _pipeline{_window, _device, _renderer};
};

} // namespace bisky
