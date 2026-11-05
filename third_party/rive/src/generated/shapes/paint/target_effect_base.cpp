#include "third_party/rive/include/rive/generated/shapes/paint/target_effect_base.hpp"
#include "third_party/rive/include/rive/shapes/paint/target_effect.hpp"

using namespace rive;

Core* TargetEffectBase::clone() const
{
  auto cloned = new TargetEffect();
  cloned->copy(*this);
  return cloned;
}
