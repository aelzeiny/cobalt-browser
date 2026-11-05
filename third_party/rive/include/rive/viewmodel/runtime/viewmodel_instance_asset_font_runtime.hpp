#ifndef _RIVE_VIEW_MODEL_INSTANCE_ASSET_FONT_RUNTIME_HPP_
#define _RIVE_VIEW_MODEL_INSTANCE_ASSET_FONT_RUNTIME_HPP_

#include <string>
#include <stdint.h>
#include "third_party/rive/include/rive/viewmodel/runtime/viewmodel_instance_value_runtime.hpp"
#include "third_party/rive/include/rive/viewmodel/viewmodel_instance_asset_font.hpp"

namespace rive
{

class ViewModelInstanceAssetFontRuntime : public ViewModelInstanceValueRuntime
{

public:
  ViewModelInstanceAssetFontRuntime(
      ViewModelInstanceAssetFont* viewModelInstance)
      : ViewModelInstanceValueRuntime(viewModelInstance)
  {}
  void value(Font* font);
  const DataType dataType() override { return DataType::assetFont; }

#ifdef TESTING
  Font* testing_value();
#endif
};
} // namespace rive
#endif
