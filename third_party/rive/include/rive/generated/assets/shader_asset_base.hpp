#ifndef _RIVE_SHADER_ASSET_BASE_HPP_
#define _RIVE_SHADER_ASSET_BASE_HPP_
#include "third_party/rive/include/rive/assets/text_asset.hpp"
namespace rive
{
class ShaderAssetBase : public TextAsset
{
protected:
  typedef TextAsset Super;

public:
  static const uint16_t typeKey = 970;

  uint16_t coreType() const override { return typeKey; }

  Core* clone() const override;
};
} // namespace rive

#endif