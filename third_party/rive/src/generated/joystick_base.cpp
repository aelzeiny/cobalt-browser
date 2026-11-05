#include "third_party/rive/include/rive/generated/joystick_base.hpp"
#include "third_party/rive/include/rive/joystick.hpp"

using namespace rive;

Core* JoystickBase::clone() const
{
  auto cloned = new Joystick();
  cloned->copy(*this);
  return cloned;
}
