#include "third_party/rive/include/rive/intrinsically_sizeable.hpp"
#include "third_party/rive/include/rive/joystick.hpp"
#include "third_party/rive/include/rive/transform_component.hpp"

using namespace rive;

IntrinsicallySizeable* IntrinsicallySizeable::from(Component* component)
{
  if (component->is<TransformComponent>())
  {
    return component->as<TransformComponent>();
  }
  else if (component->is<Joystick>())
  {
    return component->as<Joystick>();
  }
  return nullptr;
}