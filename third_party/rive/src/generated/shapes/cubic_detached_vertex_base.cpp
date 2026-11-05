#include "third_party/rive/include/rive/generated/shapes/cubic_detached_vertex_base.hpp"
#include "third_party/rive/include/rive/shapes/cubic_detached_vertex.hpp"

using namespace rive;

Core* CubicDetachedVertexBase::clone() const
{
  auto cloned = new CubicDetachedVertex();
  cloned->copy(*this);
  return cloned;
}
