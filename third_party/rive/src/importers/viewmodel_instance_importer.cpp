#include "third_party/rive/include/rive/importers/viewmodel_instance_importer.hpp"
#include "third_party/rive/include/rive/viewmodel/viewmodel_instance.hpp"
#include "third_party/rive/include/rive/viewmodel/viewmodel_instance_value.hpp"

using namespace rive;

ViewModelInstanceImporter::ViewModelInstanceImporter(
    ViewModelInstance* viewModelInstance)
    : m_ViewModelInstance(viewModelInstance)
{}
void ViewModelInstanceImporter::addValue(ViewModelInstanceValue* value)
{
  m_ViewModelInstance->addValue(value);
}

StatusCode ViewModelInstanceImporter::resolve() { return StatusCode::Ok; }