#include "third_party/rive/include/rive/generated/animation/transition_value_id_comparator_base.hpp"
#include "third_party/rive/include/rive/animation/transition_value_id_comparator.hpp"

using namespace rive;

Core* TransitionValueIdComparatorBase::clone() const
{
  auto cloned = new TransitionValueIdComparator();
  cloned->copy(*this);
  return cloned;
}
