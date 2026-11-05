#include "third_party/rive/include/rive/generated/layout/axis_x_base.hpp"
#include "third_party/rive/include/rive/layout/axis_x.hpp"

using namespace rive;

Core* AxisXBase::clone() const
{
  auto cloned = new AxisX();
  cloned->copy(*this);
  return cloned;
}
