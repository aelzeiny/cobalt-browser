#include "third_party/rive/include/rive/text/text_style_feature.hpp"
#include "third_party/rive/include/rive/text/text_style.hpp"
#include "third_party/rive/include/rive/container_component.hpp"

using namespace rive;

StatusCode TextStyleFeature::onAddedDirty(CoreContext* context)
{
  StatusCode code = Super::onAddedDirty(context);
  if (code == StatusCode::Ok)
  {
    if (!parent()->is<TextStyle>())
    {
      return StatusCode::InvalidObject;
    }
    auto style = parent()->as<TextStyle>();
    style->addFeature(this);
  }
  return code;
}