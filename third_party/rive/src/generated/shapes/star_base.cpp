#include "third_party/rive/include/rive/generated/shapes/star_base.hpp"
#include "third_party/rive/include/rive/shapes/star.hpp"

using namespace rive;

Core* StarBase::clone() const
{
  auto cloned = new Star();
  cloned->copy(*this);
  return cloned;
}
