#include "third_party/rive/include/rive/generated/constraints/scrolling/scroll_constraint_base.hpp"
#include "third_party/rive/include/rive/constraints/scrolling/scroll_constraint.hpp"

using namespace rive;

Core* ScrollConstraintBase::clone() const
{
  auto cloned = new ScrollConstraint();
  cloned->copy(*this);
  return cloned;
}
