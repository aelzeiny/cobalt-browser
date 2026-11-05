#include "third_party/rive/include/rive/artboard.hpp"
#include "third_party/rive/include/rive/shapes/points_common_path.hpp"
#include "third_party/rive/include/rive/shapes/vertex.hpp"
#include "third_party/rive/include/rive/shapes/path_vertex.hpp"
#include "third_party/rive/include/rive/shapes/shape.hpp"
#include "third_party/rive/include/rive/bones/skin.hpp"
#include "third_party/rive/include/rive/span.hpp"
#include "third_party/rive/include/rive/shapes/shape_path_flags.hpp"

using namespace rive;

bool PointsCommonPath::isClockwise() const
{
  return (pathFlags() & (int)ShapePathFlags::isCounterClockwise) == 0;
}
