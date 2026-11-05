#ifndef _RIVE_BLEND_STATE_DIRECT_INSTANCE_HPP_
#define _RIVE_BLEND_STATE_DIRECT_INSTANCE_HPP_

#include "third_party/rive/include/rive/animation/blend_state_instance.hpp"
#include "third_party/rive/include/rive/animation/blend_state_direct.hpp"
#include "third_party/rive/include/rive/animation/blend_animation_direct.hpp"

namespace rive
{
class BlendStateDirectInstance
    : public BlendStateInstance<BlendStateDirect, BlendAnimationDirect>
{
public:
  BlendStateDirectInstance(
      const BlendStateDirect* blendState,
      ArtboardInstance* instance);
  void advance(float seconds, StateMachineInstance* stateMachineInstance)
      override;
};
} // namespace rive
#endif