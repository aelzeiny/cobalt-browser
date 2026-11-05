#include "third_party/rive/include/rive/generated/nested_artboard_layout_base.hpp"
#include "third_party/rive/include/rive/nested_artboard_layout.hpp"

using namespace rive;

Core* NestedArtboardLayoutBase::clone() const
{
  auto cloned = new NestedArtboardLayout();
  cloned->copy(*this);
  return cloned;
}
