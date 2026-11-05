#include "third_party/rive/include/rive/animation/nested_trigger.hpp"
#include "third_party/rive/include/rive/animation/nested_state_machine.hpp"
#include "third_party/rive/include/rive/animation/state_machine_input_instance.hpp"
#include "third_party/rive/include/rive/container_component.hpp"

using namespace rive;
class StateMachineInstance;

void NestedTrigger::fire(const CallbackData& value) { this->applyValue(); }

void NestedTrigger::applyValue()
{
  auto inputInstance = input();
  if (inputInstance != nullptr)
  {
    auto numInput = static_cast<SMITrigger*>(inputInstance);
    if (numInput != nullptr)
    {
      numInput->fire();
    }
  }
}