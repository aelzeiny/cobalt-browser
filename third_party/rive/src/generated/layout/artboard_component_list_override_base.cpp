#include "third_party/rive/include/rive/generated/layout/artboard_component_list_override_base.hpp"
#include "third_party/rive/include/rive/layout/artboard_component_list_override.hpp"

using namespace rive;

Core* ArtboardComponentListOverrideBase::clone() const
{
  auto cloned = new ArtboardComponentListOverride();
  cloned->copy(*this);
  return cloned;
}
