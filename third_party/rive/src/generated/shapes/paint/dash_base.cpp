#include "third_party/rive/include/rive/generated/shapes/paint/dash_base.hpp"
#include "third_party/rive/include/rive/shapes/paint/dash.hpp"

using namespace rive;

Core* DashBase::clone() const
{
  auto cloned = new Dash();
  cloned->copy(*this);
  return cloned;
}
