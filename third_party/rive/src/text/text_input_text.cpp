#include "third_party/rive/include/rive/text/text_input_text.hpp"
#include "third_party/rive/include/rive/math/mat2d.hpp"
#include "third_party/rive/include/rive/text/text_input.hpp"

using namespace rive;

Core* TextInputText::hitTest(HitInfo*, const Mat2D&) { return nullptr; }

ShapePaintPath* TextInputText::localClockwisePath()
{
#ifdef WITH_RIVE_TEXT
  return textInput()->rawTextInput()->textPath();
#else
  return nullptr;
#endif
}