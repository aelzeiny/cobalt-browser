#include "third_party/rive/include/rive/generated/scripted/scripted_interpolator_base.hpp"
#include "third_party/rive/include/rive/scripted/scripted_interpolator.hpp"

using namespace rive;

Core* ScriptedInterpolatorBase::clone() const
{
  auto cloned = new ScriptedInterpolator();
  cloned->copy(*this);
  return cloned;
}
