#include "third_party/rive/include/rive/artboard_component_list.hpp"
#include "third_party/rive/include/rive/shapes/list_path.hpp"
#include "third_party/rive/include/rive/component.hpp"
#include "third_party/rive/include/rive/data_bind/data_bind_list_item_consumer.hpp"
#include "third_party/rive/include/rive/generated/core_registry.hpp"
#include "third_party/rive/include/rive/scripted/scripted_transition.hpp"
#include "third_party/rive/include/rive/viewmodel/viewmodel_instance_list.hpp"

using namespace rive;

DataBindListItemConsumer* DataBindListItemConsumer::from(Core* component)
{
  switch (component->coreType())
  {
    case ArtboardComponentList::typeKey:
      return component->as<ArtboardComponentList>();
    case ListPath::typeKey:
      return component->as<ListPath>();
    case Text::typeKey:
      return component->as<Text>();
    case ViewModelInstanceList::typeKey:
      return component->as<ViewModelInstanceList>();
    case ScriptedTransition::typeKey:
      return component->as<ScriptedTransition>();
  }
  return nullptr;
}