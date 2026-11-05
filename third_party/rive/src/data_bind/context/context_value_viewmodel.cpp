#include "third_party/rive/include/rive/data_bind/context/context_value_viewmodel.hpp"
#include "third_party/rive/include/rive/data_bind/bindable_property_viewmodel.hpp"
#include "third_party/rive/include/rive/generated/data_bind/bindable_property_id_base.hpp"
#include "third_party/rive/include/rive/data_bind/data_bind_viewmodel_consumer.hpp"
#include "third_party/rive/include/rive/generated/core_registry.hpp"
#include "third_party/rive/include/rive/viewmodel/viewmodel_instance_list_item.hpp"
#include "third_party/rive/include/rive/viewmodel/viewmodel_instance.hpp"

using namespace rive;

DataBindContextValueViewModel::DataBindContextValueViewModel(DataBind* dataBind)
    : DataBindContextValue(dataBind)
{}

void DataBindContextValueViewModel::apply(
    Core* target,
    uint32_t propertyKey,
    bool isMainDirection,
    DataBind* dataBind)
{
  syncSourceValue(dataBind);
  auto value = calculateValue<DataValueViewModel, ViewModelInstance*>(
      m_dataValue,
      isMainDirection,
      dataBind);
  if (target != nullptr)
  {
    auto consumer = DataBindViewModelConsumer::from(target);
    if (consumer != nullptr)
    {
      consumer->updateViewModel(value);
    }
    else if (target->is<BindablePropertyViewModel>())
    {
      auto bindable = target->as<BindablePropertyViewModel>();
      bindable->viewModelInstanceValue(value);
      CoreRegistry::setUint(
          target,
          BindablePropertyIdBase::propertyValuePropertyKey,
          value != nullptr ? ViewModelInstance::pointerKey(value)
                           : BindablePropertyViewModel::defaultValue);
    }
  }
}