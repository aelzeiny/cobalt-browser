#include "third_party/rive/include/rive/generated/scripted/scripted_transition_base.hpp"
#include "third_party/rive/include/rive/scripted/scripted_transition.hpp"

using namespace rive;

Core* ScriptedTransitionBase::clone() const
{
  auto cloned = new ScriptedTransition();
  cloned->copy(*this);
  return cloned;
}
