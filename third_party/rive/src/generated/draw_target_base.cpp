#include "third_party/rive/include/rive/generated/draw_target_base.hpp"
#include "third_party/rive/include/rive/draw_target.hpp"

using namespace rive;

Core* DrawTargetBase::clone() const
{
  auto cloned = new DrawTarget();
  cloned->copy(*this);
  return cloned;
}
