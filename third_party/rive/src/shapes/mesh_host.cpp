#include "third_party/rive/include/rive/shapes/mesh_host.hpp"
#include "third_party/rive/include/rive/nested_artboard.hpp"
#include "third_party/rive/include/rive/nested_artboard_mesh_host.hpp"
#include "third_party/rive/include/rive/shapes/image.hpp"

using namespace rive;

MeshHost* MeshHost::from(Component* component)
{
  if (component == nullptr)
  {
    return nullptr;
  }
  switch (component->coreType())
  {
    case Image::typeKey:
      return component->as<Image>();
    case NestedArtboard::typeKey:
      return component->as<NestedArtboard>()->meshHost();
  }
  return nullptr;
}
