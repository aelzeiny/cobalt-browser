#include "third_party/rive/include/rive/generated/bones/cubic_weight_base.hpp"
#include "third_party/rive/include/rive/bones/cubic_weight.hpp"

using namespace rive;

Core* CubicWeightBase::clone() const
{
  auto cloned = new CubicWeight();
  cloned->copy(*this);
  return cloned;
}
