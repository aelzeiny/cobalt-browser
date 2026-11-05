#ifndef _RIVE_VIEW_MODEL_INSTANCE_ASSET_IMAGE_HPP_
#define _RIVE_VIEW_MODEL_INSTANCE_ASSET_IMAGE_HPP_
#include "third_party/rive/include/rive/generated/viewmodel/viewmodel_instance_asset_image_base.hpp"
#include "third_party/rive/include/rive/renderer.hpp"
#include "third_party/rive/include/rive/refcnt.hpp"
#include "third_party/rive/include/rive/data_bind/data_values/data_value_integer.hpp"
#include "third_party/rive/include/rive/assets/image_asset.hpp"
#include <stdio.h>
namespace rive
{
class ViewModelInstanceAssetImage : public ViewModelInstanceAssetImageBase
{
protected:
  void propertyValueChanged() override;

public:
  ViewModelInstanceAssetImage();
  void value(RenderImage* image);
  rcp<ImageAsset> asset() { return m_imageAsset; }
  Core* clone() const override;
  void applyValue(DataValueInteger*);

private:
  rcp<ImageAsset> m_imageAsset = nullptr;
};
} // namespace rive

#endif