#include "third_party/rive/include/rive/generated/constraints/distance_constraint_base.hpp"
#include "third_party/rive/include/rive/constraints/distance_constraint.hpp"

using namespace rive;

Core* DistanceConstraintBase::clone() const
{
  auto cloned = new DistanceConstraint();
  cloned->copy(*this);
  return cloned;
}
