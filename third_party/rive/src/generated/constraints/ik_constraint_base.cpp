#include "third_party/rive/include/rive/generated/constraints/ik_constraint_base.hpp"
#include "third_party/rive/include/rive/constraints/ik_constraint.hpp"

using namespace rive;

Core* IKConstraintBase::clone() const
{
  auto cloned = new IKConstraint();
  cloned->copy(*this);
  return cloned;
}
