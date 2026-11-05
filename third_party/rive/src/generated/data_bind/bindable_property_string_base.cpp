#include "third_party/rive/include/rive/generated/data_bind/bindable_property_string_base.hpp"
#include "third_party/rive/include/rive/data_bind/bindable_property_string.hpp"

using namespace rive;

Core* BindablePropertyStringBase::clone() const
{
  auto cloned = new BindablePropertyString();
  cloned->copy(*this);
  return cloned;
}
