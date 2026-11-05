#ifndef _RIVE_TRANSFORM_CONSTRAINT_HPP_
#define _RIVE_TRANSFORM_CONSTRAINT_HPP_
#include "third_party/rive/include/rive/generated/constraints/transform_constraint_base.hpp"
#include "third_party/rive/include/rive/math/transform_components.hpp"

#include <stdio.h>
namespace rive
{
class TransformConstraint : public TransformConstraintBase
{
private:
  TransformComponents m_ComponentsA;
  TransformComponents m_ComponentsB;

public:
  virtual const Mat2D targetTransform() const;
  void constrain(TransformComponent* component) override;
  void originXChanged() override;
  void originYChanged() override;

  static void constrainWorld(
      TransformComponent* component,
      Mat2D from,
      TransformComponents componentsFrom,
      Mat2D to,
      TransformComponents componentsTo,
      float strength);
};
} // namespace rive

#endif