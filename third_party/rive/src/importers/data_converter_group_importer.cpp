#include "third_party/rive/include/rive/artboard.hpp"
#include "third_party/rive/include/rive/importers/data_converter_group_importer.hpp"
#include "third_party/rive/include/rive/data_bind/converters/data_converter.hpp"

using namespace rive;

DataConverterGroupImporter::DataConverterGroupImporter(
    DataConverterGroup* group)
    : m_dataConverterGroup(group)
{}