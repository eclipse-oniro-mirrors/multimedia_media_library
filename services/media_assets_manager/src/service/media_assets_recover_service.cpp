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

#include "media_assets_recover_service.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

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
#include "medialibrary_album_operations.h"
#include "medialibrary_photo_operations.h"
#include "file_manager_asset_operations.h"
#include "media_library_error_code.h"
#include "burst_dao.h"

namespace OHOS::Media::Common {
int32_t MediaAssetsRecoverService::BatchMoveOutTrashAndMergeWithSameAsset(
    const std::vector<std::string> &assetIds, std::vector<std::string> &targetFileIds,
    std::vector<AssetUriResult> &results)
{
    std::vector<std::string> fileIds;
    std::transform(
        std::begin(assetIds), std::end(assetIds), std::back_inserter(fileIds), [](const std::string &assetId) {
            return MediaAssetsUtils::GetFileId(assetId);
        });
    // The results are aligned with the input assets, so the original fileIds must be kept.
    const std::vector<std::string> inputFileIds = fileIds;
    // Complete the burst members, so that the whole burst group can be recovered by the cover only.
    int32_t ret = this->PrepareForBurstRecovery(fileIds);
    CHECK_AND_PRINT_LOG(ret == E_OK, "PrepareForBurstRecovery failed, ret: %{public}d", ret);
    MediaLibraryAlbumOperations::DealwithNoAlbumAssets(fileIds);
    // Find assets info.
    std::vector<PhotosPo> photosList;
    ret = this->mediaAssetsDao_.QueryAssets(fileIds, photosList);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("QueryAssets fail, ret: %{public}d, fileIds: %{public}zu", ret, fileIds.size());
        this->FillRecoverResults(inputFileIds, results);
        return ret;
    }
    std::optional<PhotosPo> targetPhotoInfoOp;
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> photoRefresh =
        std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    CHECK_AND_EXECUTE(photoRefresh != nullptr, this->FillRecoverResults(inputFileIds, results));
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL,
        "BatchMoveOutTrashAndMergeWithSameAsset Failed to get photoRefresh.");
    std::sort(photosList.begin(), photosList.end(), [](const PhotosPo &a, const PhotosPo &b) {
        return a.fileId.value_or(-1) < b.fileId.value_or(-1);
    });
    // The fileId of the input asset to the uri of the asset which is moved out of the trash successfully.
    std::unordered_map<std::string, std::string> recoveredUris;
    for (const PhotosPo &photoInfo : photosList) {
        // Only the assets in the trash can be recovered.
        CHECK_AND_CONTINUE_INFO_LOG(photoInfo.dateTrashed.value_or(0) > 0,
            "Skip the asset which is not in the trash, fileId: %{public}d", photoInfo.fileId.value_or(-1));
        targetPhotoInfoOp.reset();
        ret = this->MoveOutTrashAndMergeWithSameAsset(photoRefresh, photoInfo, targetPhotoInfoOp);
        ret = this->RecordRecoveredAsset(photoInfo, ret, targetPhotoInfoOp, targetFileIds, recoveredUris);
        CHECK_AND_PRINT_LOG(ret == E_OK, "Failed to recover the asset, ret: %{public}d, fileId: %{public}d",
            ret, photoInfo.fileId.value_or(-1));
    }
    this->FillRecoverResults(inputFileIds, recoveredUris, results);
    photoRefresh->RefreshAlbumNoDateModified(static_cast<NotifyAlbumType>(
        NotifyAlbumType::SYS_ALBUM | NotifyAlbumType::USER_ALBUM | NotifyAlbumType::SOURCE_ALBUM));
    photoRefresh->Notify();
    return E_OK;
}

