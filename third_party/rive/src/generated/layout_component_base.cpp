#include "third_party/rive/include/rive/generated/layout_component_base.hpp"
#include "third_party/rive/include/rive/layout_component.hpp"

using namespace rive;

Core* LayoutComponentBase::clone() const
{
  auto cloned = new LayoutComponent();
  cloned->copy(*this);
  return cloned;
}
