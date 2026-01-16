/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define MLOG_TAG "Media_Cloud_Service"

#include "cloud_media_photos_rename_service.h"

#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_unistore_manager.h"

namespace OHOS::Media::CloudSync {
int32_t CloudMediaPhotosRenameService::FindTitleAndSuffix(
    const std::string &displayName, std::string &title, std::string &suffix)
{
    title = displayName;
    suffix = "";
    size_t dotPos = displayName.rfind('.');
    CHECK_AND_RETURN_RET_LOG(
        dotPos != std::string::npos, E_OK, "displayName have no suffix, displayName: %{public}s.", displayName.c_str());
    title = displayName.substr(0, dotPos);
    suffix = displayName.substr(dotPos);
    return E_OK;
}

int32_t CloudMediaPhotosRenameService::FindNextDisplayName(const PhotosPo &photoInfo, std::string &nextDisplayName)
{
    // need to find the next file name from database.
    PhotoAssetInfo photoAssetInfo;
    photoAssetInfo.displayName = photoInfo.displayName.value_or("");
    photoAssetInfo.ownerAlbumId = photoInfo.ownerAlbumId.value_or(0);
    photoAssetInfo.subtype = photoInfo.subtype.value_or(0);
    // Find the unique display name for the photo.
    std::shared_ptr<MediaLibraryRdbStore> rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    nextDisplayName = this->displayNameOperation_.SetTargetPhotoInfo(photoAssetInfo).FindDisplayName(rdbStore);
    return E_OK;
}

int32_t CloudMediaPhotosRenameService::FindNextStoragePath(
    const PhotosPo &photoInfo, const std::string &nextDisplayName, std::string &nextStoragePath)
{
    std::string storagePath = photoInfo.storagePath.value_or("");
    bool isValid = !storagePath.empty() && !nextDisplayName.empty();
    CHECK_AND_RETURN_RET(isValid, E_DATA); // Ignore
    auto index = storagePath.rfind('/');
    isValid = index != std::string::npos;
    CHECK_AND_RETURN_RET_LOG(isValid, E_DATA, "FindNextStoragePath, storagePath is invalid.");
    std::string newStoragePath = storagePath.substr(0, index) + "/" + nextDisplayName;
    isValid = rename(storagePath.c_str(), newStoragePath.c_str()) == 0;
    CHECK_AND_RETURN_RET_LOG(isValid,
        E_DATA,
        "Failed to rename local anco file, file path: %{public}s, errno: %{public}d",
        storagePath.c_str(),
        errno);
    nextStoragePath = newStoragePath;
    MEDIA_INFO_LOG("FindNextStoragePath, rename lake asset, storagePath:%{public}s, newStoragePath: %{public}s",
        storagePath.c_str(),
        newStoragePath.c_str());
    return E_OK;
}

int32_t CloudMediaPhotosRenameService::RenameAsset(const PhotosPo &photoInfo, const std::string &nextDisplayName,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    // Check conditions.
    CHECK_AND_RETURN_RET_LOG(!nextDisplayName.empty(), E_DATA, "nextDisplayName is too short!");
    std::string nextTitle = "";
    std::string suffix = "";
    int32_t ret = this->FindTitleAndSuffix(nextDisplayName, nextTitle, suffix);
    CHECK_AND_RETURN_RET_LOG(!nextTitle.empty(), E_DATA, "nextTitle is too short!");
    // Update displayName of Asset.
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL, "rename same name get store failed.");
    NativeRdb::ValuesBucket values;
    std::string nextStoragePath;
    ret = this->FindNextStoragePath(photoInfo, nextDisplayName, nextStoragePath);
    if (ret == E_OK && !nextStoragePath.empty()) {
        values.PutString(PhotoColumn::PHOTO_STORAGE_PATH, nextStoragePath);
    }
    values.PutString(PhotoColumn::MEDIA_TITLE, nextTitle);
    values.PutString(PhotoColumn::MEDIA_NAME, nextDisplayName);
    int32_t changedRows = -1;
    std::vector<std::string> whereArgs = {std::to_string(photoInfo.fileId.value_or(0))};
    std::string whereClause = PhotoColumn::MEDIA_ID + " = ?";
    ret = photoRefresh->Update(changedRows, PhotoColumn::PHOTOS_TABLE, values, whereClause, whereArgs);
    MEDIA_INFO_LOG("RenameAsset, ret: %{public}d, changedRows: %{public}d, "
        "fileId: %{public}d, cloudId: %{public}s",
        ret,
        changedRows,
        photoInfo.fileId.value_or(0),
        photoInfo.cloudId.value_or("").c_str());
    bool isSuccess = (ret == E_OK) && changedRows > 0;
    return isSuccess ? E_OK : E_DATA;
}

int32_t CloudMediaPhotosRenameService::HandleSameNameRename(
    const PhotosDto &photo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    // Check conditions.
    bool isValid = photo.localInfoOp.has_value();
    CHECK_AND_RETURN_RET_LOG(isValid, E_DATA, "HandleSameNameRename, localInfoOp has no value.");
    PhotosPo photoInfo = photo.localInfoOp.value();
    // Generate next displayName.
    std::string nextDisplayName;
    int32_t ret = this->FindNextDisplayName(photoInfo, nextDisplayName);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "FindNextDisplayName error, ret: %{public}d", ret);
    // Rename displayName of Asset.
    ret = this->RenameAsset(photoInfo, nextDisplayName, photoRefresh);
    MEDIA_INFO_LOG("HandleSameNameRename completed, "
        "ret: %{public}d, fileId: %{public}d, cloudId: %{public}s, "
        "displayName(old): %{public}s, displayName(new): %{public}s",
        ret,
        photo.fileId,
        photo.cloudId.c_str(),
        photoInfo.displayName.value_or("").c_str(),
        nextDisplayName.c_str());
    return ret;
}
} // namespace OHOS::Media::CloudSync