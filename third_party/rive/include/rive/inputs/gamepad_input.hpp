#ifndef _RIVE_GAMEPAD_INPUT_HPP_
#define _RIVE_GAMEPAD_INPUT_HPP_
#include "third_party/rive/include/rive/generated/inputs/gamepad_input_base.hpp"
#include "third_party/rive/include/rive/inputs/gamepad_button_phase.hpp"
#include "third_party/rive/include/rive/inputs/gamepad_input_kind.hpp"
namespace rive
{
class GamepadInput : public GamepadInputBase
{
public:
  StatusCode import(ImportStack& importStack) override;
};
} // namespace rive

#endif
