#include "third_party/rive/include/rive/generated/viewmodel/data_enum_base.hpp"
#include "third_party/rive/include/rive/viewmodel/data_enum.hpp"

using namespace rive;

Core* DataEnumBase::clone() const
{
  auto cloned = new DataEnum();
  cloned->copy(*this);
  return cloned;
}
