#include "third_party/rive/include/rive/artboard_component_list.hpp"
#include "third_party/rive/include/rive/shapes/list_path.hpp"
#include "third_party/rive/include/rive/viewmodel/viewmodel_instance_viewmodel.hpp"
#include "third_party/rive/include/rive/data_bind/data_bind_viewmodel_consumer.hpp"
#include "third_party/rive/include/rive/generated/core_registry.hpp"

using namespace rive;

DataBindViewModelConsumer* DataBindViewModelConsumer::from(Core* component)
{
  switch (component->coreType())
  {
    case ViewModelInstanceViewModel::typeKey:
      return component->as<ViewModelInstanceViewModel>();
  }
  return nullptr;
}