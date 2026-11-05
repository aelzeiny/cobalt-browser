#include "third_party/rive/include/rive/artboard.hpp"
#include "third_party/rive/include/rive/artboard_component_list.hpp"
#include "third_party/rive/include/rive/component.hpp"
#include "third_party/rive/include/rive/layout/layout_node_provider.hpp"
#include "third_party/rive/include/rive/virtualizing_component.hpp"

using namespace rive;

VirtualizingComponent* VirtualizingComponent::from(Component* component)
{
  switch (component->coreType())
  {
    case ArtboardComponentList::typeKey:
      return component->as<ArtboardComponentList>();
  }
  return nullptr;
}

VirtualizingComponent* VirtualizingComponent::fromScrollChild(
    LayoutNodeProvider* child)
{
  auto component = child != nullptr ? child->transformComponent() : nullptr;
  return component != nullptr ? from(component) : nullptr;
}