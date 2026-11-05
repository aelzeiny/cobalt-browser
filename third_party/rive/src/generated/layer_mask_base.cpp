#include "third_party/rive/include/rive/generated/layer_mask_base.hpp"
#include "third_party/rive/include/rive/layer_mask.hpp"

using namespace rive;

Core* LayerMaskBase::clone() const
{
  auto cloned = new LayerMask();
  cloned->copy(*this);
  return cloned;
}
