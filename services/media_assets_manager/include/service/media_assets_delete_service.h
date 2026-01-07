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

#ifndef OHOS_MEDIA_MEDIA_ASSETS_DELETE_SERVICE_H
#define OHOS_MEDIA_MEDIA_ASSETS_DELETE_SERVICE_H

#include <vector>
#include <mutex>

#include "media_assets_dao.h"
#include "cloud_media_define.h"
#include "cloud_sync_manager.h"

namespace OHOS::Media::Common {
using namespace OHOS::Media::ORM;
class EXPORT MediaAssetsDeleteService {
public: // constructors
    MediaAssetsDeleteService() = default;
    explicit MediaAssetsDeleteService(bool isCloudPullData) : isCloudPullData_(isCloudPullData)
    {}

public:
    int32_t DeleteLocalAssets(const std::vector<std::string> &fileIds);
    int32_t DeleteCloudAssets(const std::vector<std::string> &fileIds);
    int32_t DeleteCloudAssetSingle(const PhotosPo &photoInfo, std::optional<PhotosPo> &targetPhotoInfoOp,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);

private:
    int32_t BatchCopyAndMoveLocalAssetToTrash(
        const std::vector<PhotosPo> &photosList, std::vector<std::string> &targetFileIds);
    int32_t BatchCopyAndMoveCloudAssetToTrash(
        const std::vector<PhotosPo> &photoInfoList, std::vector<std::string> &targetFileIds);
    int32_t CopyAndMoveLocalAssetToTrash(const PhotosPo &photoInfo, std::optional<PhotosPo> &targetPhotoInfoOp,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t BuildTargetFilePath(const PhotosPo &photoInfo, std::string &targetPath);
    int32_t EraseCloudInfo(PhotosPo &photoInfo);
    int32_t ResetFileId(PhotosPo &photoInfo);
    int32_t ResetVirtualPath(PhotosPo &photoInfo);
    int32_t SetDateTrashed(PhotosPo &photoInfo, int64_t dateTrashed);
    int32_t SetPosition(PhotosPo &photoInfo, int32_t position);
    int32_t SetFilePath(PhotosPo &photoInfo, const std::string &filePath);
    int32_t SetFileId(PhotosPo &photoInfo, const int32_t fileId);
    int32_t SetMdirty(PhotosPo &photoInfo);
    int32_t GetValuesBucket(const PhotosPo &photoInfo, NativeRdb::ValuesBucket &valuesBucket);
    int32_t ClearCloudInfo(PhotosPo &photoInfo);
    int32_t ResetNullableFields(PhotosPo &photoInfo);
    int32_t GetCleanFileInfo(const PhotosPo &photoInfo, FileManagement::CloudSync::CleanFileInfo &cleanFileInfo);
    int32_t CleanLocalFileAndCreateDentryFile(
        const PhotosPo &photoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t CreateLocalAssetWithFile(const PhotosPo &photoInfo, PhotosPo &targetPhotoInfo,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t CreateLocalTrashedPhotosPo(const PhotosPo &photoInfo, PhotosPo &targetPhotoInfo);
    int32_t MoveLocalAssetFile(const PhotosPo &photoInfo, const PhotosPo &targetPhotoInfo);
    int32_t CreateCloudTrashedPhotosPo(const PhotosPo &photoInfo, PhotosPo &targetPhotoInfo);
    int32_t CreateCloudAssetThumbnail(const PhotosPo &photoInfo, const PhotosPo &targetPhotoInfo);
    int32_t CreateCloudAssetWithDentryFile(const PhotosPo &photoInfo, PhotosPo &targetPhotoInfo,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t ResetPhotosToLocalOnly(
        const PhotosPo &photoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t ResetFileSourceType(PhotosPo &photoInfo);
    int32_t MoveAssetFileOutOfLake(const PhotosPo &photoInfo);
    int32_t CreateLocalAssetWithLakeFile(const PhotosPo &photoInfo, PhotosPo &targetPhotoInfo,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t CopyAndMoveMediaLocalAssetToTrash(const PhotosPo &photoInfo, std::optional<PhotosPo> &targetPhotoInfoOp,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t CopyAndMoveLakeLocalAssetToTrash(const PhotosPo &photoInfo, std::optional<PhotosPo> &targetPhotoInfoOp,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t CreateNewAssetInfoAndReturnFileId(
        PhotosPo &photoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t CreateCloudAssetWithoutDentryFile(const PhotosPo &photoInfo, PhotosPo &targetPhotoInfo,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t CopyAndMoveMediaCloudAssetToTrash(const PhotosPo &photoInfo, std::optional<PhotosPo> &targetPhotoInfoOp,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t CopyAndMoveLakeCloudAssetToTrash(const PhotosPo &photoInfo, std::optional<PhotosPo> &targetPhotoInfoOp,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t BuildMediaFilePath(const PhotosPo &photoInfo, std::string &targetPath);
    int32_t BuildLakeFilePath(const PhotosPo &photoInfo, std::string &targetPath);
    int32_t SetValue(NativeRdb::ValuesBucket &valuesBucket,
        const std::unordered_map<std::string, std::string> &valueBucketMap, std::stringstream &ss);
    int32_t SetNull4EmptyColumn(NativeRdb::ValuesBucket &valuesBucket,
        const std::unordered_map<std::string, std::string> &valueBucketMap, std::stringstream &ss);
    int32_t SetNull4MissColumn(NativeRdb::ValuesBucket &valuesBucket,
        const std::unordered_map<std::string, std::string> &valueBucketMap, std::stringstream &ss);
    int32_t CopyAndMoveCloudAssetToTrash(const PhotosPo &photoInfo, std::optional<PhotosPo> &targetPhotoInfoOp,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t DeleteLocalAssetSingle(const PhotosPo &photoInfo, std::optional<PhotosPo> &targetPhotoInfoOp,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t DeleteLocalBurstAssets(const PhotosPo &photoInfo, std::optional<PhotosPo> &targetPhotoInfoOp,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t FindBurstAssetsAndResetBurstKey(
        const std::string &originalBurstKey, std::optional<PhotosPo> &coverAsset, std::vector<PhotosPo> &burstAssets);
    int32_t CheckAndFindBurstAssets(
        const PhotosPo &photoInfo, std::optional<PhotosPo> &coverAssetOp, std::vector<PhotosPo> &burstAssets);
    int32_t DeleteCloudBurstAssets(const PhotosPo &photoInfo, std::optional<PhotosPo> &targetPhotoInfoOp,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);

private:
    MediaAssetsDao mediaAssetsDao_;
    std::mutex deleteAssetsMutex_;
    using DeleteFuncHandle = int32_t (MediaAssetsDeleteService::*)(
        const PhotosPo &, std::optional<PhotosPo> &, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &);
    using SetValHandle = int32_t (MediaAssetsDeleteService::*)(
        NativeRdb::ValuesBucket &, const std::unordered_map<std::string, std::string> &, std::stringstream &);
    const bool isCloudPullData_ = false;
};
}  // namespace OHOS::Media::Common
#endif  // OHOS_MEDIA_MEDIA_ASSETS_DELETE_SERVICE_H