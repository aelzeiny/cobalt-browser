#ifndef _RIVE_SCALE_CONSTRAINT_HPP_
#define _RIVE_SCALE_CONSTRAINT_HPP_
#include "third_party/rive/include/rive/generated/constraints/scale_constraint_base.hpp"
#include "third_party/rive/include/rive/math/transform_components.hpp"
#include <stdio.h>
namespace rive
{
class ScaleConstraint : public ScaleConstraintBase
{
private:
  TransformComponents m_ComponentsA;
  TransformComponents m_ComponentsB;

protected:
  bool requiresTarget() override { return false; }

public:
  void constrain(TransformComponent* component) override;
};
} // namespace rive

#endif