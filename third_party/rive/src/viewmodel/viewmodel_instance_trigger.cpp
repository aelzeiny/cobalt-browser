#include <sstream>
#include <iomanip>
#include <array>

#include "third_party/rive/include/rive/viewmodel/viewmodel_instance_trigger.hpp"
#include "third_party/rive/include/rive/component_dirt.hpp"

using namespace rive;

void ViewModelInstanceTrigger::propertyValueChanged()
{
  addDirt(ComponentDirt::Bindings);
#ifdef WITH_RIVE_TOOLS
  if (m_changedCallback != nullptr)
  {
    m_changedCallback(this, propertyValue());
  }
#endif
  onValueChanged();
}

void ViewModelInstanceTrigger::applyValue(DataValueInteger* dataValue)
{
  propertyValue(dataValue->value());
}