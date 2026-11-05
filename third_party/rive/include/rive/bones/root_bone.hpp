#ifndef _RIVE_ROOT_BONE_HPP_
#define _RIVE_ROOT_BONE_HPP_
#include "third_party/rive/include/rive/generated/bones/root_bone_base.hpp"
#include <stdio.h>
namespace rive
{
class RootBone : public RootBoneBase
{
public:
  StatusCode onAddedClean(CoreContext* context) override;

protected:
  void xChanged() override;
  void yChanged() override;
};
} // namespace rive

#endif