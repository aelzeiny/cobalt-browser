#include "third_party/rive/include/rive/inputs/semantic_input.hpp"
#include "third_party/rive/include/rive/animation/listener_types/listener_input_type_semantic.hpp"
#include "third_party/rive/include/rive/generated/animation/listener_types/listener_input_type_semantic_base.hpp"
#include "third_party/rive/include/rive/generated/artboard_base.hpp"
#include "third_party/rive/include/rive/importers/artboard_importer.hpp"
#include "third_party/rive/include/rive/importers/listener_input_type_semantic_importer.hpp"

using namespace rive;

StatusCode SemanticInput::import(ImportStack& importStack)
{
  auto* litImporter = importStack.latest<ListenerInputTypeSemanticImporter>(
      ListenerInputTypeSemanticBase::typeKey);
  if (litImporter == nullptr)
  {
    return StatusCode::MissingObject;
  }
  litImporter->listenerInputTypeSemantic()->addSemanticInput(this);

  // File::readObjects already put this in the artboard's object list (see
  // UserInput::claimsArtboardSlot), which owns it.
  auto artboardImporter = importStack.latest<ArtboardImporter>(
      ArtboardBase::typeKey);
  if (artboardImporter == nullptr)
  {
    return StatusCode::MissingObject;
  }
  return Super::import(importStack);
}
