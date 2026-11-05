#include "third_party/rive/include/rive/generated/animation/transition_number_condition_base.hpp"
#include "third_party/rive/include/rive/animation/transition_number_condition.hpp"

using namespace rive;

Core* TransitionNumberConditionBase::clone() const
{
  auto cloned = new TransitionNumberCondition();
  cloned->copy(*this);
  return cloned;
}
