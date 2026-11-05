#include "third_party/rive/include/rive/generated/animation/listener_bool_change_base.hpp"
#include "third_party/rive/include/rive/animation/listener_bool_change.hpp"

using namespace rive;

Core* ListenerBoolChangeBase::clone() const
{
  auto cloned = new ListenerBoolChange();
  cloned->copy(*this);
  return cloned;
}
