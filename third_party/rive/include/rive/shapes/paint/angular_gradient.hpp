#ifndef _RIVE_ANGULAR_GRADIENT_HPP_
#define _RIVE_ANGULAR_GRADIENT_HPP_
#include "third_party/rive/include/rive/generated/shapes/paint/angular_gradient_base.hpp"
namespace rive
{
class AngularGradient : public AngularGradientBase
{
public:
  void makeGradient(
      RenderPaint* renderPaint,
      Vec2D start,
      Vec2D end,
      const ColorInt[],
      const float[],
      size_t count) const override;
};
} // namespace rive

#endif
