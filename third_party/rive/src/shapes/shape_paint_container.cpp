#include "third_party/rive/include/rive/shapes/shape_paint_container.hpp"
#include "third_party/rive/include/rive/artboard.hpp"
#include "third_party/rive/include/rive/factory.hpp"
#include "third_party/rive/include/rive/component.hpp"
#include "third_party/rive/include/rive/layout_component.hpp"
#include "third_party/rive/include/rive/foreground_layout_drawable.hpp"
#include "third_party/rive/include/rive/shapes/paint/shape_paint.hpp"
#include "third_party/rive/include/rive/shapes/shape.hpp"
#include "third_party/rive/include/rive/text/text_style_background.hpp"
#include "third_party/rive/include/rive/text/text_style_paint.hpp"
#include "third_party/rive/include/rive/text/text_input_selected_text.hpp"
#include "third_party/rive/include/rive/text/text_input_cursor.hpp"
#include "third_party/rive/include/rive/text/text_input_selection.hpp"
#include "third_party/rive/include/rive/text/text_input_text.hpp"
#ifdef WITH_RIVE_EDITOR
#include <algorithm>
#endif

using namespace rive;

ShapePaintContainer* ShapePaintContainer::from(Component* component)
{
  switch (component->coreType())
  {
    case Artboard::typeKey:
      return component->as<Artboard>();
    case LayoutComponent::typeKey:
      return component->as<LayoutComponent>();
    case Shape::typeKey:
      return component->as<Shape>();
    case TextStylePaint::typeKey:
      return component->as<TextStylePaint>();
    case TextStyleBackground::typeKey:
      return component->as<TextStyleBackground>();
    case ForegroundLayoutDrawable::typeKey:
      return component->as<ForegroundLayoutDrawable>();
    case TextInputCursor::typeKey:
      return component->as<TextInputCursor>();
    case TextInputSelection::typeKey:
      return component->as<TextInputSelection>();
    case TextInputText::typeKey:
      return component->as<TextInputText>();
    case TextInputSelectedText::typeKey:
      return component->as<TextInputSelectedText>();
  }
  return nullptr;
}

void ShapePaintContainer::addPaint(ShapePaint* paint)
{
#ifdef WITH_RIVE_EDITOR
  // RIV initialization runs editorParentChanged before onAddedClean, and
  // both can register the same paint. Keep the shared registration path
  // idempotent; duplicate entries draw the fill/stroke more than once.
  if (std::find(m_ShapePaints.begin(), m_ShapePaints.end(), paint) !=
      m_ShapePaints.end())
  {
    return;
  }
#endif
  m_ShapePaints.push_back(paint);
}

PathFlags ShapePaintContainer::pathFlags() const
{
  PathFlags space = m_pathFlags;
  for (auto paint : m_ShapePaints)
  {
    space |= paint->pathFlags();
  }
  return space;
}

void ShapePaintContainer::invalidateStrokeEffects()
{
  for (auto paint : m_ShapePaints)
  {
    if (paint->is<ShapePaint>())
    {
      paint->as<ShapePaint>()->invalidateEffects();
    }
  }
}

void ShapePaintContainer::propagateOpacity(float opacity)
{
  for (auto shapePaint : m_ShapePaints)
  {
    shapePaint->renderOpacity(opacity);
  }
}
