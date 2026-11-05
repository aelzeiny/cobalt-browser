#include <sstream>
#include <iomanip>
#include <array>

#include "third_party/rive/include/rive/viewmodel/data_enum.hpp"
#include "third_party/rive/include/rive/viewmodel/data_enum_custom.hpp"
#include "third_party/rive/include/rive/viewmodel/data_enum_value.hpp"
#include "third_party/rive/include/rive/importers/enum_importer.hpp"

using namespace rive;

StatusCode DataEnumValue::import(ImportStack& importStack)
{
  auto enumImporter = importStack.latest<EnumImporter>(DataEnumCustom::typeKey);
  if (enumImporter == nullptr)
  {
    return StatusCode::MissingObject;
  }

  enumImporter->addValue(this);
  return Super::import(importStack);
}