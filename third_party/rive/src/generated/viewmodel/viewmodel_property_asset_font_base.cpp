#include "third_party/rive/include/rive/generated/viewmodel/viewmodel_property_asset_font_base.hpp"
#include "third_party/rive/include/rive/viewmodel/viewmodel_property_asset_font.hpp"

using namespace rive;

Core* ViewModelPropertyAssetFontBase::clone() const
{
  auto cloned = new ViewModelPropertyAssetFont();
  cloned->copy(*this);
  return cloned;
}
