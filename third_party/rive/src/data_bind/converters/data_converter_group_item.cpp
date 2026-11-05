#include "third_party/rive/include/rive/backboard.hpp"
#include "third_party/rive/include/rive/math/math_types.hpp"
#include "third_party/rive/include/rive/data_bind/converters/data_converter_group_item.hpp"
#include "third_party/rive/include/rive/data_bind/converters/data_converter_group.hpp"
#include "third_party/rive/include/rive/importers/data_converter_group_importer.hpp"
#include "third_party/rive/include/rive/importers/backboard_importer.hpp"

using namespace rive;

DataConverterGroupItem::~DataConverterGroupItem()
{
  if (m_ownsConverter)
  {

    delete m_dataConverter;
  }
}

StatusCode DataConverterGroupItem::import(ImportStack& importStack)
{
  auto backboardImporter = importStack.latest<BackboardImporter>(
      Backboard::typeKey);
  if (backboardImporter == nullptr)
  {
    return StatusCode::MissingObject;
  }
  backboardImporter->addDataConverterGroupItemReferencer(this);
  auto dataConveterGroupImporter = importStack
                                       .latest<DataConverterGroupImporter>(
                                           DataConverterGroupBase::typeKey);
  if (dataConveterGroupImporter == nullptr)
  {
    return StatusCode::MissingObject;
  }
  dataConveterGroupImporter->group()->addItem(this);
  return Super::import(importStack);
}

Core* DataConverterGroupItem::clone() const
{
  auto cloned = DataConverterGroupItemBase::clone()
                    ->as<DataConverterGroupItem>();
  if (converter() != nullptr)
  {
    cloned->converter(converter()->clone()->as<DataConverter>());
    cloned->ownsConverter(true);
  }
  return cloned;
}

#ifdef WITH_RIVE_EDITOR
void DataConverterGroupItem::editorPrepareForDestruction()
{
  if (m_dataConverter != nullptr)
  {
    if (m_ownsConverter && m_dataConverter->editorArena() == nullptr)
      m_dataConverter->editorPrepareForDestruction();
    else
      m_dataConverter = nullptr;
  }
}
#endif
