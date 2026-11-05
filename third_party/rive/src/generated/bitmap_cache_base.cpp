#include "third_party/rive/include/rive/generated/bitmap_cache_base.hpp"
#include "third_party/rive/include/rive/bitmap_cache.hpp"

using namespace rive;

Core* BitmapCacheBase::clone() const
{
  auto cloned = new BitmapCache();
  cloned->copy(*this);
  return cloned;
}
