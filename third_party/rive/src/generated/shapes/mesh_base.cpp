#include "third_party/rive/include/rive/generated/shapes/mesh_base.hpp"
#include "third_party/rive/include/rive/shapes/mesh.hpp"

using namespace rive;

Core* MeshBase::clone() const
{
  auto cloned = new Mesh();
  cloned->copy(*this);
  return cloned;
}
