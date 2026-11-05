#include "third_party/rive/include/rive/generated/animation/focus_action_traversal_base.hpp"
#include "third_party/rive/include/rive/animation/focus_action_traversal.hpp"

using namespace rive;

Core* FocusActionTraversalBase::clone() const
{
  auto cloned = new FocusActionTraversal();
  cloned->copy(*this);
  return cloned;
}
