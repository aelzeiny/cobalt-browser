#include "third_party/rive/include/rive/generated/animation/nested_bool_base.hpp"
#include "third_party/rive/include/rive/animation/nested_bool.hpp"

using namespace rive;

Core* NestedBoolBase::clone() const
{
  auto cloned = new NestedBool();
  cloned->copy(*this);
  return cloned;
}
