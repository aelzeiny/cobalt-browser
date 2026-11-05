#include "third_party/rive/include/rive/generated/assets/shader_asset_base.hpp"
#include "third_party/rive/include/rive/assets/shader_asset.hpp"

using namespace rive;

Core* ShaderAssetBase::clone() const
{
  auto cloned = new ShaderAsset();
  cloned->copy(*this);
  return cloned;
}
