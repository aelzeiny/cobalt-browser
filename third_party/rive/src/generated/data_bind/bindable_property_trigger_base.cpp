#include "third_party/rive/include/rive/generated/data_bind/bindable_property_trigger_base.hpp"
#include "third_party/rive/include/rive/data_bind/bindable_property_trigger.hpp"

using namespace rive;

Core* BindablePropertyTriggerBase::clone() const
{
  auto cloned = new BindablePropertyTrigger();
  cloned->copy(*this);
  return cloned;
}
