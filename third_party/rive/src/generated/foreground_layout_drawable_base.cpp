#include "third_party/rive/include/rive/generated/foreground_layout_drawable_base.hpp"
#include "third_party/rive/include/rive/foreground_layout_drawable.hpp"

using namespace rive;

Core* ForegroundLayoutDrawableBase::clone() const
{
  auto cloned = new ForegroundLayoutDrawable();
  cloned->copy(*this);
  return cloned;
}
