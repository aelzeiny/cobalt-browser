#include "third_party/rive/include/rive/generated/bones/bone_base.hpp"
#include "third_party/rive/include/rive/bones/bone.hpp"

using namespace rive;

Core* BoneBase::clone() const
{
  auto cloned = new Bone();
  cloned->copy(*this);
  return cloned;
}
