#include "third_party/rive/include/rive/generated/animation/state_machine_number_base.hpp"
#include "third_party/rive/include/rive/animation/state_machine_number.hpp"

using namespace rive;

Core* StateMachineNumberBase::clone() const
{
  auto cloned = new StateMachineNumber();
  cloned->copy(*this);
  return cloned;
}