int32_t MediaAssetsRecoverService::MoveOutTrashAndMergeWithSameAsset(
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh, const PhotosPo &photoInfo,
    std::optional<PhotosPo> &targetPhotoInfoOp)
{
    CHECK_AND_RETURN_RET_LOG(
        photoRefresh != nullptr, E_RDB_STORE_NULL, "MoveOutTrashAndMergeWithSameAsset Failed to get photoRefresh.");
    targetPhotoInfoOp = photoInfo;  // By default, move the asset out of the trash.
    // Ensure the process is single-threaded.
    std::lock_guard<std::mutex> lock(this->moveOutTrashMutex_);
    // Find the same asset in the PhotoAlbum, which has the same display_name, size, orientation(picture only).
    std::optional<PhotosPo> samePhotoInfoOp;
    int32_t ret = this->mediaAssetsDao_.FindSamePhoto(photoInfo, samePhotoInfoOp);
    bool hasFoundSameAsset = ret == E_OK && samePhotoInfoOp.has_value();
    hasFoundSameAsset =
        hasFoundSameAsset && samePhotoInfoOp.value().fileId.value_or(-1) != photoInfo.fileId.value_or(-1);  // itself
    if (!hasFoundSameAsset) {
        // Move the asset out of the trash.
        ret = this->RecoverPhotoAsset(photoInfo.BuildFileUri());
        MEDIA_INFO_LOG("MoveOutTrash completed, Same photo not found, ret: %{public}d, fileId: %{public}d",
            ret,
            photoInfo.fileId.value_or(-1));
        return ret;
    }
    // Merge the same asset.
    ret = this->MergeSameAssets(photoInfo, samePhotoInfoOp.value(), photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "MergeSameAssets fail, ret: %{public}d", ret);
    targetPhotoInfoOp = samePhotoInfoOp;  // return the merged object
    return E_OK;
}

int32_t MediaAssetsRecoverService::RecoverPackageName(const PhotosPo &sourcePhotoInfo, const PhotosPo &targetPhotoInfo,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    MEDIA_WARN_LOG("RecoverPackageName");
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL,
        "RecoverPackageName Failed to get photoRefresh.");
    if (sourcePhotoInfo.packageName.value_or("").empty() || !targetPhotoInfo.packageName.value_or("").empty()) {
        return E_OK;
    }
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, targetPhotoInfo.fileId.value_or(0));
    NativeRdb::ValuesBucket values;
    values.PutString(PhotoColumn::MEDIA_PACKAGE_NAME, sourcePhotoInfo.packageName.value_or(""));
    int32_t changedRows = -1;
    int32_t ret = photoRefresh->Update(changedRows, values, predicates);
    MEDIA_INFO_LOG("RecoverPackageName Completed, "
                   "ret: %{public}d, ChangedRows: %{public}d, cloudId: %{public}s",
        ret,
        changedRows,
        targetPhotoInfo.cloudId.value_or("").c_str());
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to RecoverPackageName.");
    CHECK_AND_RETURN_RET_WARN_LOG(
        changedRows > 0, ret, "RecoverPackageName Check updateRows: %{public}d.", changedRows);
    return E_OK;
}

int32_t MediaAssetsRecoverService::MergeSameAssets(const PhotosPo &sourcePhotoInfo, const PhotosPo &targetPhotoInfo,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    const std::vector<MergeFuncHandle> mergeFuncs = {
        &MediaAssetsRecoverService::MergeSameAssetOfMediaAndMedia,
        &MediaAssetsRecoverService::MergeSameAssetOfMediaAndLake,
        // No MergeSameAssetOfLakeAndLake,
        // No MergeSameAssetOfLakeAndMedia,
    };
    int32_t ret = E_INVALID_MODE;
    for (auto mergeFunc : mergeFuncs) {  // Chain of responsibility.
        ret = (this->*(mergeFunc))(sourcePhotoInfo, targetPhotoInfo, photoRefresh);
        CHECK_AND_BREAK(ret != E_OK);
    }
    this->RecoverPackageName(sourcePhotoInfo, targetPhotoInfo, photoRefresh);
    MEDIA_INFO_LOG("MergeSameAssets completed, ret: %{public}d", ret);
    return ret;
}

