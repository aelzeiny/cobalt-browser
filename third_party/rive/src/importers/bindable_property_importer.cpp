#include "third_party/rive/include/rive/artboard.hpp"
#include "third_party/rive/include/rive/importers/bindable_property_importer.hpp"
#include "third_party/rive/include/rive/data_bind/bindable_property.hpp"

using namespace rive;

BindablePropertyImporter::BindablePropertyImporter(
    BindableProperty* bindableProperty)
    : m_bindableProperty(bindableProperty)
{}