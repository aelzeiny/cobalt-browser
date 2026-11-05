#include "third_party/rive/include/rive/assets/file_asset_referencer.hpp"
#include "third_party/rive/include/rive/backboard.hpp"
#include "third_party/rive/include/rive/assets/file_asset.hpp"
#include "third_party/rive/include/rive/importers/backboard_importer.hpp"

using namespace rive;

FileAssetReferencer::~FileAssetReferencer()
{
  if (m_fileAsset != nullptr)
  {
    m_fileAsset->removeFileAssetReferencer(this);
  }
}

StatusCode FileAssetReferencer::registerReferencer(ImportStack& importStack)
{
  auto backboardImporter = importStack.latest<BackboardImporter>(
      Backboard::typeKey);
  if (backboardImporter == nullptr)
  {
    return StatusCode::MissingObject;
  }
  backboardImporter->addFileAssetReferencer(this);

  return StatusCode::Ok;
}

void FileAssetReferencer::setAsset(rcp<FileAsset> asset)
{
  if (m_fileAsset != nullptr)
  {
    m_fileAsset->removeFileAssetReferencer(this);
  }
  m_fileAsset = asset;
  if (asset != nullptr)
  {
    asset->addFileAssetReferencer(this);
  }
};