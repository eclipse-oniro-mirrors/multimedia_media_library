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

namespace OHOS::Media::Common {
int32_t MediaAssetsRecoverService::BatchMoveOutTrashAndMergeWithSameAsset(
    const std::vector<std::string> &assetIds, std::vector<std::string> &targetFileIds)
{
    std::vector<std::string> fileIds;
    std::transform(
        std::begin(assetIds), std::end(assetIds), std::back_inserter(fileIds), [](const std::string &assetId) {
            return MediaAssetsUtils::GetFileId(assetId);
        });
    MediaLibraryAlbumOperations::DealwithNoAlbumAssets(fileIds);
    // Find assets info.
    std::vector<PhotosPo> photosList;
    int32_t ret = this->mediaAssetsDao_.QueryAssets(fileIds, photosList);
    CHECK_AND_RETURN_RET_LOG(
        ret == E_OK, ret, "QueryAssets fail, ret: %{public}d, fileIds: %{public}zu", ret, fileIds.size());
    std::optional<PhotosPo> targetPhotoInfoOp;
    bool isValid = false;
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> photoRefresh =
        std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr,
        E_RDB_STORE_NULL,
        "BatchMoveOutTrashAndMergeWithSameAsset Failed to get photoRefresh.");
    std::sort(photosList.begin(), photosList.end(), [](const PhotosPo &a, const PhotosPo &b) {
        return a.fileId.value_or(-1) < b.fileId.value_or(-1);
    });
    for (const PhotosPo &photoInfo : photosList) {
        targetPhotoInfoOp.reset();
        ret = this->MoveOutTrashAndMergeWithSameAsset(photoRefresh, photoInfo, targetPhotoInfoOp);
        isValid = ret == E_OK && targetPhotoInfoOp.has_value();
        CHECK_AND_EXECUTE(!isValid, targetFileIds.emplace_back(targetPhotoInfoOp.value().BuildFileUri()));
        MEDIA_INFO_LOG("MoveOutTrashAndMergeWithSameAsset completed, "
                       "isValid: %{public}d, ret: %{public}d, original file_id: %{public}d, target file_id: %{public}d",
            isValid,
            ret,
            photoInfo.fileId.value_or(-1),
            targetPhotoInfoOp.value_or(PhotosPo()).fileId.value_or(-1));
    }
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
    ret = fileOperation.DeleteThumbnail(targetPhotoInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "DeleteThumbnail fail, ret: %{public}d", ret);
    ret = fileOperation.CopyThumbnail(photoInfo, targetPhotoInfo, false);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CopyThumbnail fail, ret: %{public}d", ret);
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
    ret = this->mediaAssetsDao_.UpdatePositionToBoth(targetPhotoInfo, photoRefresh);
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
    ret = this->mediaAssetsDao_.UpdatePositionToBothAndFileSourceTypeToLake(targetPhotoInfo, photoRefresh);
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
    ret = this->mediaAssetsDao_.UpdatePositionToBoth(targetPhotoInfo, photoRefresh);
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
}  // namespace OHOS::Media::Common