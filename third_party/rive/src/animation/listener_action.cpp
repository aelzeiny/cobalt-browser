#include "third_party/rive/include/rive/animation/listener_action.hpp"
#include "third_party/rive/include/rive/animation/state_machine_listener.hpp"
#include "third_party/rive/include/rive/importers/import_stack.hpp"
#include "third_party/rive/include/rive/importers/state_machine_layer_component_importer.hpp"
#include "third_party/rive/include/rive/importers/state_machine_listener_importer.hpp"
#include "third_party/rive/include/rive/importers/state_machine_importer.hpp"
#include "third_party/rive/include/rive/animation/listener_input_change.hpp"
#include "third_party/rive/include/rive/animation/state_machine.hpp"
#include "third_party/rive/include/rive/generated/animation/state_machine_layer_component_base.hpp"
#include "third_party/rive/include/rive/generated/animation/state_machine_listener_base.hpp"

using namespace rive;

StatusCode ListenerAction::import(ImportStack& importStack)
{
  switch (parentKind())
  {
    case ParentKind::Listener:
    {
      auto listenerImporter = importStack.latest<StateMachineListenerImporter>(
          StateMachineListenerBase::typeKey);
      if (listenerImporter == nullptr)
      {
        return StatusCode::MissingObject;
      }
      listenerImporter->addAction(std::unique_ptr<ListenerAction>(this));
      return Super::import(importStack);
    }
    case ParentKind::Transition:
    case ParentKind::State:
    {
      auto layerImporter = importStack
                               .latest<StateMachineLayerComponentImporter>(
                                   StateMachineLayerComponentBase::typeKey);
      if (layerImporter == nullptr)
      {
        return StatusCode::MissingObject;
      }
      layerImporter->addListenerAction(std::unique_ptr<ListenerAction>(this));
      return Super::import(importStack);
    }
  }
  return StatusCode::MissingObject;
}
