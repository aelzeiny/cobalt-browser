/*
 * Copyright 2024 Rive
 */

#include "third_party/rive/include/rive/animation/focus_action_traversal.hpp"
#include "third_party/rive/include/rive/animation/listener_invocation.hpp"
#include "third_party/rive/include/rive/animation/state_machine_instance.hpp"

using namespace rive;

void FocusActionTraversal::perform(
    StateMachineInstance* stateMachineInstance,
    const ListenerInvocation& invocation) const
{
  (void)invocation;
  if (stateMachineInstance == nullptr)
  {
    return;
  }

  stateMachineInstance->queueFocusTraversal(traversalKind());
}
