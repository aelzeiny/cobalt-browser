#include <sstream>
#include <iomanip>
#include <array>

#include "third_party/rive/include/rive/viewmodel/viewmodel_instance_boolean.hpp"
#include "third_party/rive/include/rive/component_dirt.hpp"

using namespace rive;

void ViewModelInstanceBoolean::propertyValueChanged()
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

void ViewModelInstanceBoolean::applyValue(DataValueBoolean* dataValue)
{
  propertyValue(dataValue->value());
}