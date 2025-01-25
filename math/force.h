#pragma once

namespace math {

class Force {
public:
  Force();
  virtual ~Force() = 0;

private:
  virtual void apply() = 0;
};

} // namespace math
