#include "third_party/rive/include/rive/generated/animation/listener_viewmodel_change_base.hpp"
#include "third_party/rive/include/rive/animation/listener_viewmodel_change.hpp"

using namespace rive;

Core* ListenerViewModelChangeBase::clone() const
{
  auto cloned = new ListenerViewModelChange();
  cloned->copy(*this);
  return cloned;
}
