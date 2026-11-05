#ifndef _RIVE_LISTENER_INPUT_TYPE_HPP_
#define _RIVE_LISTENER_INPUT_TYPE_HPP_
#include "third_party/rive/include/rive/generated/animation/listener_types/listener_input_type_base.hpp"
#include "third_party/rive/include/rive/pointer_button.hpp"
#include <stdio.h>
namespace rive
{
class ListenerInputType : public ListenerInputTypeBase
{
public:
  StatusCode import(ImportStack& importStack) override;
  virtual PointerButton pointerButton() const { return PointerButton::primary; }
};
} // namespace rive

#endif