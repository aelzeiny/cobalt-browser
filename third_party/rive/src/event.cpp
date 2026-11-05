#include "third_party/rive/include/rive/event.hpp"
#include "third_party/rive/include/rive/animation/state_machine_instance.hpp"
#include "third_party/rive/include/rive/artboard.hpp"
#include "third_party/rive/include/rive/importers/artboard_importer.hpp"

using namespace rive;

void Event::trigger(const CallbackData& value)
{
  value.context()->reportEvent(this, value.delaySeconds());
}