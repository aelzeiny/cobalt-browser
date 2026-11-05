#include "third_party/rive/include/rive/generated/backboard_base.hpp"
#include "third_party/rive/include/rive/backboard.hpp"

using namespace rive;

Core* BackboardBase::clone() const
{
  auto cloned = new Backboard();
  cloned->copy(*this);
  return cloned;
}
