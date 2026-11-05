#include "third_party/rive/include/rive/generated/shapes/polygon_base.hpp"
#include "third_party/rive/include/rive/shapes/polygon.hpp"

using namespace rive;

Core* PolygonBase::clone() const
{
  auto cloned = new Polygon();
  cloned->copy(*this);
  return cloned;
}
