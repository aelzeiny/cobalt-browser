#include "third_party/rive/include/rive/generated/shapes/paint/solid_color_base.hpp"
#include "third_party/rive/include/rive/shapes/paint/solid_color.hpp"

using namespace rive;

Core* SolidColorBase::clone() const
{
  auto cloned = new SolidColor();
  cloned->copy(*this);
  return cloned;
}
