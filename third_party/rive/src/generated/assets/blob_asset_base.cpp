#include "third_party/rive/include/rive/generated/assets/blob_asset_base.hpp"
#include "third_party/rive/include/rive/assets/blob_asset.hpp"

using namespace rive;

Core* BlobAssetBase::clone() const
{
  auto cloned = new BlobAsset();
  cloned->copy(*this);
  return cloned;
}
