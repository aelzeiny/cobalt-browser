#include "third_party/rive/include/rive/custom_property_trigger.hpp"
#include "third_party/rive/include/rive/viewmodel/viewmodel_instance_value.hpp"

using namespace rive;

void CustomPropertyTrigger::propertyValueChanged()
{
  m_changeSequence = ViewModelInstanceValue::nextChangeSequence();
}
