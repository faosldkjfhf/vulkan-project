#pragma once

namespace bisky {

class Callbacks {
public:
  virtual void onKey(int key, int scancode, int action, int mods) = 0;
  virtual void onResize(int width, int height) = 0;
  virtual void onClick(int button, int action, int mods) = 0;
  virtual void onMouseMove(double xpos, double ypos) = 0;
};

} // namespace bisky
