#include "third_party/rive/include/rive/generated/inputs/user_input_base.hpp"
#include "third_party/rive/include/rive/inputs/user_input.hpp"

using namespace rive;

Core* UserInputBase::clone() const
{
  auto cloned = new UserInput();
  cloned->copy(*this);
  return cloned;
}
