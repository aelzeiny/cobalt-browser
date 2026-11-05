#include "third_party/rive/include/rive/generated/animation/cubic_interpolator_component_base.hpp"
#include "third_party/rive/include/rive/animation/cubic_interpolator_component.hpp"

using namespace rive;

Core* CubicInterpolatorComponentBase::clone() const
{
  auto cloned = new CubicInterpolatorComponent();
  cloned->copy(*this);
  return cloned;
}
