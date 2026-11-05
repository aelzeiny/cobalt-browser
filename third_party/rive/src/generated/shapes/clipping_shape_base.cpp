#include "third_party/rive/include/rive/generated/shapes/clipping_shape_base.hpp"
#include "third_party/rive/include/rive/shapes/clipping_shape.hpp"

using namespace rive;

Core* ClippingShapeBase::clone() const
{
  auto cloned = new ClippingShape();
  cloned->copy(*this);
  return cloned;
}