int32_t MediaAssetsRecoverService::MergeSameAssetOfMediaAndMedia(const PhotosPo &sourcePhotoInfo,
    const PhotosPo &targetPhotoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    bool isValidAssetType = sourcePhotoInfo.ShouldHandleAsMediaFile() && !targetPhotoInfo.ShouldHandleAsLakeFile();
    CHECK_AND_RETURN_RET(isValidAssetType, E_INVALID_MODE);
    const std::vector<MergeFuncHandle> mergeFuncs = {
        &MediaAssetsRecoverService::CommonMergeDiffCloudAsset,
        &MediaAssetsRecoverService::CommonMergeSameCloudAsset,
        &MediaAssetsRecoverService::CommonMergeCloudToLocalAsset,
        &MediaAssetsRecoverService::CommonMergeLocalToLocalAsset,
        &MediaAssetsRecoverService::MediaAndMediaMergeLocalToCloudAsset,
    };
    int32_t ret = E_INVALID_MODE;
    for (auto mergeFunc : mergeFuncs) {  // Chain of responsibility.
        ret = (this->*(mergeFunc))(sourcePhotoInfo, targetPhotoInfo, photoRefresh);
        CHECK_AND_BREAK(ret != E_OK);
    }
    MEDIA_INFO_LOG("MergeSameAssetOfMediaAndMedia completed, ret: %{public}d", ret);
    return ret;
}

int32_t MediaAssetsRecoverService::MergeSameAssetOfMediaAndLake(const PhotosPo &sourcePhotoInfo,
    const PhotosPo &targetPhotoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    bool isValidAssetType = sourcePhotoInfo.ShouldHandleAsMediaFile() && targetPhotoInfo.ShouldHandleAsLakeFile();
    CHECK_AND_RETURN_RET(isValidAssetType, E_INVALID_MODE);
    const std::vector<MergeFuncHandle> mergeFuncs = {
        &MediaAssetsRecoverService::CommonMergeDiffCloudAsset,
        &MediaAssetsRecoverService::CommonMergeSameCloudAsset,
        &MediaAssetsRecoverService::CommonMergeCloudToLocalAsset,
        &MediaAssetsRecoverService::CommonMergeLocalToLocalAsset,
        &MediaAssetsRecoverService::MediaAndLakeMergeLocalToCloudAsset,
        &MediaAssetsRecoverService::MediaAndLakeMergeLocalToHiddenCloudAsset,
    };
    int32_t ret = E_INVALID_MODE;
    for (auto mergeFunc : mergeFuncs) {  // Chain of responsibility.
        ret = (this->*(mergeFunc))(sourcePhotoInfo, targetPhotoInfo, photoRefresh);
        CHECK_AND_BREAK(ret != E_OK);
    }
    MEDIA_INFO_LOG("MergeSameAssetOfMediaAndLake completed, ret: %{public}d", ret);
    return ret;
}

