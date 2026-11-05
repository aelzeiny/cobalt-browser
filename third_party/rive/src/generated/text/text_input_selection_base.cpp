#include "third_party/rive/include/rive/generated/text/text_input_selection_base.hpp"
#include "third_party/rive/include/rive/text/text_input_selection.hpp"

using namespace rive;

Core* TextInputSelectionBase::clone() const
{
  auto cloned = new TextInputSelection();
  cloned->copy(*this);
  return cloned;
}
