#include "third_party/rive/include/rive/generated/shapes/shape_base.hpp"
#include "third_party/rive/include/rive/shapes/shape.hpp"

using namespace rive;

Core* ShapeBase::clone() const
{
  auto cloned = new Shape();
  cloned->copy(*this);
  return cloned;
}
