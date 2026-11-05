#ifndef _RIVE_CUBIC_VERTEX_BASE_HPP_
#define _RIVE_CUBIC_VERTEX_BASE_HPP_
#include "third_party/rive/include/rive/shapes/path_vertex.hpp"
namespace rive
{
class CubicVertexBase : public PathVertex
{
protected:
  typedef PathVertex Super;

public:
  static const uint16_t typeKey = 36;

  uint16_t coreType() const override { return typeKey; }
};
} // namespace rive

#endif