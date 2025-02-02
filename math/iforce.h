#pragma once

namespace math {

class IForce {
public:
  IForce();
  virtual ~IForce() = 0;

private:
  virtual void apply() = 0;
};

} // namespace math
