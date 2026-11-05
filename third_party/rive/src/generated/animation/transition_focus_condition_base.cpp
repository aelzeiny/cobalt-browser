#include "third_party/rive/include/rive/generated/animation/transition_focus_condition_base.hpp"
#include "third_party/rive/include/rive/animation/transition_focus_condition.hpp"

using namespace rive;

Core* TransitionFocusConditionBase::clone() const
{
  auto cloned = new TransitionFocusCondition();
  cloned->copy(*this);
  return cloned;
}
