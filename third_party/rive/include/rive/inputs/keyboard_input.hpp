#ifndef _RIVE_KEYBOARD_INPUT_HPP_
#define _RIVE_KEYBOARD_INPUT_HPP_
#include "third_party/rive/include/rive/generated/inputs/keyboard_input_base.hpp"
#include "third_party/rive/include/rive/inputs/keyboard_key_phase.hpp"
#include <stdio.h>
namespace rive
{
class KeyboardInput : public KeyboardInputBase
{
public:
  StatusCode import(ImportStack& importStack) override;
};
} // namespace rive

#endif