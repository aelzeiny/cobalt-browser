#include "third_party/rive/include/rive/generated/text/text_style_global_base.hpp"
#include "third_party/rive/include/rive/text/text_style_global.hpp"

using namespace rive;

Core* TextStyleGlobalBase::clone() const
{
  auto cloned = new TextStyleGlobal();
  cloned->copy(*this);
  return cloned;
}
