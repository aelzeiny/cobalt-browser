#include "third_party/rive/include/rive/generated/bones/skin_base.hpp"
#include "third_party/rive/include/rive/bones/skin.hpp"

using namespace rive;

Core* SkinBase::clone() const
{
  auto cloned = new Skin();
  cloned->copy(*this);
  return cloned;
}
