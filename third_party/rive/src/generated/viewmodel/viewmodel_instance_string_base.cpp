#include "third_party/rive/include/rive/generated/viewmodel/viewmodel_instance_string_base.hpp"
#include "third_party/rive/include/rive/viewmodel/viewmodel_instance_string.hpp"

using namespace rive;

Core* ViewModelInstanceStringBase::clone() const
{
  auto cloned = new ViewModelInstanceString();
  cloned->copy(*this);
  return cloned;
}
