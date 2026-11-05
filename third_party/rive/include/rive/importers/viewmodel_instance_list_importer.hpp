#ifndef _RIVE_VIEWMODEL_INSTANCE_LIST_IMPORTER_HPP_
#define _RIVE_VIEWMODEL_INSTANCE_LIST_IMPORTER_HPP_

#include "third_party/rive/include/rive/importers/import_stack.hpp"
#include "third_party/rive/include/rive/refcnt.hpp"

namespace rive
{
class Core;
class ViewModelInstanceList;
class ViewModelInstanceListItem;
class ViewModelInstanceListImporter : public ImportStackObject
{
private:
  ViewModelInstanceList* m_ViewModelInstanceList;

public:
  ViewModelInstanceListImporter(ViewModelInstanceList* viewModelInstanceList);
  void addItem(rcp<ViewModelInstanceListItem> listItem);
  StatusCode resolve() override;
};
} // namespace rive
#endif
