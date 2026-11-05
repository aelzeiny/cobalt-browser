#include "third_party/rive/include/rive/generated/animation/listener_align_target_base.hpp"
#include "third_party/rive/include/rive/animation/listener_align_target.hpp"

using namespace rive;

Core* ListenerAlignTargetBase::clone() const
{
  auto cloned = new ListenerAlignTarget();
  cloned->copy(*this);
  return cloned;
}
