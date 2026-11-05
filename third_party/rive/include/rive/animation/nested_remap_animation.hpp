#ifndef _RIVE_NESTED_REMAP_ANIMATION_HPP_
#define _RIVE_NESTED_REMAP_ANIMATION_HPP_
#include "third_party/rive/include/rive/generated/animation/nested_remap_animation_base.hpp"
#include <stdio.h>
namespace rive
{
class NestedRemapAnimation : public NestedRemapAnimationBase
{
public:
  void timeChanged() override;
  bool advance(float elapsedSeconds, bool newFrame) override;
  void initializeAnimation(ArtboardInstance*) override;
};
} // namespace rive

#endif