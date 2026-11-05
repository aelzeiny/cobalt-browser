#include "third_party/rive/include/rive/generated/animation/blend_animation_1d_base.hpp"
#include "third_party/rive/include/rive/animation/blend_animation_1d.hpp"

using namespace rive;

Core* BlendAnimation1DBase::clone() const
{
  auto cloned = new BlendAnimation1D();
  cloned->copy(*this);
  return cloned;
}
