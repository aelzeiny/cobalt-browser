#include "third_party/rive/include/rive/generated/animation/keyframe_bool_base.hpp"
#include "third_party/rive/include/rive/animation/keyframe_bool.hpp"

using namespace rive;

Core* KeyFrameBoolBase::clone() const
{
  auto cloned = new KeyFrameBool();
  cloned->copy(*this);
  return cloned;
}
