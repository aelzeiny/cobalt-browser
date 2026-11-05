#include "third_party/rive/include/rive/generated/bones/root_bone_base.hpp"
#include "third_party/rive/include/rive/bones/root_bone.hpp"

using namespace rive;

Core* RootBoneBase::clone() const
{
  auto cloned = new RootBone();
  cloned->copy(*this);
  return cloned;
}
