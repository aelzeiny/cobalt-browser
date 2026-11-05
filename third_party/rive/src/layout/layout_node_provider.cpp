#include "third_party/rive/include/rive/artboard_component_list.hpp"
#include "third_party/rive/include/rive/component.hpp"
#include "third_party/rive/include/rive/constraints/layout_constraint.hpp"
#include "third_party/rive/include/rive/layout_component.hpp"
#include "third_party/rive/include/rive/layout/layout_node_provider.hpp"
#include "third_party/rive/include/rive/layout/layout_participant.hpp"
#include "third_party/rive/include/rive/nested_artboard_layout.hpp"
#include "third_party/rive/include/rive/text/text.hpp"
#include "third_party/rive/include/rive/shapes/image.hpp"
#include "third_party/rive/include/rive/shapes/shape.hpp"
#include "third_party/rive/include/rive/solo.hpp"

using namespace rive;

LayoutNodeProvider* LayoutNodeProvider::from(Component* component)
{
  if (component == nullptr)
  {
    return nullptr;
  }
  switch (component->coreType())
  {
    case LayoutComponent::typeKey:
      return component->as<LayoutComponent>();
    case NestedArtboardLayout::typeKey:
      return component->as<NestedArtboardLayout>();
    case ArtboardComponentListBase::typeKey:
      return component->as<ArtboardComponentList>();
    case TextBase::typeKey:
      // A migrated Text is no longer a provider itself; it
      // provides via its optional LayoutParticipant child.
      return component->as<Text>()->layoutParticipant();
    case ImageBase::typeKey:
      // A migrated Image is no longer a provider itself; it
      // provides via its optional LayoutParticipant child.
      return component->as<Image>()->layoutParticipant();
    case ShapeBase::typeKey:
      // A migrated Shape is no longer a provider itself; it
      // provides via its optional LayoutParticipant child.
      return component->as<Shape>()->layoutParticipant();
  }
  return nullptr;
}

void LayoutNodeProvider::addLayoutConstraint(LayoutConstraint* constraint)
{
  if (m_layoutConstraints == nullptr)
  {
    m_layoutConstraints = new std::vector<LayoutConstraint*>();
  }
  assert(
      std::find(
          m_layoutConstraints->begin(),
          m_layoutConstraints->end(),
          constraint) == m_layoutConstraints->end());
  m_layoutConstraints->push_back(constraint);
  constraint->addLayoutChild(this);
}