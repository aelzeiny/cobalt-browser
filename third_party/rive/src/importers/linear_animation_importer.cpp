#include "third_party/rive/include/rive/importers/linear_animation_importer.hpp"
#include "third_party/rive/include/rive/animation/keyed_object.hpp"
#include "third_party/rive/include/rive/animation/linear_animation.hpp"
#include "third_party/rive/include/rive/artboard.hpp"

using namespace rive;

LinearAnimationImporter::LinearAnimationImporter(LinearAnimation* animation)
    : m_Animation(animation)
{
  // Its keyed properties are imported next; a callback among them sets this
  // again (see KeyedProperty::import).
  m_Animation->hasKeyedCallbacks(false);
}

void LinearAnimationImporter::addKeyedObject(
    std::unique_ptr<KeyedObject> object)
{
  m_Animation->addKeyedObject(std::move(object));
}