#ifndef _RIVE_SCRIPT_INPUT_TRIGGER_HPP_
#define _RIVE_SCRIPT_INPUT_TRIGGER_HPP_
#include "third_party/rive/include/rive/generated/script_input_trigger_base.hpp"
#include "third_party/rive/include/rive/assets/script_asset.hpp"
#include "third_party/rive/include/rive/scripted/scripted_object.hpp"
#include <stdio.h>
namespace rive
{
class ScriptInputTrigger : public ScriptInputTriggerBase, public ScriptInput
{
protected:
  void propertyValueChanged() override;

public:
  ~ScriptInputTrigger();
  bool validateForScriptInit() override { return true; }
  StatusCode import(ImportStack& importStack) override;
  bool claimsArtboardSlot(ImportStack& importStack) const override
  {
    return ScriptInput::takesArtboardSlot(this);
  }
  StatusCode onAddedClean(CoreContext* context) override;
};
} // namespace rive

#endif