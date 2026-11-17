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

#ifndef OHOS_MEDIA_MEDIA_ASSETS_RECOVER_SERVICE_H
#define OHOS_MEDIA_MEDIA_ASSETS_RECOVER_SERVICE_H

#include <vector>
#include <mutex>
#include <unordered_map>

#include "media_assets_dao.h"
#include "cloud_media_define.h"
#include "cloud_sync_manager.h"
#include "asset_uri_result_vo.h"

namespace OHOS::Media::Common {
using namespace OHOS::Media::ORM;
class EXPORT MediaAssetsRecoverService {
public:
    // targetFileIds only holds the uris of the assets which are moved out of the trash successfully.
    // results holds one item per input asset id, in the same order as fileIds.
    int32_t BatchMoveOutTrashAndMergeWithSameAsset(const std::vector<std::string> &fileIds,
        std::vector<std::string> &targetFileIds, std::vector<AssetUriResult> &results);
    int32_t MoveOutTrashAndMergeWithSameAsset(std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh,
        const PhotosPo &photoInfo, std::optional<PhotosPo> &targetPhotoInfoOp);

private:
    int32_t MergeAssetFile(const PhotosPo &photoInfo, const PhotosPo &targetPhotoInfo);
    int32_t RemoveAssetAndFile(
        const PhotosPo &photoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t MoveAssetFileFromMediaToLake(const PhotosPo &photoInfo, const PhotosPo &targetPhotoInfo);
    int32_t MergeAssetFileFromMediaToLake(const PhotosPo &photoInfo, const PhotosPo &targetPhotoInfo);
    /****Routers of Chain-Of-Responsibility*****/
    int32_t MergeSameAssets(const PhotosPo &sourcePhotoInfo, const PhotosPo &targetPhotoInfo,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t MergeSameAssetOfMediaAndMedia(const PhotosPo &sourcePhotoInfo, const PhotosPo &targetPhotoInfo,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t MergeSameAssetOfMediaAndLake(const PhotosPo &sourcePhotoInfo, const PhotosPo &targetPhotoInfo,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    /****Common process, trashed file source is Media*****/
    int32_t CommonMergeDiffCloudAsset(const PhotosPo &sourcePhotoInfo, const PhotosPo &targetPhotoInfo,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t CommonMergeSameCloudAsset(const PhotosPo &sourcePhotoInfo, const PhotosPo &targetPhotoInfo,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t CommonMergeCloudToLocalAsset(const PhotosPo &sourcePhotoInfo, const PhotosPo &targetPhotoInfo,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t CommonMergeLocalToLocalAsset(const PhotosPo &sourcePhotoInfo, const PhotosPo &targetPhotoInfo,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    /****Media and Media*****/
    int32_t MediaAndMediaMergeLocalToCloudAsset(const PhotosPo &sourcePhotoInfo, const PhotosPo &targetPhotoInfo,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    /****Media and Lake****/
    int32_t MediaAndLakeMergeLocalToCloudAsset(const PhotosPo &sourcePhotoInfo, const PhotosPo &targetPhotoInfo,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t MediaAndLakeMergeLocalToHiddenCloudAsset(const PhotosPo &sourcePhotoInfo, const PhotosPo &targetPhotoInfo,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t RecoverPhotoAsset(const std::string &fileUri);
    int32_t StoreThumbnailAndEditSize(const PhotosPo &photoInfo);
    int32_t RecoverPackageName(const PhotosPo &sourcePhotoInfo, const PhotosPo &targetPhotoInfo,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t MoveAssetFileFromMediaToFileManager(const PhotosPo &photoInfo, const PhotosPo &targetPhotoInfo);
    // Builds one result per input fileId, so that the results are aligned with the input assets.
    int32_t FillRecoverResults(const std::vector<std::string> &fileIds,
        const std::unordered_map<std::string, std::string> &recoveredUris,
        std::vector<AssetUriResult> &results);
    // Marks every input fileId as failed, used when the batch cannot be executed at all.
    int32_t FillRecoverResults(const std::vector<std::string> &fileIds, std::vector<AssetUriResult> &results);
    // Completes the burst members of the input fileIds, so that the whole burst group can be recovered
    // even when only the cover uri is passed in.
    int32_t PrepareForBurstRecovery(std::vector<std::string> &fileIds);
    // Removes the duplicated elements of values, keeping the order of the first occurrence.
    int32_t RemoveDuplicates(std::vector<std::string> &values);
    // Records the result of one asset which has been moved out of the trash: only the assets recovered
    // successfully are collected into targetFileIds and recoveredUris.
    int32_t RecordRecoveredAsset(const PhotosPo &photoInfo, int32_t ret,
        const std::optional<PhotosPo> &targetPhotoInfoOp, std::vector<std::string> &targetFileIds,
        std::unordered_map<std::string, std::string> &recoveredUris);

private:
    MediaAssetsDao mediaAssetsDao_;
    std::mutex moveOutTrashMutex_;
    using MergeFuncHandle = int32_t (MediaAssetsRecoverService::*)(
        const PhotosPo &, const PhotosPo &, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &);
};
}  // namespace OHOS::Media::Common
#endif  // OHOS_MEDIA_MEDIA_ASSETS_RECOVER_SERVICE_H