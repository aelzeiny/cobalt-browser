#include "third_party/rive/include/rive/generated/inputs/gamepad_input_base.hpp"
#include "third_party/rive/include/rive/inputs/gamepad_input.hpp"

using namespace rive;

Core* GamepadInputBase::clone() const
{
  auto cloned = new GamepadInput();
  cloned->copy(*this);
  return cloned;
}
