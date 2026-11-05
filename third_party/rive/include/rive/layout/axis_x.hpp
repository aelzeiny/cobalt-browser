#ifndef _RIVE_AXIS_X_HPP_
#define _RIVE_AXIS_X_HPP_
#include "third_party/rive/include/rive/generated/layout/axis_x_base.hpp"
#include <stdio.h>
namespace rive
{
class AxisX : public AxisXBase
{
public:
  StatusCode onAddedDirty(CoreContext* context) override;
};
} // namespace rive

#endif