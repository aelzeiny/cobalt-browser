#include "third_party/rive/include/rive/generated/bones/weight_base.hpp"
#include "third_party/rive/include/rive/bones/weight.hpp"

using namespace rive;

Core* WeightBase::clone() const
{
  auto cloned = new Weight();
  cloned->copy(*this);
  return cloned;
}
