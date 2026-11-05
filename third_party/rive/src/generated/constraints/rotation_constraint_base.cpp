#include "third_party/rive/include/rive/generated/constraints/rotation_constraint_base.hpp"
#include "third_party/rive/include/rive/constraints/rotation_constraint.hpp"

using namespace rive;

Core* RotationConstraintBase::clone() const
{
  auto cloned = new RotationConstraint();
  cloned->copy(*this);
  return cloned;
}
