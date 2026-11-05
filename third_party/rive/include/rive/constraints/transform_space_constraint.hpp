#ifndef _RIVE_TRANSFORM_SPACE_CONSTRAINT_HPP_
#define _RIVE_TRANSFORM_SPACE_CONSTRAINT_HPP_
#include "third_party/rive/include/rive/generated/constraints/transform_space_constraint_base.hpp"
#include "third_party/rive/include/rive/transform_space.hpp"
#include <stdio.h>
namespace rive
{
class TransformSpaceConstraint : public TransformSpaceConstraintBase
{
public:
  TransformSpace sourceSpace() const
  {
    return (TransformSpace)sourceSpaceValue();
  }
  TransformSpace destSpace() const { return (TransformSpace)destSpaceValue(); }
};
} // namespace rive

#endif