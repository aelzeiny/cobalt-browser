#include "third_party/rive/include/rive/generated/shapes/image_base.hpp"
#include "third_party/rive/include/rive/shapes/image.hpp"

using namespace rive;

Core* ImageBase::clone() const
{
  auto cloned = new Image();
  cloned->copy(*this);
  return cloned;
}
