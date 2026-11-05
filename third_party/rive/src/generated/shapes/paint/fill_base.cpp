#include "third_party/rive/include/rive/generated/shapes/paint/fill_base.hpp"
#include "third_party/rive/include/rive/shapes/paint/fill.hpp"

using namespace rive;

Core* FillBase::clone() const
{
  auto cloned = new Fill();
  cloned->copy(*this);
  return cloned;
}
