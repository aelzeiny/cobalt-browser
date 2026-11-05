#include "third_party/rive/include/rive/generated/custom_property_enum_base.hpp"
#include "third_party/rive/include/rive/custom_property_enum.hpp"

using namespace rive;

Core* CustomPropertyEnumBase::clone() const
{
  auto cloned = new CustomPropertyEnum();
  cloned->copy(*this);
  return cloned;
}
