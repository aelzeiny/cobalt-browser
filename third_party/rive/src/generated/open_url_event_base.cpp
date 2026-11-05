#include "third_party/rive/include/rive/generated/open_url_event_base.hpp"
#include "third_party/rive/include/rive/open_url_event.hpp"

using namespace rive;

Core* OpenUrlEventBase::clone() const
{
  auto cloned = new OpenUrlEvent();
  cloned->copy(*this);
  return cloned;
}
