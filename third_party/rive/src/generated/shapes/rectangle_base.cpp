#include "third_party/rive/include/rive/generated/shapes/rectangle_base.hpp"
#include "third_party/rive/include/rive/shapes/rectangle.hpp"

using namespace rive;

Core* RectangleBase::clone() const
{
  auto cloned = new Rectangle();
  cloned->copy(*this);
  return cloned;
}
