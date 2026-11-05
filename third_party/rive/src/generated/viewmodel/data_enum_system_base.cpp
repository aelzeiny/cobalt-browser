#include "third_party/rive/include/rive/generated/viewmodel/data_enum_system_base.hpp"
#include "third_party/rive/include/rive/viewmodel/data_enum_system.hpp"

using namespace rive;

Core* DataEnumSystemBase::clone() const
{
  auto cloned = new DataEnumSystem();
  cloned->copy(*this);
  return cloned;
}
