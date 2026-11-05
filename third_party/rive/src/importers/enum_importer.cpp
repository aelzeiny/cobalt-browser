#include "third_party/rive/include/rive/importers/enum_importer.hpp"
#include "third_party/rive/include/rive/viewmodel/data_enum.hpp"
#include "third_party/rive/include/rive/viewmodel/data_enum_value.hpp"

using namespace rive;

EnumImporter::EnumImporter(DataEnum* dataEnum) : m_DataEnum(dataEnum) {}

void EnumImporter::addValue(DataEnumValue* value)
{
  m_DataEnum->addValue(value);
}

StatusCode EnumImporter::resolve() { return StatusCode::Ok; }