#include "third_party/rive/include/rive/importers/viewmodel_instance_list_importer.hpp"
#include "third_party/rive/include/rive/viewmodel/viewmodel_instance_list.hpp"
#include "third_party/rive/include/rive/viewmodel/viewmodel_instance_list_item.hpp"

using namespace rive;

ViewModelInstanceListImporter::ViewModelInstanceListImporter(
    ViewModelInstanceList* viewModelInstanceList)
    : m_ViewModelInstanceList(viewModelInstanceList)
{}
void ViewModelInstanceListImporter::addItem(
    rcp<ViewModelInstanceListItem> listItem)
{
  m_ViewModelInstanceList->internalAddItem(listItem);
}

StatusCode ViewModelInstanceListImporter::resolve() { return StatusCode::Ok; }