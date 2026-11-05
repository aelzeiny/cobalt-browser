#include "third_party/rive/include/rive/animation/blend_state_direct.hpp"
#include "third_party/rive/include/rive/animation/state_machine.hpp"
#include "third_party/rive/include/rive/animation/state_machine_number.hpp"
#include "third_party/rive/include/rive/animation/blend_state_direct_instance.hpp"

using namespace rive;

std::unique_ptr<StateInstance> BlendStateDirect::makeInstance(
    ArtboardInstance* instance) const
{
  return std::make_unique<BlendStateDirectInstance>(this, instance);
}