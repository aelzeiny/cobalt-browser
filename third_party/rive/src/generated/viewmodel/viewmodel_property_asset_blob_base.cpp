#include "third_party/rive/include/rive/generated/viewmodel/viewmodel_property_asset_blob_base.hpp"
#include "third_party/rive/include/rive/viewmodel/viewmodel_property_asset_blob.hpp"

using namespace rive;

Core* ViewModelPropertyAssetBlobBase::clone() const
{
  auto cloned = new ViewModelPropertyAssetBlob();
  cloned->copy(*this);
  return cloned;
}
