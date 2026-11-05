#include "third_party/rive/include/rive/generated/scripted/scripted_layout_base.hpp"
#include "third_party/rive/include/rive/scripted/scripted_layout.hpp"

using namespace rive;

Core* ScriptedLayoutBase::clone() const
{
  auto cloned = new ScriptedLayout();
  cloned->copy(*this);
  return cloned;
}
