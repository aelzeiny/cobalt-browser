#include "third_party/rive/include/rive/animation/listener_input_change.hpp"
#include "third_party/rive/include/rive/animation/nested_input.hpp"
#include "third_party/rive/include/rive/animation/state_machine.hpp"
#include "third_party/rive/include/rive/animation/state_machine_listener.hpp"
#include "third_party/rive/include/rive/artboard.hpp"
#include "third_party/rive/include/rive/importers/artboard_importer.hpp"
#include "third_party/rive/include/rive/importers/import_stack.hpp"
#include "third_party/rive/include/rive/importers/state_machine_listener_importer.hpp"
#include "third_party/rive/include/rive/importers/state_machine_importer.hpp"

using namespace rive;

StatusCode ListenerInputChange::import(ImportStack& importStack)
{
  auto stateMachineImporter = importStack.latest<StateMachineImporter>(
      StateMachine::typeKey);
  if (stateMachineImporter == nullptr)
  {
    return StatusCode::MissingObject;
  }

  auto artboardImporter = importStack.latest<ArtboardImporter>(
      ArtboardBase::typeKey);
  if (artboardImporter == nullptr)
  {
    return StatusCode::MissingObject;
  }
  // First validate if this input change applies to a nested input
  auto nested = artboardImporter->artboard()->resolve(nestedInputId());
  auto nestedInput = nested != nullptr ? nested->as<NestedInput>() : nullptr;
  if (nestedInput != nullptr)
  {
    if (!validateNestedInputType(nestedInput))
    {
      return StatusCode::InvalidObject;
    }
  }
  // Only if it doesn't apply to a nested input, check against an input
  else
  {
    auto input = stateMachineImporter->stateMachine()->input((size_t)inputId());
    if (!validateInputType(input))
    {
      return StatusCode::InvalidObject;
    }
  }
  return Super::import(importStack);
}
