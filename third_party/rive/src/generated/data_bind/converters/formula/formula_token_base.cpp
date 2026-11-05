#include "third_party/rive/include/rive/generated/data_bind/converters/formula/formula_token_base.hpp"
#include "third_party/rive/include/rive/data_bind/converters/formula/formula_token.hpp"

using namespace rive;

Core* FormulaTokenBase::clone() const
{
  auto cloned = new FormulaToken();
  cloned->copy(*this);
  return cloned;
}
