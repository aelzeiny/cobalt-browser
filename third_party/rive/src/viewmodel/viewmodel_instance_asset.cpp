#include <sstream>
#include <iomanip>
#include <array>

#include "third_party/rive/include/rive/viewmodel/viewmodel_instance_asset.hpp"
#include "third_party/rive/include/rive/viewmodel/viewmodel_instance.hpp"
#include "third_party/rive/include/rive/importers/backboard_importer.hpp"
#include "third_party/rive/include/rive/backboard.hpp"
#include "third_party/rive/include/rive/component_dirt.hpp"
#include "third_party/rive/include/rive/refcnt.hpp"

using namespace rive;

StatusCode ViewModelInstanceAsset::import(ImportStack& importStack)
{
  auto backboardImporter = importStack.latest<BackboardImporter>(
      Backboard::typeKey);
  if (backboardImporter == nullptr)
  {
    return StatusCode::MissingObject;
  }
  auto assets = backboardImporter->assets();
  for (const auto& asset : *assets)
  {
    m_assets.push_back(asset);
  }
  return Super::import(importStack);
}