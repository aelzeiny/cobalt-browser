#include "third_party/rive/include/rive/generated/artboard_list_map_rule_base.hpp"
#include "third_party/rive/include/rive/artboard_list_map_rule.hpp"

using namespace rive;

Core* ArtboardListMapRuleBase::clone() const
{
  auto cloned = new ArtboardListMapRule();
  cloned->copy(*this);
  return cloned;
}
