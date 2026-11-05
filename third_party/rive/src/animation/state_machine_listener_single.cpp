#include "third_party/rive/include/rive/animation/state_machine_listener_single.hpp"
#include "third_party/rive/include/rive/importers/import_stack.hpp"
#include "third_party/rive/include/rive/importers/state_machine_importer.hpp"
#include "third_party/rive/include/rive/generated/animation/state_machine_base.hpp"
#include "third_party/rive/include/rive/artboard.hpp"
#include "third_party/rive/include/rive/shapes/shape.hpp"
#include "third_party/rive/include/rive/animation/state_machine_instance.hpp"
#include "third_party/rive/include/rive/animation/listener_input_change.hpp"
#include "third_party/rive/include/rive/animation/listener_types/listener_input_type.hpp"

using namespace rive;

StatusCode StateMachineListenerSingle::import(ImportStack& importStack)
{
  importDataBindPath(importStack);
  return Super::import(importStack);
}

void StateMachineListenerSingle::decodeViewModelPathIds(
    Span<const uint8_t> value)
{
  decodeDataBindPath(value);
}

void StateMachineListenerSingle::copyViewModelPathIds(
    const StateMachineListenerSingleBase& object)
{
  copyDataBindPath(object.as<StateMachineListenerSingle>()->dataBindPath());
}

std::vector<uint32_t> StateMachineListenerSingle::viewModelPathIdsBuffer() const
{
  return dataBindPath()->path();
}