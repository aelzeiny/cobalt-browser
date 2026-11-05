#include "third_party/rive/include/rive/generated/shapes/points_path_base.hpp"
#include "third_party/rive/include/rive/shapes/points_path.hpp"

using namespace rive;

Core* PointsPathBase::clone() const
{
  auto cloned = new PointsPath();
  cloned->copy(*this);
  return cloned;
}
