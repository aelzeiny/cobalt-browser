#include "third_party/rive/include/rive/generated/shapes/paint/gradient_stop_base.hpp"
#include "third_party/rive/include/rive/shapes/paint/gradient_stop.hpp"

using namespace rive;

Core* GradientStopBase::clone() const
{
  auto cloned = new GradientStop();
  cloned->copy(*this);
  return cloned;
}
