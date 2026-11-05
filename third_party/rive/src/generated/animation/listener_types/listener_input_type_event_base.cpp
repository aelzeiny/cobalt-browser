#include "third_party/rive/include/rive/generated/animation/listener_types/listener_input_type_event_base.hpp"
#include "third_party/rive/include/rive/animation/listener_types/listener_input_type_event.hpp"

using namespace rive;

Core* ListenerInputTypeEventBase::clone() const
{
  auto cloned = new ListenerInputTypeEvent();
  cloned->copy(*this);
  return cloned;
}
