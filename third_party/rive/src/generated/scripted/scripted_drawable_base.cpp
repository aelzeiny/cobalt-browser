#include "third_party/rive/include/rive/generated/scripted/scripted_drawable_base.hpp"
#include "third_party/rive/include/rive/scripted/scripted_drawable.hpp"

using namespace rive;

Core* ScriptedDrawableBase::clone() const
{
  auto cloned = new ScriptedDrawable();
  cloned->copy(*this);
  return cloned;
}
