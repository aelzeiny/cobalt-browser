#ifndef _RIVE_CUBIC_INTERPOLATOR_COMPONENT_HPP_
#define _RIVE_CUBIC_INTERPOLATOR_COMPONENT_HPP_
#include "third_party/rive/include/rive/generated/animation/cubic_interpolator_component_base.hpp"
#include "third_party/rive/include/rive/animation/cubic_interpolator_solver.hpp"

namespace rive
{
class CubicInterpolatorComponent : public CubicInterpolatorComponentBase
{
public:
  StatusCode onAddedDirty(CoreContext* context) override;
  float transform(float factor) const;

private:
  CubicInterpolatorSolver m_solver;
};
} // namespace rive

#endif