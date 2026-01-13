/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_Service"

#include "media_assets_delete_service.h"

#include "media_log.h"
#include "medialibrary_type_const.h"
#include "medialibrary_db_const.h"
#include "medialibrary_album_fusion_utils.h"
#include "photos_po_writer.h"
#include "photo_file_operation.h"
#include "media_assets_service.h"
#include "media_assets_utils.h"
#include "medialibrary_asset_operations.h"
#include "lake_file_operations.h"
#include "dfx_utils.h"
#include "lake_file_utils.h"
#include "medialibrary_notify.h"

namespace OHOS::Media::Common {
int32_t MediaAssetsDeleteService::DeleteLocalAssets(const std::vector<std::string> &fileIds)
{
    // Ensure the process is single-threaded.
    std::lock_guard<std::mutex> lock(this->deleteAssetsMutex_);
    // Find assets info.
    std::vector<PhotosPo> photosList;
    int32_t ret = this->mediaAssetsDao_.QueryAssets(fileIds, photosList);
    CHECK_AND_RETURN_RET_LOG(
        ret == E_OK, ret, "QueryAssets fail, ret: %{public}d, fileIds: %{public}zu", ret, fileIds.size());
    // Aggreate all the target fileIds.
    std::vector<std::string> targetFileIds;
    // Handle the LOCAL_AND_CLOUD assets, provide the LOCAL copy.
    ret = this->BatchCopyAndMoveLocalAssetToTrash(photosList, targetFileIds);
    CHECK_AND_RETURN_RET_LOG(
        !targetFileIds.empty(), E_OK, "No need to handle, all are CLOUD. fileIds: %{public}zu", fileIds.size());
    // Move the assets to trash.
    ret = MediaAssetsService::GetInstance().TrashPhotos(targetFileIds);
    MEDIA_INFO_LOG("DeleteLocalAssets completed, ret: %{public}d, fileIds size: %{public}zu, "
                   "photosList: %{public}zu, targetFileIds: %{public}zu",
        ret,
        fileIds.size(),
        photosList.size(),
        targetFileIds.size());
    return ret;
}

int32_t MediaAssetsDeleteService::BatchCopyAndMoveLocalAssetToTrash(
    const std::vector<PhotosPo> &photosList, std::vector<std::string> &targetFileIds)
{
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> photoRefresh =
        std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(
        photoRefresh != nullptr, E_RDB_STORE_NULL, "BatchCopyAndMoveLocalAssetToTrash Failed to get photoRefresh.");
    int32_t ret = E_OK;
    bool isValid = false;
    std::optional<PhotosPo> targetPhotoInfoOp;
    bool isCoverAsset = false;
    for (const PhotosPo &photoInfo : photosList) {
        // Only process cover asset.
        isCoverAsset = photoInfo.burstCoverLevel.value_or(1) == 1;
        CHECK_AND_CONTINUE_INFO_LOG(isCoverAsset,
            "Skip member asset. fileId: %{public}d, position: %{public}d, cloudId: %{public}s, burstKey: %{public}s",
            photoInfo.fileId.value_or(-1),
            photoInfo.position.value_or(-1),
            photoInfo.cloudId.value_or("").c_str(),
            photoInfo.burstKey.value_or("").c_str());
        // Fdirty asset should move to trash directly.
        isValid = photoInfo.dirty.value_or(0) != static_cast<int32_t>(DirtyType::TYPE_FDIRTY);
        CHECK_AND_EXECUTE(isValid, targetFileIds.emplace_back(photoInfo.BuildFileUri()));
        CHECK_AND_CONTINUE_INFO_LOG(isValid,
            "Delete directly. fileId: %{public}d, position: %{public}d, cloudId: %{public}s",
            photoInfo.fileId.value_or(-1),
            photoInfo.position.value_or(-1),
            photoInfo.cloudId.value_or("").c_str());
        targetPhotoInfoOp.reset();
        // Only handle the LOCAL_AND_CLOUD assets.
        ret = this->DeleteLocalAssetSingle(photoInfo, targetPhotoInfoOp, photoRefresh);
        isValid = ret == E_OK && targetPhotoInfoOp.has_value();
        CHECK_AND_EXECUTE(!isValid, targetFileIds.emplace_back(targetPhotoInfoOp.value().BuildFileUri()));
        // Handle the LOCAL assets, ignore the CLOUD assets.
        isValid = photoInfo.position.value_or(1) == static_cast<int32_t>(PhotoPositionType::LOCAL);
        CHECK_AND_EXECUTE(!isValid, targetFileIds.emplace_back(photoInfo.BuildFileUri()));
    }
    photoRefresh->RefreshAlbumNoDateModified(static_cast<NotifyAlbumType>(
        NotifyAlbumType::SYS_ALBUM | NotifyAlbumType::USER_ALBUM | NotifyAlbumType::SOURCE_ALBUM));
    photoRefresh->Notify();
    return E_OK;
}

int32_t MediaAssetsDeleteService::CopyAndMoveLocalAssetToTrash(const PhotosPo &photoInfo,
    std::optional<PhotosPo> &targetPhotoInfoOp, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    CHECK_AND_RETURN_RET_LOG(
        photoRefresh != nullptr, E_RDB_STORE_NULL, "CopyAndMoveLocalAssetToTrash Failed to get photoRefresh.");
    bool isValid = photoInfo.position.value_or(1) == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    CHECK_AND_RETURN_RET_LOG(isValid,
        E_OK,
        "No need to handle, not LOCAL_AND_CLOUD. fileId: %{public}d, position: %{public}d, cloudId: %{public}s",
        photoInfo.fileId.value_or(-1),
        photoInfo.position.value_or(-1),
        photoInfo.cloudId.value_or("").c_str());
    bool isLocalDirty = photoInfo.dirty.value_or(0) == static_cast<int32_t>(DirtyType::TYPE_FDIRTY);
    CHECK_AND_RETURN_RET_LOG(!isLocalDirty,
        E_OK,
        "Can not handle, file-modified. "
        "fileId: %{public}d, position: %{public}d, dirty: %{public}d, cloudId: %{public}s",
        photoInfo.fileId.value_or(-1),
        photoInfo.position.value_or(-1),
        photoInfo.dirty.value_or(0),
        photoInfo.cloudId.value_or("").c_str());
    const std::vector<DeleteFuncHandle> deleteFuncs = {
        &MediaAssetsDeleteService::CopyAndMoveMediaLocalAssetToTrash,
        &MediaAssetsDeleteService::CopyAndMoveLakeLocalAssetToTrash,
    };
    int32_t ret = E_INVALID_MODE;
    for (auto deleteFunc : deleteFuncs) {  // Chain of responsibility.
        ret = (this->*(deleteFunc))(photoInfo, targetPhotoInfoOp, photoRefresh);
        CHECK_AND_BREAK(ret != E_OK);
    }
    MEDIA_INFO_LOG("CopyAndMoveLocalAssetToTrash completed, ret: %{public}d, "
        "fileId: %{public}d, position: %{public}d, cloudId: %{public}s",
        ret,
        photoInfo.fileId.value_or(-1),
        photoInfo.position.value_or(-1),
        photoInfo.cloudId.value_or("").c_str());
    return ret;
}

int32_t MediaAssetsDeleteService::EraseCloudInfo(PhotosPo &photoInfo)
{
    photoInfo.cloudId.reset();
    photoInfo.dirty = static_cast<int32_t>(DirtyType::TYPE_NEW);
    photoInfo.position = static_cast<int32_t>(CloudFilePosition::POSITION_LOCAL);
    photoInfo.cloudVersion.reset();
    return E_OK;
}

int32_t MediaAssetsDeleteService::ResetFileId(PhotosPo &photoInfo)
{
    photoInfo.fileId.reset();
    return E_OK;
}

int32_t MediaAssetsDeleteService::ResetVirtualPath(PhotosPo &photoInfo)
{
    // Photos.virtual_path is deprecated and not used any more.
    // Keep the null field value to avoid the unique constraint of (virtual_path).
    photoInfo.virtualPath.reset();
    return E_OK;
}

int32_t MediaAssetsDeleteService::SetDateTrashed(PhotosPo &photoInfo, int64_t dateTrashed)
{
    photoInfo.dateTrashed = dateTrashed;
    return E_OK;
}

int32_t MediaAssetsDeleteService::SetPosition(PhotosPo &photoInfo, int32_t position)
{
    photoInfo.position = position;
    return E_OK;
}

int32_t MediaAssetsDeleteService::SetFilePath(PhotosPo &photoInfo, const std::string &filePath)
{
    photoInfo.data = filePath;
    return E_OK;
}

int32_t MediaAssetsDeleteService::SetFileId(PhotosPo &photoInfo, const int32_t fileId)
{
    photoInfo.fileId = fileId;
    return E_OK;
}

int32_t MediaAssetsDeleteService::ClearCloudInfo(PhotosPo &photoInfo)
{
    photoInfo.cloudId.reset();
    photoInfo.dirty = static_cast<int32_t>(DirtyType::TYPE_NEW);
    photoInfo.position = static_cast<int32_t>(PhotoPositionType::LOCAL);
    // south_device_type will be set to SOUTH_DEVICE_NULL by default.
    photoInfo.cloudVersion.reset();
    return E_OK;
}

int32_t MediaAssetsDeleteService::ResetNullableFields(PhotosPo &photoInfo)
{
    bool conn = photoInfo.burstKey.has_value() && photoInfo.burstKey.value().empty();
    CHECK_AND_EXECUTE(!conn, photoInfo.burstKey.reset());
    conn = photoInfo.originalAssetCloudId.has_value() && photoInfo.originalAssetCloudId.value().empty();
    CHECK_AND_EXECUTE(!conn, photoInfo.originalAssetCloudId.reset());
    conn = photoInfo.relativePath.has_value() && photoInfo.relativePath.value().empty();
    CHECK_AND_EXECUTE(!conn, photoInfo.relativePath.reset());
    conn = photoInfo.latitude.has_value() && photoInfo.latitude.value() == 0;
    conn = conn && photoInfo.longitude.has_value() && photoInfo.longitude.value() == 0;
    CHECK_AND_EXECUTE(!conn, photoInfo.latitude.reset());
    CHECK_AND_EXECUTE(!conn, photoInfo.longitude.reset());
    conn = photoInfo.userComment.has_value() && photoInfo.userComment.value().empty();
    CHECK_AND_EXECUTE(!conn, photoInfo.userComment.reset());
    return E_OK;
}

int32_t MediaAssetsDeleteService::DeleteCloudAssets(const std::vector<std::string> &fileIds)
{
    // Ensure the process is single-threaded.
    std::lock_guard<std::mutex> lock(this->deleteAssetsMutex_);
    // Find assets info.
    std::vector<PhotosPo> photosList;
    int32_t ret = this->mediaAssetsDao_.QueryAssets(fileIds, photosList);
    CHECK_AND_RETURN_RET_LOG(
        ret == E_OK, ret, "QueryAssets fail, ret: %{public}d, fileIds: %{public}zu", ret, fileIds.size());
    // Aggreate all the target fileIds.
    std::vector<std::string> targetFileIds;
    // Handle the LOCAL_AND_CLOUD assets, provide the CLOUD copy.
    ret = this->BatchCopyAndMoveCloudAssetToTrash(photosList, targetFileIds);
    CHECK_AND_RETURN_RET_LOG(
        !targetFileIds.empty(), E_OK, "No need to handle, all are LOCAL. fileIds: %{public}zu", fileIds.size());
    // Move the assets to trash.
    ret = MediaAssetsService::GetInstance().TrashPhotos(targetFileIds);
    MEDIA_INFO_LOG("DeleteCloudAssets completed, ret: %{public}d, fileIds size: %{public}zu, "
                   "photosList: %{public}zu, targetFileIds: %{public}zu",
        ret,
        fileIds.size(),
        photosList.size(),
        targetFileIds.size());
    return E_OK;
}

int32_t MediaAssetsDeleteService::BatchCopyAndMoveCloudAssetToTrash(
    const std::vector<PhotosPo> &photosList, std::vector<std::string> &targetFileIds)
{
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> photoRefresh =
        std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(
        photoRefresh != nullptr, E_RDB_STORE_NULL, "BatchCopyAndMoveCloudAssetToTrash Failed to get photoRefresh.");
    int32_t ret = E_OK;
    bool isValid = false;
    std::optional<PhotosPo> targetPhotoInfoOp;
    bool isCoverAsset = false;
    for (const PhotosPo &photoInfo : photosList) {
        // Only process cover asset.
        isCoverAsset = photoInfo.burstCoverLevel.value_or(1) == 1;
        CHECK_AND_CONTINUE_INFO_LOG(isCoverAsset,
            "Skip member asset. fileId: %{public}d, position: %{public}d, cloudId: %{public}s, burstKey: %{public}s",
            photoInfo.fileId.value_or(-1),
            photoInfo.position.value_or(-1),
            photoInfo.cloudId.value_or("").c_str(),
            photoInfo.burstKey.value_or("").c_str());
        // Fdirty asset should move to trash directly.
        isValid = photoInfo.dirty.value_or(0) != static_cast<int32_t>(DirtyType::TYPE_FDIRTY);
        CHECK_AND_EXECUTE(isValid, targetFileIds.emplace_back(photoInfo.BuildFileUri()));
        CHECK_AND_CONTINUE_INFO_LOG(isValid,
            "Delete directly. fileId: %{public}d, position: %{public}d, cloudId: %{public}s",
            photoInfo.fileId.value_or(-1),
            photoInfo.position.value_or(-1),
            photoInfo.cloudId.value_or("").c_str());
        targetPhotoInfoOp.reset();
        // Only handle the LOCAL_AND_CLOUD assets.
        ret = this->DeleteCloudAssetSingle(photoInfo, targetPhotoInfoOp, photoRefresh);
        isValid = ret == E_OK && targetPhotoInfoOp.has_value();
        CHECK_AND_EXECUTE(!isValid, targetFileIds.emplace_back(targetPhotoInfoOp.value().BuildFileUri()));
        // Handle the CLOUD assets, ignore the LOCAL assets.
        isValid = photoInfo.position.value_or(1) == static_cast<int32_t>(PhotoPositionType::CLOUD);
        CHECK_AND_EXECUTE(!isValid, targetFileIds.emplace_back(photoInfo.BuildFileUri()));
    }
    photoRefresh->RefreshAlbumNoDateModified(static_cast<NotifyAlbumType>(
        NotifyAlbumType::SYS_ALBUM | NotifyAlbumType::USER_ALBUM | NotifyAlbumType::SOURCE_ALBUM));
    photoRefresh->Notify();
    return E_OK;
}

int32_t MediaAssetsDeleteService::BuildTargetFilePath(const PhotosPo &photoInfo, std::string &targetPath)
{
    std::string storagePath = photoInfo.storagePath.value_or("");
    int32_t ret = E_OK;
    if (!storagePath.empty()) {
        ret = this->BuildLakeFilePath(photoInfo, targetPath);
    } else {
        ret = this->BuildMediaFilePath(photoInfo, targetPath);
    }
    return ret;
}

int32_t MediaAssetsDeleteService::CopyAndMoveCloudAssetToTrash(const PhotosPo &photoInfo,
    std::optional<PhotosPo> &targetPhotoInfoOp, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    CHECK_AND_RETURN_RET_LOG(
        photoRefresh != nullptr, E_RDB_STORE_NULL, "CopyAndMoveCloudAssetToTrash Failed to get photoRefresh.");
    bool isValid = photoInfo.position.value_or(1) == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    CHECK_AND_RETURN_RET_WARN_LOG(isValid,
        E_OK,
        "No need to handle, not LOCAL_AND_CLOUD. fileId: %{public}d, position: %{public}d, cloudId: %{public}s",
        photoInfo.fileId.value_or(-1),
        photoInfo.position.value_or(-1),
        photoInfo.cloudId.value_or("").c_str());
    bool isLocalDirty = photoInfo.dirty.value_or(0) == static_cast<int32_t>(DirtyType::TYPE_FDIRTY);
    CHECK_AND_RETURN_RET_LOG(!isLocalDirty,
        E_OK,
        "Can not handle, file-modified. "
        "fileId: %{public}d, position: %{public}d, dirty: %{public}d, cloudId: %{public}s",
        photoInfo.fileId.value_or(-1),
        photoInfo.position.value_or(-1),
        photoInfo.dirty.value_or(0),
        photoInfo.cloudId.value_or("").c_str());
    // 1. Copy the CLOUD asset record from LOCAL_AND_CLOUD asset record, and move it into trash.
    const std::vector<DeleteFuncHandle> deleteFuncs = {
        &MediaAssetsDeleteService::CopyAndMoveMediaCloudAssetToTrash,
        &MediaAssetsDeleteService::CopyAndMoveLakeCloudAssetToTrash,
    };
    int32_t ret = E_INVALID_MODE;
    for (auto deleteFunc : deleteFuncs) {  // Chain of responsibility.
        ret = (this->*(deleteFunc))(photoInfo, targetPhotoInfoOp, photoRefresh);
        CHECK_AND_BREAK(ret != E_OK);
    }
    MEDIA_INFO_LOG("CopyAndMoveCloudAssetToTrash completed, ret: %{public}d, "
        "fileId: %{public}d, position: %{public}d, cloudId: %{public}s",
        ret,
        photoInfo.fileId.value_or(-1),
        photoInfo.position.value_or(-1),
        photoInfo.cloudId.value_or("").c_str());
    return E_OK;
}

int32_t MediaAssetsDeleteService::CleanLocalFileAndCreateDentryFile(
    const PhotosPo &photoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    FileManagement::CloudSync::CleanFileInfo cleanFileInfo;
    int32_t ret = this->GetCleanFileInfo(photoInfo, cleanFileInfo);
    std::vector<std::string> failCloudIdList;
    ret = FileManagement::CloudSync::CloudSyncManager::GetInstance().BatchCleanFile({cleanFileInfo}, failCloudIdList);
    MEDIA_INFO_LOG("BatchCleanFile, ret: %{public}d, fileId: %{public}d, failSize: %{public}zu.",
        ret,
        photoInfo.fileId.value_or(-1),
        failCloudIdList.size());
    CHECK_AND_RETURN_RET(ret != E_OK, ret);
    // Case when BatchCleanFile exception, update position to cloud only.
    ret = this->mediaAssetsDao_.ResetPositionToCloudOnly(photoRefresh, photoInfo.fileId.value_or(-1));
    MEDIA_INFO_LOG("ResetPositionToCloudOnly, "
                   "ret: %{public}d, fileId: %{public}d.",
        ret,
        photoInfo.fileId.value_or(-1));
    return ret;
}

int32_t MediaAssetsDeleteService::GetCleanFileInfo(
    const PhotosPo &photoInfo, FileManagement::CloudSync::CleanFileInfo &cleanFileInfo)
{
    std::shared_ptr<FileAsset> fileAssetPtr = make_shared<FileAsset>();
    auto &map = fileAssetPtr->GetMemberMap();
    map[PhotoColumn::PHOTO_CLOUD_ID] = photoInfo.cloudId.value_or("");
    map[MediaColumn::MEDIA_SIZE] = photoInfo.size.value_or(0);
    map[MediaColumn::MEDIA_DATE_MODIFIED] = photoInfo.dateModified.value_or(0);
    map[MediaColumn::MEDIA_FILE_PATH] = photoInfo.data.value_or("");
    map[MediaColumn::MEDIA_NAME] = photoInfo.displayName.value_or("");
    map[MediaColumn::MEDIA_ID] = photoInfo.fileId.value_or(-1);
    map[PhotoColumn::PHOTO_POSITION] = photoInfo.position.value_or(-1);
    map[PhotoColumn::PHOTO_BURST_KEY] = photoInfo.burstKey.value_or("");
    map[PhotoColumn::PHOTO_SUBTYPE] = photoInfo.subtype.value_or(-1);
    map[PhotoColumn::MOVING_PHOTO_EFFECT_MODE] = photoInfo.movingPhotoEffectMode.value_or(-1);
    map[PhotoColumn::PHOTO_ORIGINAL_SUBTYPE] = photoInfo.originalSubtype.value_or(-1);
    map[PhotoColumn::PHOTO_EDIT_TIME] = photoInfo.editTime.value_or(0);
    map[PhotoColumn::PHOTO_FILE_SOURCE_TYPE] = photoInfo.fileSourceType.value_or(0);
    map[PhotoColumn::PHOTO_STORAGE_PATH] = photoInfo.storagePath.value_or("");
    cleanFileInfo = MediaLibraryAssetOperations::GetCleanFileInfo(fileAssetPtr);
    return E_OK;
}

// Copy the LOCAL asset record from LOCAL_AND_CLOUD asset record, and move it into trash.
int32_t MediaAssetsDeleteService::CreateLocalAssetWithFile(const PhotosPo &photoInfo, PhotosPo &targetPhotoInfo,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    int32_t ret = this->CreateLocalTrashedPhotosPo(photoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CreateLocalTrashedPhotosPo fail, ret: %{public}d", ret);
    ret = this->MoveLocalAssetFile(photoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CreateLocalAssetFile fail, ret: %{public}d", ret);
    // Create the new asset record in the database.
    return this->CreateNewAssetInfoAndReturnFileId(targetPhotoInfo, photoRefresh);
}

int32_t MediaAssetsDeleteService::CreateLocalTrashedPhotosPo(const PhotosPo &photoInfo, PhotosPo &targetPhotoInfo)
{
    std::string targetPath;
    int32_t ret = this->BuildTargetFilePath(photoInfo, targetPath);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    targetPhotoInfo = photoInfo;
    this->SetFilePath(targetPhotoInfo, targetPath);
    this->ResetFileId(targetPhotoInfo);
    this->ResetVirtualPath(targetPhotoInfo);
    this->SetDateTrashed(targetPhotoInfo, MediaFileUtils::UTCTimeMilliSeconds());
    this->ClearCloudInfo(targetPhotoInfo);
    this->ResetNullableFields(targetPhotoInfo);
    this->ResetFileSourceType(targetPhotoInfo);  // Set file_source_type to MEDIA (default).
    return E_OK;
}

// Move the origianl files from the LOCAL_AND_CLOUD asset to the LOCAL asset.
int32_t MediaAssetsDeleteService::MoveLocalAssetFile(const PhotosPo &photoInfo, const PhotosPo &targetPhotoInfo)
{
    PhotoFileOperation fileOperation;
    // Use Move instead of Copy, risk: if move fail, the source file is still in use by other process.
    int32_t ret = fileOperation.MovePhoto(photoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "MovePhoto fail, ret: %{public}d", ret);
    // Copy the thumnbail files from the LOCAL_AND_CLOUD asset to the LOCAL asset.
    ret = fileOperation.CopyThumbnail(photoInfo, targetPhotoInfo, false);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CopyThumbnail fail, ret: %{public}d", ret);
    MEDIA_INFO_LOG("CreateLocalAssetFile completed, "
                   "sourceFileId: %{public}d, targetFileId: %{public}d, auditLog: %{public}s",
        photoInfo.fileId.value_or(0),
        targetPhotoInfo.fileId.value_or(0),
        fileOperation.GetAuditLog().c_str());
    return E_OK;
}

int32_t MediaAssetsDeleteService::CreateCloudTrashedPhotosPo(const PhotosPo &photoInfo, PhotosPo &targetPhotoInfo)
{
    std::string targetPath;
    int32_t ret = this->BuildTargetFilePath(photoInfo, targetPath);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    targetPhotoInfo = photoInfo;
    this->SetFilePath(targetPhotoInfo, targetPath);
    this->ResetFileId(targetPhotoInfo);
    this->ResetVirtualPath(targetPhotoInfo);
    this->SetDateTrashed(targetPhotoInfo, MediaFileUtils::UTCTimeMilliSeconds());
    this->SetPosition(targetPhotoInfo, static_cast<int32_t>(PhotoPositionType::CLOUD));
    this->ResetNullableFields(targetPhotoInfo);
    // Scenario, Cloud pulled data, require targetPhotoInfo's dirty should be same as photoInfo.
    CHECK_AND_EXECUTE(this->isCloudPullData_, this->SetMdirty(targetPhotoInfo));
    this->ResetFileSourceType(targetPhotoInfo);  // Set file_source_type to MEDIA (default).
    return E_OK;
}

int32_t MediaAssetsDeleteService::CreateCloudAssetThumbnail(const PhotosPo &photoInfo, const PhotosPo &targetPhotoInfo)
{
    // Copy the thumnbail files from the LOCAL_AND_CLOUD asset to the LOCAL asset.
    PhotoFileOperation fileOperation;
    int32_t ret = fileOperation.CopyThumbnail(photoInfo, targetPhotoInfo, false);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CopyThumbnail fail, ret: %{public}d", ret);
    MEDIA_INFO_LOG("CreateCloudAssetThumbnail completed, "
                   "sourceFileId: %{public}d, targetFileId: %{public}d, auditLog: %{public}s",
        photoInfo.fileId.value_or(0),
        targetPhotoInfo.fileId.value_or(0),
        fileOperation.GetAuditLog().c_str());
    return E_OK;
}

int32_t MediaAssetsDeleteService::CreateCloudAssetWithDentryFile(const PhotosPo &photoInfo, PhotosPo &targetPhotoInfo,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    int32_t ret = this->CreateCloudTrashedPhotosPo(photoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CreateCloudTrashedPhotosPo fail, ret: %{public}d", ret);
    ret = this->CreateCloudAssetThumbnail(photoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CreateCloudAssetThumbnail fail, ret: %{public}d", ret);
    // Must Clean the cloud info of original asset firstly, to avoid duplicate cloud info in the database.
    // Clear cloud info of the LOCAL_AND_CLOUD asset record to LOCAL asset record (local only).
    ret = this->ResetPhotosToLocalOnly(photoInfo, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "ResetPhotosToLocalOnly fail, ret: %{public}d", ret);
    // Create the new asset record in the database.
    ret = this->CreateNewAssetInfoAndReturnFileId(targetPhotoInfo, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CreateNewAssetInfoAndReturnFileId fail, ret: %{public}d", ret);
    ret = this->CleanLocalFileAndCreateDentryFile(targetPhotoInfo, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CleanLocalFileAndCreateDentryFile fail, ret: %{public}d", ret);
    return E_OK;
}

int32_t MediaAssetsDeleteService::ResetPhotosToLocalOnly(
    const PhotosPo &photoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    return this->mediaAssetsDao_.ClearCloudInfo(photoRefresh, photoInfo.fileId.value_or(-1));
}

int32_t MediaAssetsDeleteService::SetMdirty(PhotosPo &photoInfo)
{
    int32_t ret = E_OK;
    photoInfo.dirty = photoInfo.TryGetMdirty();
    return ret;
}

int32_t MediaAssetsDeleteService::ResetFileSourceType(PhotosPo &photoInfo)
{
    int32_t ret = E_OK;
    photoInfo.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);
    return ret;
}

int32_t MediaAssetsDeleteService::MoveAssetFileOutOfLake(const PhotosPo &photoInfo)
{
    std::string lakePath = photoInfo.storagePath.value_or("");
    std::string mediaPath = photoInfo.data.value_or("");
    int32_t ret = LakeFileOperations::MoveLakeFile(lakePath, mediaPath);
    MEDIA_INFO_LOG(
        "MoveAssetFileOutOfLake. ret: %{public}d, storagePath: %{public}s, data: %{public}s, errno: %{public}d",
        ret,
        DfxUtils::GetSafePath(lakePath).c_str(),
        DfxUtils::GetSafePath(mediaPath).c_str(),
        errno);
    return ret;
}

int32_t MediaAssetsDeleteService::CreateNewAssetInfoAndReturnFileId(
    PhotosPo &photoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    NativeRdb::ValuesBucket valuesBucket;
    this->GetValuesBucket(photoInfo, valuesBucket);
    int64_t targetFileId;
    int32_t ret = this->mediaAssetsDao_.CreateNewAsset(photoRefresh, targetFileId, valuesBucket);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CreateNewAssetInfoAndReturnFileId fail, ret: %{public}d", ret);
    this->SetFileId(photoInfo, static_cast<int32_t>(targetFileId));
    return E_OK;
}

// Copy the LOCAL asset record from LOCAL_AND_CLOUD asset record, and move it into trash.
int32_t MediaAssetsDeleteService::CreateLocalAssetWithLakeFile(const PhotosPo &photoInfo, PhotosPo &targetPhotoInfo,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    int32_t ret = this->CreateLocalTrashedPhotosPo(photoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CreateLocalTrashedPhotosPo fail, ret: %{public}d", ret);
    // identify Media HO Lake asset, and move the original file from lake.
    ret = this->MoveAssetFileOutOfLake(photoInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "MoveAssetFileOutOfLake fail, ret: %{public}d", ret);
    ret = this->MoveLocalAssetFile(photoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CreateLocalAssetFile fail, ret: %{public}d", ret);
    // Create the new asset record in the database.
    return this->CreateNewAssetInfoAndReturnFileId(targetPhotoInfo, photoRefresh);
}

int32_t MediaAssetsDeleteService::CopyAndMoveMediaLocalAssetToTrash(const PhotosPo &photoInfo,
    std::optional<PhotosPo> &targetPhotoInfoOp, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    bool isValid = photoInfo.ShouldHandleAsMediaFile();
    isValid = isValid && photoInfo.dateTrashed.value_or(0) == 0;
    CHECK_AND_RETURN_RET(isValid, E_INVALID_MODE);
    PhotosPo targetPhotoInfo;
    int32_t ret = this->CreateLocalAssetWithFile(photoInfo, targetPhotoInfo, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CreateLocalAsset fail, ret: %{public}d", ret);
    // Reset the storage position of the LOCAL_AND_CLOUD asset record to CLOUD asset record (cloud only).
    ret = this->CleanLocalFileAndCreateDentryFile(photoInfo, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CleanLocalFileAndCreateDentryFile fail, ret: %{public}d", ret);
    MEDIA_INFO_LOG("CopyAndMoveMediaLocalAssetToTrash completed, "
                   "sourceFileId: %{public}d, targetFileId: %{public}d, cloudId: %{public}s",
        photoInfo.fileId.value_or(0),
        targetPhotoInfo.fileId.value_or(0),
        photoInfo.cloudId.value_or("").c_str());
    targetPhotoInfoOp = targetPhotoInfo;
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, ret, "watch is nullptr");
    watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(photoInfo.fileId.value_or(0)), NotifyType::NOTIFY_UPDATE);
    return E_OK;
}

int32_t MediaAssetsDeleteService::CopyAndMoveLakeLocalAssetToTrash(const PhotosPo &photoInfo,
    std::optional<PhotosPo> &targetPhotoInfoOp, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    bool isValid = !photoInfo.ShouldHandleAsMediaFile();
    isValid = isValid && photoInfo.dateTrashed.value_or(0) == 0;
    CHECK_AND_RETURN_RET(isValid, E_INVALID_MODE);
    PhotosPo targetPhotoInfo;
    int32_t ret = this->CreateLocalAssetWithLakeFile(photoInfo, targetPhotoInfo, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CreateLocalAsset fail, ret: %{public}d", ret);
    // Reset the storage position of the LOCAL_AND_CLOUD asset record to CLOUD asset record (cloud only).
    ret = this->mediaAssetsDao_.ResetPositionToCloudOnly(photoRefresh, photoInfo.fileId.value_or(-1));
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "UpdatePosition fail, ret: %{public}d", ret);
    MEDIA_INFO_LOG("CopyAndMoveLakeLocalAssetToTrash completed, "
                   "sourceFileId: %{public}d, targetFileId: %{public}d, cloudId: %{public}s",
        photoInfo.fileId.value_or(0),
        targetPhotoInfo.fileId.value_or(0),
        photoInfo.cloudId.value_or("").c_str());
    targetPhotoInfoOp = targetPhotoInfo;
    return E_OK;
}

int32_t MediaAssetsDeleteService::CreateCloudAssetWithoutDentryFile(const PhotosPo &photoInfo,
    PhotosPo &targetPhotoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    int32_t ret = this->CreateCloudTrashedPhotosPo(photoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CreateCloudTrashedPhotosPo fail, ret: %{public}d", ret);
    ret = this->CreateCloudAssetThumbnail(photoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CreateCloudAssetThumbnail fail, ret: %{public}d", ret);
    // Must Clean the cloud info of original asset firstly, to avoid duplicate cloud info in the database.
    // Clear cloud info of the LOCAL_AND_CLOUD asset record to LOCAL asset record (local only).
    ret = this->ResetPhotosToLocalOnly(photoInfo, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "ResetPhotosToLocalOnly fail, ret: %{public}d", ret);
    // Create the new asset record in the database.
    ret = this->CreateNewAssetInfoAndReturnFileId(targetPhotoInfo, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CreateNewAssetInfoAndReturnFileId fail, ret: %{public}d", ret);
    // In this case, Only Media file need to create dentry file. (No need for Lake file).
    // The asset file in Lake does not move out of the Lake, and does not need to create a dentry file.
    // Otherwise, the original file in Lake will be removed and make the asset record invalid.
    return E_OK;
}

int32_t MediaAssetsDeleteService::CopyAndMoveMediaCloudAssetToTrash(const PhotosPo &photoInfo,
    std::optional<PhotosPo> &targetPhotoInfoOp, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    bool isValid = photoInfo.ShouldHandleAsMediaFile();
    isValid = isValid && photoInfo.dateTrashed.value_or(0) == 0;
    CHECK_AND_RETURN_RET(isValid, E_INVALID_MODE);
    PhotosPo targetPhotoInfo;
    int32_t ret = this->CreateCloudAssetWithDentryFile(photoInfo, targetPhotoInfo, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CreateCloudAssetWithDentryFile fail, ret: %{public}d", ret);
    MEDIA_INFO_LOG("CopyAndMoveMediaCloudAssetToTrash completed, "
                   "sourceFileId: %{public}d, targetFileId: %{public}d, cloudId: %{public}s",
        photoInfo.fileId.value_or(0),
        targetPhotoInfo.fileId.value_or(0),
        photoInfo.cloudId.value_or("").c_str());
    targetPhotoInfoOp = targetPhotoInfo;
    return E_OK;
}

int32_t MediaAssetsDeleteService::CopyAndMoveLakeCloudAssetToTrash(const PhotosPo &photoInfo,
    std::optional<PhotosPo> &targetPhotoInfoOp, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    bool isValid = !photoInfo.ShouldHandleAsMediaFile();
    isValid = isValid && photoInfo.dateTrashed.value_or(0) == 0;
    CHECK_AND_RETURN_RET(isValid, E_INVALID_MODE);
    PhotosPo targetPhotoInfo;
    int32_t ret = this->CreateCloudAssetWithoutDentryFile(photoInfo, targetPhotoInfo, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CreateCloudAssetWithoutDentryFile fail, ret: %{public}d", ret);
    MEDIA_INFO_LOG("CopyAndMoveLakeCloudAssetToTrash completed, "
                   "sourceFileId: %{public}d, targetFileId: %{public}d, cloudId: %{public}s",
        photoInfo.fileId.value_or(0),
        targetPhotoInfo.fileId.value_or(0),
        photoInfo.cloudId.value_or("").c_str());
    targetPhotoInfoOp = targetPhotoInfo;
    return E_OK;
}

int32_t MediaAssetsDeleteService::BuildMediaFilePath(const PhotosPo &photoInfo, std::string &targetPath)
{
    // check arguements
    std::string displayName = photoInfo.displayName.value_or("");
    int32_t mediaType = photoInfo.mediaType.value_or(0);
    CHECK_AND_RETURN_RET_LOG(!displayName.empty(), E_FILE_NAME_INVALID, "displayName is empty");
    MediaLibraryAlbumFusionUtils::BuildTargetFilePath(targetPath, displayName, mediaType);
    CHECK_AND_RETURN_RET_LOG(!targetPath.empty(),
        E_FILE_NAME_INVALID,
        "Build target path fail, photoInfo: %{public}s",
        photoInfo.ToString().c_str());
    return E_OK;
}

int32_t MediaAssetsDeleteService::BuildLakeFilePath(const PhotosPo &photoInfo, std::string &targetPath)
{
    // check arguements
    std::string displayName = photoInfo.displayName.value_or("");
    int32_t mediaType = photoInfo.mediaType.value_or(0);
    CHECK_AND_RETURN_RET_LOG(!displayName.empty(), E_FILE_NAME_INVALID, "displayName is empty");
    return LakeFileUtils::BuildLakeFilePath(displayName, mediaType, targetPath);
}

int32_t MediaAssetsDeleteService::GetValuesBucket(const PhotosPo &photoInfo, NativeRdb::ValuesBucket &valuesBucket)
{
    PhotosPo tempPhotoInfo = photoInfo;
    PhotosPoWriter writer = PhotosPoWriter(tempPhotoInfo);
    std::unordered_map<std::string, std::string> valueBucketMap = writer.ToMap(false);
    std::stringstream ss;
    const std::vector<SetValHandle> setValFuncs = {
        &MediaAssetsDeleteService::SetValue,
        &MediaAssetsDeleteService::SetNull4EmptyColumn,
        &MediaAssetsDeleteService::SetNull4MissColumn,
    };
    int32_t ret = E_OK;
    int32_t index = 1;
    for (auto setValFunc : setValFuncs) {  // Loop to set value.
        ret = (this->*setValFunc)(valuesBucket, valueBucketMap, ss);
        ss << "fun: " << index++ << ", ret: " << ret << ", ";
    }
    MEDIA_DEBUG_LOG("GetValuesBucket, %{public}s", ss.str().c_str());
    return E_OK;
}

int32_t MediaAssetsDeleteService::SetValue(NativeRdb::ValuesBucket &valuesBucket,
    const std::unordered_map<std::string, std::string> &valueBucketMap, std::stringstream &ss)
{
    for (const auto &pair : valueBucketMap) {
        valuesBucket.Put(pair.first, pair.second);
        ss << pair.first << " : " << pair.second << ", ";
    }
    return E_OK;
}

int32_t MediaAssetsDeleteService::SetNull4EmptyColumn(NativeRdb::ValuesBucket &valuesBucket,
    const std::unordered_map<std::string, std::string> &valueBucketMap, std::stringstream &ss)
{
    std::vector<std::string> nullableColumns = {
        PhotoColumn::PHOTO_STORAGE_PATH,
        MediaColumn::MEDIA_DEVICE_NAME,
        PhotoColumn::PHOTO_USER_COMMENT,
    };
    bool isValid = false;
    for (const auto &columnName : nullableColumns) {
        auto it = valueBucketMap.find(columnName);
        isValid = it != valueBucketMap.end();
        isValid = isValid && it->second.empty();
        CHECK_AND_CONTINUE(isValid);  // Continue set NULL if empty.
        valuesBucket.Delete(columnName);
        valuesBucket.PutNull(columnName);
        ss << columnName << ": reset to null, ";
    }
    return E_OK;
}

int32_t MediaAssetsDeleteService::SetNull4MissColumn(NativeRdb::ValuesBucket &valuesBucket,
    const std::unordered_map<std::string, std::string> &valueBucketMap, std::stringstream &ss)
{
    std::set<std::string> nullableColumns = {
        PhotoColumn::PHOTO_LATITUDE,
        PhotoColumn::PHOTO_LONGITUDE,
    };
    bool isValid = false;
    for (const auto &columnName : nullableColumns) {
        isValid = valueBucketMap.find(columnName) == valueBucketMap.end();
        CHECK_AND_CONTINUE(isValid);  // Continue set NULL if not exist
        valuesBucket.PutNull(columnName);
        ss << columnName << ": null, ";
    }
    return E_OK;
}

int32_t MediaAssetsDeleteService::FindBurstAssetsAndResetBurstKey(
    const std::string &originalBurstKey, std::optional<PhotosPo> &coverAsset, std::vector<PhotosPo> &burstAssets)
{
    int32_t ret = this->mediaAssetsDao_.FindAssetsByBurstKey(originalBurstKey, burstAssets);
    bool isValid = ret == E_OK && !burstAssets.empty();
    CHECK_AND_RETURN_RET(isValid, E_INVAL_ARG);
    // Generate new burst_key.
    const std::string newBurstKey = LakeFileUtils::GenerateUuid();
    std::for_each(burstAssets.begin(), burstAssets.end(), [&](auto &element) { element.burstKey = newBurstKey; });
    auto it = std::find_if(burstAssets.begin(), burstAssets.end(), [](const PhotosPo &element) {
        return element.burstCoverLevel.value_or(static_cast<int32_t>(BurstCoverLevelType::COVER)) ==
            static_cast<int32_t>(BurstCoverLevelType::COVER);
    });
    CHECK_AND_EXECUTE(it == burstAssets.end(), coverAsset = *it);
    return E_OK;
}

/**
 * Caller should distinguish following scenario:
 * Scenario 1, Should handle as Burst Assets.
 *   Check: E_OK & !burstAssets.empty()
 * Scenario 2, Continuous to handle by responsibility-chain.
 */
int32_t MediaAssetsDeleteService::CheckAndFindBurstAssets(
    const PhotosPo &photoInfo, std::optional<PhotosPo> &coverAssetOp, std::vector<PhotosPo> &burstAssets)
{
    bool isValid = photoInfo.dateTrashed.value_or(0) == 0;
    const std::string originalBurstKey = photoInfo.burstKey.value_or("");
    isValid = isValid && !originalBurstKey.empty();
    CHECK_AND_RETURN_RET(isValid, E_INVALID_MODE);
    // Find burst assets by burst_key.
    return this->FindBurstAssetsAndResetBurstKey(originalBurstKey, coverAssetOp, burstAssets);
}

int32_t MediaAssetsDeleteService::DeleteLocalAssetSingle(const PhotosPo &photoInfo,
    std::optional<PhotosPo> &targetPhotoInfoOp, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    CHECK_AND_RETURN_RET_LOG(
        photoRefresh != nullptr, E_RDB_STORE_NULL, "Failed to get photoRefresh.");
    bool isValid = photoInfo.position.value_or(1) == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    CHECK_AND_RETURN_RET_WARN_LOG(isValid,
        E_OK,
        "No need to handle, not LOCAL_AND_CLOUD. fileId: %{public}d, position: %{public}d, cloudId: %{public}s",
        photoInfo.fileId.value_or(-1),
        photoInfo.position.value_or(-1),
        photoInfo.cloudId.value_or("").c_str());
    const std::vector<DeleteFuncHandle> deleteFuncs = {
        &MediaAssetsDeleteService::DeleteLocalBurstAssets,
        &MediaAssetsDeleteService::CopyAndMoveLocalAssetToTrash,
    };
    int32_t ret = E_INVALID_MODE;
    for (auto deleteFunc : deleteFuncs) {  // Chain of responsibility.
        ret = (this->*(deleteFunc))(photoInfo, targetPhotoInfoOp, photoRefresh);
        CHECK_AND_BREAK(ret != E_OK);
    }
    return E_OK;
}

/**
 * The group (same burstKey) of photoInfo will be deleteLocal into trash.
 * @param photoInfo deleteLocal for this object.
 * @param targetPhotoInfoOp the new local trashed object of photoInfo.
 */
int32_t MediaAssetsDeleteService::DeleteLocalBurstAssets(const PhotosPo &photoInfo,
    std::optional<PhotosPo> &targetPhotoInfoOp, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    std::optional<PhotosPo> coverAssetOp;
    std::vector<PhotosPo> burstAssets;
    int32_t ret = this->CheckAndFindBurstAssets(photoInfo, coverAssetOp, burstAssets);
    CHECK_AND_RETURN_RET(ret == E_OK, ret); // Not burst assets, continuous in responsibility-chain.
    const bool hasCoverAsset = coverAssetOp.has_value();
    const int32_t coverPosition = hasCoverAsset ? coverAssetOp.value().position.value_or(1) : 1;
    const int32_t coverFileId = hasCoverAsset ? coverAssetOp.value().fileId.value_or(0) : 0;
    const std::string coverCloudId = hasCoverAsset ? coverAssetOp.value().cloudId.value_or("") : "";
    bool isValid = hasCoverAsset;
    isValid = isValid && coverPosition == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    CHECK_AND_RETURN_RET_WARN_LOG(isValid,
        E_OK, // end of responsibility-chain.
        "No need to handle, cover is not LOCAL_AND_CLOUD. "
        "fileId: %{public}d, position: %{public}d, cloudId: %{public}s, burstKey: %{public}s, "
        "hasCoverAsset: %{public}d, coverPosition: %{public}d, coverFileId: %{public}d, coverCloudId: %{public}s",
        photoInfo.fileId.value_or(-1),
        photoInfo.position.value_or(-1),
        photoInfo.cloudId.value_or("").c_str(),
        photoInfo.burstKey.value_or("").c_str(),
        hasCoverAsset, coverPosition, coverFileId, coverCloudId.c_str());
    std::optional<PhotosPo> tempPhotoInfoOp;
    int32_t opCount = 0;
    bool isCurrAsset = false;
    for (const auto &element : burstAssets) {
        ret = this->CopyAndMoveLocalAssetToTrash(element, tempPhotoInfoOp, photoRefresh);
        isValid = (ret == E_OK) && tempPhotoInfoOp.has_value();
        CHECK_AND_EXECUTE(!isValid, opCount++);
        isCurrAsset = photoInfo.fileId.value_or(0) == element.fileId.value_or(0);
        CHECK_AND_EXECUTE(!(isValid && isCurrAsset), targetPhotoInfoOp = tempPhotoInfoOp); // new asset.
    }
    const bool hasNewAsset = targetPhotoInfoOp.has_value();
    const int32_t targetFileId = hasNewAsset ? targetPhotoInfoOp.value().fileId.value_or(0) : 0;
    const std::string targetBurstKey = hasNewAsset ? targetPhotoInfoOp.value().burstKey.value_or("") : "";
    MEDIA_INFO_LOG(
        "DeleteLocalBurstAssets completed, "
        "fileId: %{public}d, cloudId: %{public}s, level: %{public}d, burstKey: %{public}s, group-size: %{public}zu, "
        "hasNewAsset: %{public}d, targetFileId: %{public}d, targetBurstKey: %{public}s, opCount: %{public}d",
        photoInfo.fileId.value_or(-1),
        photoInfo.cloudId.value_or("").c_str(),
        photoInfo.burstCoverLevel.value_or(1),
        photoInfo.burstKey.value_or("").c_str(),
        burstAssets.size(),
        hasNewAsset, targetFileId, targetBurstKey.c_str(), opCount);
    return E_OK; // end of responsibility-chain.
}

int32_t MediaAssetsDeleteService::DeleteCloudBurstAssets(const PhotosPo &photoInfo,
    std::optional<PhotosPo> &targetPhotoInfoOp, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    std::optional<PhotosPo> coverAssetOp;
    std::vector<PhotosPo> burstAssets;
    int32_t ret = this->CheckAndFindBurstAssets(photoInfo, coverAssetOp, burstAssets);
    CHECK_AND_RETURN_RET(ret == E_OK, ret); // Not burst assets, continuous in responsibility-chain.
    const bool hasCoverAsset = coverAssetOp.has_value();
    const int32_t coverPosition = hasCoverAsset ? coverAssetOp.value().position.value_or(1) : 1;
    const int32_t coverFileId = hasCoverAsset ? coverAssetOp.value().fileId.value_or(0) : 0;
    const std::string coverCloudId = hasCoverAsset ? coverAssetOp.value().cloudId.value_or("") : "";
    bool isValid = hasCoverAsset;
    isValid = isValid && coverPosition == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    CHECK_AND_RETURN_RET_WARN_LOG(isValid,
        E_OK, // end of responsibility-chain.
        "No need to handle, cover is not LOCAL_AND_CLOUD. "
        "fileId: %{public}d, position: %{public}d, cloudId: %{public}s, burstKey: %{public}s, "
        "hasCoverAsset: %{public}d, coverPosition: %{public}d, coverFileId: %{public}d, coverCloudId: %{public}s",
        photoInfo.fileId.value_or(-1),
        photoInfo.position.value_or(-1),
        photoInfo.cloudId.value_or("").c_str(),
        photoInfo.burstKey.value_or("").c_str(),
        hasCoverAsset, coverPosition, coverFileId, coverCloudId.c_str());
    std::optional<PhotosPo> tempPhotoInfoOp;
    int32_t opCount = 0;
    bool isCurrAsset = false;
    for (const auto &element : burstAssets) {
        ret = this->CopyAndMoveCloudAssetToTrash(element, tempPhotoInfoOp, photoRefresh);
        isValid = (ret == E_OK) && tempPhotoInfoOp.has_value();
        CHECK_AND_EXECUTE(!isValid, opCount++);
        isCurrAsset = photoInfo.fileId.value_or(0) == element.fileId.value_or(0);
        CHECK_AND_EXECUTE(!(isValid && isCurrAsset), targetPhotoInfoOp = tempPhotoInfoOp); // new asset.
    }
    const bool hasNewAsset = targetPhotoInfoOp.has_value();
    const int32_t targetFileId = hasNewAsset ? targetPhotoInfoOp.value().fileId.value_or(0) : 0;
    const std::string targetBurstKey = hasNewAsset ? targetPhotoInfoOp.value().burstKey.value_or("") : "";
    MEDIA_INFO_LOG(
        "DeleteCloudBurstAssets completed, "
        "fileId: %{public}d, cloudId: %{public}s, level: %{public}d, burstKey: %{public}s, group-size: %{public}zu, "
        "hasNewAsset: %{public}d, targetFileId: %{public}d, targetBurstKey: %{public}s, opCount: %{public}d",
        photoInfo.fileId.value_or(-1),
        photoInfo.cloudId.value_or("").c_str(),
        photoInfo.burstCoverLevel.value_or(1),
        photoInfo.burstKey.value_or("").c_str(),
        burstAssets.size(),
        hasNewAsset, targetFileId, targetBurstKey.c_str(), opCount);
    return E_OK; // end of responsibility-chain.
}

int32_t MediaAssetsDeleteService::DeleteCloudAssetSingle(const PhotosPo &photoInfo,
    std::optional<PhotosPo> &targetPhotoInfoOp, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    CHECK_AND_RETURN_RET_LOG(
        photoRefresh != nullptr, E_RDB_STORE_NULL, "Failed to get photoRefresh.");
    bool isValid = photoInfo.position.value_or(1) == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    CHECK_AND_RETURN_RET_WARN_LOG(isValid,
        E_OK,
        "No need to handle, not LOCAL_AND_CLOUD. fileId: %{public}d, position: %{public}d, cloudId: %{public}s",
        photoInfo.fileId.value_or(-1),
        photoInfo.position.value_or(-1),
        photoInfo.cloudId.value_or("").c_str());
    const std::vector<DeleteFuncHandle> deleteFuncs = {
        &MediaAssetsDeleteService::DeleteCloudBurstAssets,
        &MediaAssetsDeleteService::CopyAndMoveCloudAssetToTrash,
    };
    int32_t ret = E_INVALID_MODE;
    for (auto deleteFunc : deleteFuncs) {  // Chain of responsibility.
        ret = (this->*(deleteFunc))(photoInfo, targetPhotoInfoOp, photoRefresh);
        CHECK_AND_BREAK(ret != E_OK);
    }
    return E_OK;
}
}  // namespace OHOS::Media::Common