int32_t MediaAssetsRecoverService::MergeAssetFile(const PhotosPo &photoInfo, const PhotosPo &targetPhotoInfo)
{
    // Only the LOCAL and LOCAL_AND_CLOUD need to move the original files.
    bool isValid = photoInfo.position.value_or(1) != static_cast<int32_t>(PhotoPositionType::CLOUD);
    CHECK_AND_RETURN_RET_INFO_LOG(isValid, E_OK, "No need to move file, source is CLOUD asset.");
    // Only the CLOUD Asset need the original files.
    isValid = targetPhotoInfo.position.value_or(1) == static_cast<int32_t>(PhotoPositionType::CLOUD);
    CHECK_AND_RETURN_RET_INFO_LOG(isValid, E_OK, "No need to move file, target is LOCAL OR LOCAL_AND_CLOUD asset.");
    PhotoFileOperation fileOperation;
    int32_t ret = fileOperation.DeletePhoto(targetPhotoInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "DeletePhoto fail, ret: %{public}d", ret);
    ret = fileOperation.MovePhoto(photoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CopyPhoto fail, ret: %{public}d", ret);
    this->StoreThumbnailAndEditSize(targetPhotoInfo);
    MEDIA_INFO_LOG(
        "MergeAssetFile completed, "
        "sourceFileId: %{public}d, sourcePosition: %{public}d, targetFileId: %{public}d, targetPosition: %{public}d, "
        "auditLog: %{public}s",
        photoInfo.fileId.value_or(0),
        photoInfo.position.value_or(0),
        targetPhotoInfo.fileId.value_or(0),
        targetPhotoInfo.position.value_or(0),
        fileOperation.GetAuditLog().c_str());
    return ret;
}

int32_t MediaAssetsRecoverService::RemoveAssetAndFile(
    const PhotosPo &photoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    int32_t ret = this->mediaAssetsDao_.DeletePhotoInfo(photoRefresh, photoInfo.fileId.value_or(-1));
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "DeletePhotoInfo fail, ret: %{public}d", ret);
    // Remove the file from the file system.
    PhotoFileOperation fileOperation;
    ret = fileOperation.DeletePhoto(photoInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "DeletePhoto fail, ret: %{public}d", ret);
    ret = fileOperation.DeleteThumbnail(photoInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "DeleteThumbnail fail, ret: %{public}d", ret);
    MEDIA_INFO_LOG("RemoveAssetAndFile completed, "
                   "fileId: %{public}d, position: %{public}d, auditLog: %{public}s",
        photoInfo.fileId.value_or(0),
        photoInfo.position.value_or(0),
        fileOperation.GetAuditLog().c_str());
    return E_OK;
}

int32_t MediaAssetsRecoverService::MoveAssetFileFromMediaToLake(
    const PhotosPo &photoInfo, const PhotosPo &targetPhotoInfo)
{
    std::string mediaPath = photoInfo.data.value_or("");
    std::string lakePath = targetPhotoInfo.storagePath.value_or("");
    if (targetPhotoInfo.ShouldHandleAsMediaFile()) {
        return MoveAssetFileFromMediaToFileManager(photoInfo, targetPhotoInfo);
    }
    bool isValid = photoInfo.ShouldHandleAsMediaFile();
    isValid = isValid && !lakePath.empty();
    CHECK_AND_RETURN_RET_LOG(isValid, E_OK, "No need to move file, target is not in lake.");
    isValid = isValid && !MediaFileUtils::IsFileExists(lakePath);
    CHECK_AND_RETURN_RET_LOG(isValid, E_OK, "No need to move file, target file already exists.");
    int32_t ret = LakeFileOperations::MoveLakeFile(mediaPath, lakePath);
    MEDIA_INFO_LOG(
        "MoveAssetFileFromMediaToLake. ret: %{public}d, storagePath: %{public}s, data: %{public}s, errno: %{public}d",
        ret,
        DfxUtils::GetSafePath(lakePath).c_str(),
        DfxUtils::GetSafePath(mediaPath).c_str(),
        errno);
    return ret;
}

int32_t MediaAssetsRecoverService::MergeAssetFileFromMediaToLake(
    const PhotosPo &photoInfo, const PhotosPo &targetPhotoInfo)
{
    // Only the LOCAL and LOCAL_AND_CLOUD need to move the original files.
    bool isValid = photoInfo.position.value_or(1) != static_cast<int32_t>(PhotoPositionType::CLOUD);
    CHECK_AND_RETURN_RET_INFO_LOG(isValid, E_OK, "No need to move file, source is CLOUD asset.");
    // Only the CLOUD Asset need the original files.
    isValid = targetPhotoInfo.position.value_or(1) == static_cast<int32_t>(PhotoPositionType::CLOUD);
    CHECK_AND_RETURN_RET_INFO_LOG(isValid, E_OK, "No need to move file, target is LOCAL OR LOCAL_AND_CLOUD asset.");
    int32_t ret = this->MoveAssetFileFromMediaToLake(photoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "MoveAssetFileFromMediaToLake fail, ret: %{public}d", ret);
    MEDIA_INFO_LOG(
        "MergeAssetFileFromMediaToLake completed, "
        "sourceFileId: %{public}d, sourcePosition: %{public}d, targetFileId: %{public}d, targetPosition: %{public}d",
        photoInfo.fileId.value_or(0),
        photoInfo.position.value_or(0),
        targetPhotoInfo.fileId.value_or(0),
        targetPhotoInfo.position.value_or(0));
    return ret;
}

