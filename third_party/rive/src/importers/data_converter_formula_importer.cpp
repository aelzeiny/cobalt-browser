#include "third_party/rive/include/rive/artboard.hpp"
#include "third_party/rive/include/rive/importers/data_converter_formula_importer.hpp"
#include "third_party/rive/include/rive/data_bind/converters/data_converter_formula.hpp"

using namespace rive;

DataConverterFormulaImporter::DataConverterFormulaImporter(
    DataConverterFormula* formula)
    : m_dataConverterFormula(formula)
{}

StatusCode DataConverterFormulaImporter::resolve()
{
  m_dataConverterFormula->calculateFormula();
  return StatusCode::Ok;
}