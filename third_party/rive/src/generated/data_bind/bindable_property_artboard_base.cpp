#include "third_party/rive/include/rive/generated/data_bind/bindable_property_artboard_base.hpp"
#include "third_party/rive/include/rive/data_bind/bindable_property_artboard.hpp"

using namespace rive;

Core* BindablePropertyArtboardBase::clone() const
{
  auto cloned = new BindablePropertyArtboard();
  cloned->copy(*this);
  return cloned;
}
