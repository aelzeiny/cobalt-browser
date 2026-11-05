#include "third_party/rive/include/rive/text/text_interface.hpp"
#include "third_party/rive/include/rive/text/text_input.hpp"
#include "third_party/rive/include/rive/text/text.hpp"

using namespace rive;

TextInterface* TextInterface::from(Core* component)
{
  if (component == nullptr)
  {
    return nullptr;
  }
  switch (component->coreType())
  {
    case Text::typeKey:
      return component->as<Text>();
      break;
    case TextInput::typeKey:
      return component->as<TextInput>();
      break;
  }
  return nullptr;
}