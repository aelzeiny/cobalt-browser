#ifndef _RIVE_NESTED_BOOL_HPP_
#define _RIVE_NESTED_BOOL_HPP_
#include "third_party/rive/include/rive/generated/animation/nested_bool_base.hpp"
#include <stdio.h>
namespace rive
{
class NestedBool : public NestedBoolBase
{
public:
  void nestedValue(bool value) override;
  bool nestedValue() const override;

  void applyValue() override;

protected:
};
} // namespace rive

#endif