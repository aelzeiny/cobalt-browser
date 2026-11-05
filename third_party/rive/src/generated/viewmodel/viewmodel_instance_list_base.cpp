#include "third_party/rive/include/rive/generated/viewmodel/viewmodel_instance_list_base.hpp"
#include "third_party/rive/include/rive/viewmodel/viewmodel_instance_list.hpp"

using namespace rive;

Core* ViewModelInstanceListBase::clone() const
{
  auto cloned = new ViewModelInstanceList();
  cloned->copy(*this);
  return cloned;
}
