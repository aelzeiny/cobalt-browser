#include "third_party/rive/include/rive/generated/shapes/paint/feather_base.hpp"
#include "third_party/rive/include/rive/shapes/paint/feather.hpp"

using namespace rive;

Core* FeatherBase::clone() const
{
  auto cloned = new Feather();
  cloned->copy(*this);
  return cloned;
}
