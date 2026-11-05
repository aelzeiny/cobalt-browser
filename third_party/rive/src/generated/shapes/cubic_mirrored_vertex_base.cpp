#include "third_party/rive/include/rive/generated/shapes/cubic_mirrored_vertex_base.hpp"
#include "third_party/rive/include/rive/shapes/cubic_mirrored_vertex.hpp"

using namespace rive;

Core* CubicMirroredVertexBase::clone() const
{
  auto cloned = new CubicMirroredVertex();
  cloned->copy(*this);
  return cloned;
}
