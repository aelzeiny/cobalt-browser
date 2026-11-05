#include "third_party/rive/include/rive/generated/artboard_base.hpp"
#include "third_party/rive/include/rive/artboard.hpp"

using namespace rive;

Core* ArtboardBase::clone() const
{
  auto cloned = new Artboard();
  cloned->copy(*this);
  return cloned;
}
