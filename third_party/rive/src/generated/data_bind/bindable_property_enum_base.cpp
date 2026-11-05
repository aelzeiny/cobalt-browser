#include "third_party/rive/include/rive/generated/data_bind/bindable_property_enum_base.hpp"
#include "third_party/rive/include/rive/data_bind/bindable_property_enum.hpp"

using namespace rive;

Core* BindablePropertyEnumBase::clone() const
{
  auto cloned = new BindablePropertyEnum();
  cloned->copy(*this);
  return cloned;
}
