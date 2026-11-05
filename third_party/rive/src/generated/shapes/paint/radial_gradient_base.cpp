#include "third_party/rive/include/rive/generated/shapes/paint/radial_gradient_base.hpp"
#include "third_party/rive/include/rive/shapes/paint/radial_gradient.hpp"

using namespace rive;

Core* RadialGradientBase::clone() const
{
  auto cloned = new RadialGradient();
  cloned->copy(*this);
  return cloned;
}