int32_t MediaAssetsRecoverService::CommonMergeDiffCloudAsset(const PhotosPo &sourcePhotoInfo,
    const PhotosPo &targetPhotoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    bool isDiffCloudAsset = sourcePhotoInfo.cloudId.value_or("") != targetPhotoInfo.cloudId.value_or("");
    bool isValid = isDiffCloudAsset && sourcePhotoInfo.IsCloudAsset() && targetPhotoInfo.IsCloudAsset();
    isValid = isValid && sourcePhotoInfo.dateTrashed.value_or(0) > 0;  // The source is in the trash.
    isValid = isValid && this->mediaAssetsDao_.IsSameAssetIgnoreAlbum(sourcePhotoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET(isValid, E_INVALID_MODE);
    int32_t ret = this->mediaAssetsDao_.LogicalDeleteCloudTrashedPhoto(sourcePhotoInfo, photoRefresh);
    MEDIA_INFO_LOG("CommonMergeDiffCloudAsset completed, ret: %{public}d", ret);
    return ret;
}

int32_t MediaAssetsRecoverService::CommonMergeSameCloudAsset(const PhotosPo &sourcePhotoInfo,
    const PhotosPo &targetPhotoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    bool isSameCloudAsset = sourcePhotoInfo.cloudId.value_or("") == targetPhotoInfo.cloudId.value_or("");
    bool isValid = isSameCloudAsset && sourcePhotoInfo.IsCloudAsset() && targetPhotoInfo.IsCloudAsset();
    isValid = isValid && sourcePhotoInfo.dateTrashed.value_or(0) > 0;  // The source is in the trash.
    isValid = isValid && this->mediaAssetsDao_.IsSameAssetIgnoreAlbum(sourcePhotoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET(isValid, E_INVALID_MODE);
    // Merge the cloud info of the same cloud asset.
    int32_t ret = this->RemoveAssetAndFile(sourcePhotoInfo, photoRefresh);
    MEDIA_INFO_LOG("CommonMergeSameCloudAsset completed, ret: %{public}d", ret);
    return ret;
}

int32_t MediaAssetsRecoverService::CommonMergeCloudToLocalAsset(const PhotosPo &sourcePhotoInfo,
    const PhotosPo &targetPhotoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    bool isValid = sourcePhotoInfo.IsCloudAsset() && !targetPhotoInfo.IsCloudAsset();
    isValid = isValid && sourcePhotoInfo.dateTrashed.value_or(0) > 0;  // The source is in the trash.
    isValid = isValid && this->mediaAssetsDao_.IsSameAssetIgnoreAlbum(sourcePhotoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET(isValid, E_INVALID_MODE);
    int32_t ret = this->RemoveAssetAndFile(sourcePhotoInfo, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "RemoveAssetAndFile fail, ret: %{public}d", ret);
    ret = this->mediaAssetsDao_.MergeCloudInfoIntoTargetPhoto(sourcePhotoInfo, targetPhotoInfo, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "MergeCloudInfoIntoTargetPhoto fail, ret: %{public}d", ret);
    MEDIA_INFO_LOG(
        "CommonMergeCloudToLocalAsset completed, "
        "sourceFileId: %{public}d, sourcePosition: %{public}d, targetFileId: %{public}d, targetPosition: %{public}d",
        sourcePhotoInfo.fileId.value_or(0),
        sourcePhotoInfo.position.value_or(0),
        targetPhotoInfo.fileId.value_or(0),
        targetPhotoInfo.position.value_or(0));
    return E_OK;
}

int32_t MediaAssetsRecoverService::MediaAndMediaMergeLocalToCloudAsset(const PhotosPo &sourcePhotoInfo,
    const PhotosPo &targetPhotoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    bool isValid = !sourcePhotoInfo.IsCloudAsset() && targetPhotoInfo.IsCloudAsset();
    isValid = isValid && sourcePhotoInfo.dateTrashed.value_or(0) > 0;  // The source is in the trash.
    isValid = isValid && this->mediaAssetsDao_.IsSameAssetIgnoreAlbum(sourcePhotoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET(isValid, E_INVALID_MODE);
    // Move the origianl files to target asset.
    int32_t ret = this->MergeAssetFile(sourcePhotoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "MergeAssetFile fail, ret: %{public}d", ret);
    ret = this->RemoveAssetAndFile(sourcePhotoInfo, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "RemoveAssetAndFile fail, ret: %{public}d", ret);
    ret = this->mediaAssetsDao_.UpdatePositionToBoth(sourcePhotoInfo, targetPhotoInfo, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "UpdatePositionToBoth fail, ret: %{public}d", ret);
    MEDIA_INFO_LOG(
        "MediaAndMediaMergeLocalToCloudAsset completed, "
        "sourceFileId: %{public}d, sourcePosition: %{public}d, targetFileId: %{public}d, targetPosition: %{public}d",
        sourcePhotoInfo.fileId.value_or(0),
        sourcePhotoInfo.position.value_or(0),
        targetPhotoInfo.fileId.value_or(0),
        targetPhotoInfo.position.value_or(0));
    return E_OK;
}

int32_t MediaAssetsRecoverService::CommonMergeLocalToLocalAsset(const PhotosPo &sourcePhotoInfo,
    const PhotosPo &targetPhotoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    bool isValid = !sourcePhotoInfo.IsCloudAsset() && !targetPhotoInfo.IsCloudAsset();
    isValid = isValid && sourcePhotoInfo.dateTrashed.value_or(0) > 0;  // The source is in the trash.
    isValid = isValid && this->mediaAssetsDao_.IsSameAssetIgnoreAlbum(sourcePhotoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET(isValid, E_INVALID_MODE);
    int32_t ret = this->RemoveAssetAndFile(sourcePhotoInfo, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "RemoveAssetAndFile fail, ret: %{public}d", ret);
    MEDIA_INFO_LOG(
        "CommonMergeLocalToLocalAsset completed, "
        "sourceFileId: %{public}d, sourcePosition: %{public}d, targetFileId: %{public}d, targetPosition: %{public}d",
        sourcePhotoInfo.fileId.value_or(0),
        sourcePhotoInfo.position.value_or(0),
        targetPhotoInfo.fileId.value_or(0),
        targetPhotoInfo.position.value_or(0));
    return E_OK;
}

int32_t MediaAssetsRecoverService::MediaAndLakeMergeLocalToCloudAsset(const PhotosPo &sourcePhotoInfo,
    const PhotosPo &targetPhotoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    bool isValid = !sourcePhotoInfo.IsCloudAsset() && targetPhotoInfo.IsCloudAsset();
    isValid = isValid && sourcePhotoInfo.dateTrashed.value_or(0) > 0;  // The source is in the trash.
    isValid = isValid && targetPhotoInfo.hidden.value_or(0) == 0;      // The target is not in the hidden.
    isValid = isValid && this->mediaAssetsDao_.IsSameAssetIgnoreAlbum(sourcePhotoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET(isValid, E_INVALID_MODE);
    // Move the origianl files to target asset.
    int32_t ret = this->MergeAssetFileFromMediaToLake(sourcePhotoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "MergeAssetFile fail, ret: %{public}d", ret);
    ret = this->RemoveAssetAndFile(sourcePhotoInfo, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "RemoveAssetAndFile fail, ret: %{public}d", ret);
    ret = this->mediaAssetsDao_.UpdatePositionToBothAndFileSourceTypeToLake(
        sourcePhotoInfo, targetPhotoInfo, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(
        ret == E_OK, ret, "UpdatePositionToBothAndFileSourceTypeToLake fail, ret: %{public}d", ret);
    MEDIA_INFO_LOG(
        "MediaAndLakeMergeLocalToCloudAsset completed, "
        "sourceFileId: %{public}d, sourcePosition: %{public}d, targetFileId: %{public}d, targetPosition: %{public}d",
        sourcePhotoInfo.fileId.value_or(0),
        sourcePhotoInfo.position.value_or(0),
        targetPhotoInfo.fileId.value_or(0),
        targetPhotoInfo.position.value_or(0));
    return E_OK;
}

int32_t MediaAssetsRecoverService::MediaAndLakeMergeLocalToHiddenCloudAsset(const PhotosPo &sourcePhotoInfo,
    const PhotosPo &targetPhotoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    bool isValid = !sourcePhotoInfo.IsCloudAsset() && targetPhotoInfo.IsCloudAsset();
    isValid = isValid && sourcePhotoInfo.dateTrashed.value_or(0) > 0;  // The source is in the trash.
    isValid = isValid && targetPhotoInfo.hidden.value_or(0) != 0;      // The target is in the hidden.
    isValid = isValid && this->mediaAssetsDao_.IsSameAssetIgnoreAlbum(sourcePhotoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET(isValid, E_INVALID_MODE);
    // Move the origianl files to target asset.
    int32_t ret = this->MergeAssetFile(sourcePhotoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "MergeAssetFile fail, ret: %{public}d", ret);
    ret = this->RemoveAssetAndFile(sourcePhotoInfo, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "RemoveAssetAndFile fail, ret: %{public}d", ret);
    ret = this->mediaAssetsDao_.UpdatePositionToBoth(sourcePhotoInfo, targetPhotoInfo, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "UpdatePositionToBoth fail, ret: %{public}d", ret);
    MEDIA_INFO_LOG(
        "MediaAndLakeMergeLocalToHiddenCloudAsset completed, "
        "sourceFileId: %{public}d, sourcePosition: %{public}d, targetFileId: %{public}d, targetPosition: %{public}d",
        sourcePhotoInfo.fileId.value_or(0),
        sourcePhotoInfo.position.value_or(0),
        targetPhotoInfo.fileId.value_or(0),
        targetPhotoInfo.position.value_or(0));
    return E_OK;
}

int32_t MediaAssetsRecoverService::RecoverPhotoAsset(const std::string &fileUri)
{
    std::vector<std::string> fileUriList = {fileUri};
    DataShare::DataSharePredicates predicates;
    predicates.In(PhotoColumn::MEDIA_ID, fileUriList);
    return MediaLibraryAlbumOperations::RecoverPhotoAssets(predicates);
}

int32_t MediaAssetsRecoverService::StoreThumbnailAndEditSize(const PhotosPo &photoInfo)
{
    int32_t fileId = photoInfo.fileId.value_or(0);
    std::string data = photoInfo.data.value_or("");
    bool isValid = fileId > 0 && !data.empty();
    CHECK_AND_RETURN_RET(isValid, E_INVALID_VALUES);
    MediaLibraryPhotoOperations::StoreThumbnailAndEditSize(std::to_string(fileId), data);
    return E_OK;
}

int32_t MediaAssetsRecoverService::MoveAssetFileFromMediaToFileManager(
    const PhotosPo &photoInfo, const PhotosPo &targetPhotoInfo)
{
    int32_t ret = FileManagerAssetOperations::MoveFileManagerAsset(photoInfo, targetPhotoInfo);
    MEDIA_INFO_LOG(
        "MoveAssetFileFromMediaToFileManager, ret: %{public}d, storagePath: %{public}s,\
        data: %{public}s, errno: %{public}d",
        ret,
        DfxUtils::GetSafePath(photoInfo.data.value_or("")).c_str(),
        DfxUtils::GetSafePath(targetPhotoInfo.storagePath.value_or("")).c_str(),
        errno);
    return ret;
}

int32_t MediaAssetsRecoverService::FillRecoverResults(const std::vector<std::string> &fileIds,
    const std::unordered_map<std::string, std::string> &recoveredUris, std::vector<AssetUriResult> &results)
{
    results.clear();
    results.reserve(fileIds.size());
    size_t failedCount = 0;
    for (const auto &fileId : fileIds) {
        AssetUriResult result;
        result.originalFileId = fileId;
        const auto it = recoveredUris.find(fileId);
        if (it != recoveredUris.end()) {
            result.resultUri = it->second;
        }
        bool isValid = !result.resultUri.empty();
        result.isSuccess = isValid;
        failedCount = isValid ? failedCount : failedCount + 1;
        results.emplace_back(std::move(result));
    }
    MEDIA_INFO_LOG("Recover results built, total: %{public}zu, failed: %{public}zu", results.size(), failedCount);
    return E_OK;
}

int32_t MediaAssetsRecoverService::FillRecoverResults(const std::vector<std::string> &fileIds,
    std::vector<AssetUriResult> &results)
{
    return this->FillRecoverResults(fileIds, std::unordered_map<std::string, std::string>(), results);
}

int32_t MediaAssetsRecoverService::PrepareForBurstRecovery(std::vector<std::string> &fileIds)
{
    CHECK_AND_RETURN_RET_LOG(!fileIds.empty(), E_OK, "fileIds is empty.");
    size_t inputSize = fileIds.size();
    std::vector<std::string> uris;
    BurstDao::CompleteBurstFileIds(fileIds, uris);
    // The input fileIds may already contain the burst members, which will be appended again by
    // CompleteBurstFileIds, so remove the duplicates to avoid recovering the same member twice.
    int32_t ret = this->RemoveDuplicates(fileIds);
    MEDIA_INFO_LOG("input fileIds: %{public}zu, fileIds size: %{public}zu.", inputSize, fileIds.size());
    CHECK_AND_PRINT_LOG(ret == E_OK, "RemoveDuplicates failed, ret: %{public}d", ret);
    return ret;
}

int32_t MediaAssetsRecoverService::RemoveDuplicates(std::vector<std::string> &values)
{
    std::unordered_set<std::string> uniqueValues;
    std::vector<std::string> deduplicated;
    deduplicated.reserve(values.size());
    for (const auto &value : values) {
        if (uniqueValues.insert(value).second) {
            deduplicated.emplace_back(value);
        }
    }
    values.swap(deduplicated);
    return E_OK;
}

int32_t MediaAssetsRecoverService::RecordRecoveredAsset(const PhotosPo &photoInfo, int32_t ret,
    const std::optional<PhotosPo> &targetPhotoInfoOp, std::vector<std::string> &targetFileIds,
    std::unordered_map<std::string, std::string> &recoveredUris)
{
    bool isValid = ret >= E_OK && targetPhotoInfoOp.has_value();
    if (isValid) {
        std::string targetUri = targetPhotoInfoOp.value().BuildFileUri();
        targetFileIds.emplace_back(targetUri);
        recoveredUris[std::to_string(photoInfo.fileId.value_or(-1))] = std::move(targetUri);
    }
    MEDIA_INFO_LOG("RecordRecoveredAsset completed, "
        "isValid: %{public}d, ret: %{public}d, original file_id: %{public}d, target file_id: %{public}d",
        isValid, ret, photoInfo.fileId.value_or(-1), targetPhotoInfoOp.value_or(PhotosPo()).fileId.value_or(-1));
    return isValid ? E_OK : ret;
}
}  // namespace OHOS::Media::Common