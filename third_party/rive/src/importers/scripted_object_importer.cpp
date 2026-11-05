#include "third_party/rive/include/rive/artboard.hpp"
#include "third_party/rive/include/rive/custom_property.hpp"
#include "third_party/rive/include/rive/assets/script_asset.hpp"
#include "third_party/rive/include/rive/scripted/scripted_object.hpp"
#include "third_party/rive/include/rive/importers/scripted_object_importer.hpp"

using namespace rive;

ScriptedObjectImporter::ScriptedObjectImporter(
    ScriptedObject* obj,
    uint32_t inputParentId)
    : m_scriptedObject(obj)
    , m_inputParentId(inputParentId)
{}

void ScriptedObjectImporter::addInput(CustomProperty* value)
{
  auto input = ScriptInput::from(value);
  if (input != nullptr)
  {
    m_scriptedObject->addProperty(value);
  }
}

StatusCode ScriptedObjectImporter::resolve() { return StatusCode::Ok; }