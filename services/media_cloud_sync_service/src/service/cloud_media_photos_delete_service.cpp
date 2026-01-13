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

#define MLOG_TAG "Media_Cloud_Service"

#include "cloud_media_photos_delete_service.h"

#include "media_log.h"
#include "photos_po.h"
#include "photo_album_upload_status_operation.h"

namespace OHOS::Media::CloudSync {
using namespace OHOS::Media::ORM;
bool CloudMediaPhotosDeleteService::FindAlbumUploadStatus(CloudMediaPullDataDto &pullData)
{
    CHECK_AND_RETURN_RET_LOG(pullData.localPhotosPoOp.has_value(), false, "localPhotosPoOp has no value");
    PhotosPo localPhotosPo = pullData.localPhotosPoOp.value();
    // hidden photo, upload_status fixed to 0. (0 - not upload, 1 - upload)
    CHECK_AND_RETURN_RET(localPhotosPo.hidden.value_or(0) != 1, false);
    CHECK_AND_EXECUTE(pullData.albumInfoOp.has_value(), this->FindPhotoAlbum(pullData));
    // Album not found, default 0 : not upload
    CHECK_AND_RETURN_RET_LOG(pullData.albumInfoOp.has_value(),
        false,
        "albumInfoOp has no value, cloudId: %{public}s",
        pullData.cloudId.c_str());
    PhotoAlbumPo albumInfo = pullData.albumInfoOp.value();
    MEDIA_INFO_LOG("FindAlbumUploadStatus, albumInfo: %{public}s", albumInfo.ToString().c_str());
    // Camera album, upload_status fixed to 1 : upload.
    CHECK_AND_RETURN_RET(!albumInfo.IsCamera(), true);
    return albumInfo.uploadStatus.value_or(0) == 1 ? true : false;
}

bool CloudMediaPhotosDeleteService::IsClearCloudInfoOnly(CloudMediaPullDataDto &pullData)
{
    CHECK_AND_RETURN_RET_LOG(pullData.localPhotosPoOp.has_value(), false, "localPhotosPoOp has no value");
    PhotosPo localPhotosPo = pullData.localPhotosPoOp.value();
    return pullData.basicIsDelete &&                      // delete from cloud.
           localPhotosPo.dateTrashed.value_or(0) == 0 &&  // not in trash.
           localPhotosPo.position.value_or(1) == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD) &&
           this->FindAlbumUploadStatus(pullData) == false &&
           PhotoAlbumUploadStatusOperation::IsSupportUploadStatus();
}

int32_t CloudMediaPhotosDeleteService::FindPhotoAlbum(CloudMediaPullDataDto &pullData)
{
    CHECK_AND_RETURN_RET_LOG(pullData.localPhotosPoOp.has_value(), E_ERR, "localPhotosPoOp has no value");
    PhotosPo localPhotosPo = pullData.localPhotosPoOp.value();
    // query album info from database by owner_album_id or source_path
    std::optional<PhotoAlbumPo> photoAlbumPoOp;
    int32_t ret = this->mediaAssetsDao_.QueryAlbum(
        pullData.localOwnerAlbumId, localPhotosPo.sourcePath.value_or(""), photoAlbumPoOp);
    bool isValid = ret == E_OK && photoAlbumPoOp.has_value();
    CHECK_AND_RETURN_RET_LOG(isValid,
        ret,
        "FindPhotoAlbum failed, ret: %{public}d, PhotoAlbumPo has_value: %{public}d",
        ret,
        photoAlbumPoOp.has_value());
    pullData.albumInfoOp = photoAlbumPoOp;
    return E_OK;
}

int32_t CloudMediaPhotosDeleteService::PullClearCloudInfo(const CloudMediaPullDataDto &pullData,
    std::set<std::string> &refreshAlbums, std::vector<int32_t> &stats,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    int32_t ret = this->photosDao_.ClearCloudInfo(pullData.cloudId, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "PullClearCloudInfo failed, ret: %{public}d", ret);
    stats[StatsIndex::DELETE_RECORDS_COUNT]++;
    return E_OK;
}

bool CloudMediaPhotosDeleteService::IsMoveOnlyCloudAssetIntoTrash(CloudMediaPullDataDto &pullData)
{
    CHECK_AND_RETURN_RET_LOG(pullData.localPhotosPoOp.has_value(), false, "localPhotosPoOp has no value");
    PhotosPo localPhotosPo = pullData.localPhotosPoOp.value();
    return !pullData.basicIsDelete &&
           localPhotosPo.position.value_or(1) == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD) &&
           pullData.basicRecycledTime > 0 && localPhotosPo.dateTrashed.value_or(0) == 0 &&
           this->FindAlbumUploadStatus(pullData) == false &&
           PhotoAlbumUploadStatusOperation::IsSupportUploadStatus();
}

bool CloudMediaPhotosDeleteService::IsMoveOutFromTrash(CloudMediaPullDataDto &pullData)
{
    CHECK_AND_RETURN_RET_LOG(pullData.localPhotosPoOp.has_value(), false, "localPhotosPoOp has no value");
    PhotosPo localPhotosPo = pullData.localPhotosPoOp.value();
    return pullData.basicRecycledTime == 0 && localPhotosPo.dateTrashed.value_or(0) > 0;
}

int32_t CloudMediaPhotosDeleteService::CopyAndMoveCloudAssetToTrash(
    CloudMediaPullDataDto &pullData, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    CHECK_AND_RETURN_RET_LOG(
        pullData.localPhotosPoOp.has_value(), E_CLOUDSYNC_INVAL_ARG, "localPhotosPoOp has no value");
    PhotosPo localPhotosPo = pullData.localPhotosPoOp.value();
    std::optional<PhotosPo> targetPhotoInfoOp;
    int32_t ret =
        this->mediaAssetsDeleteService_.DeleteCloudAssetSingle(localPhotosPo, targetPhotoInfoOp, photoRefresh);
    bool isValid = ret == E_OK && targetPhotoInfoOp.has_value();
    CHECK_AND_EXECUTE(!isValid, pullData.localPhotosPoOp = targetPhotoInfoOp);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "DeleteCloudAssetSingle failed, ret: %{public}d", ret);
    return E_OK;
}

int32_t CloudMediaPhotosDeleteService::MoveOutTrashAndMergeWithSameAsset(
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh, CloudMediaPullDataDto &pullData)
{
    CHECK_AND_RETURN_RET_LOG(
        pullData.localPhotosPoOp.has_value(), E_CLOUDSYNC_INVAL_ARG, "localPhotosPoOp has no value");
    PhotosPo localPhotosPo = pullData.localPhotosPoOp.value();
    std::optional<PhotosPo> targetLocalPhotosPoOp;
    int32_t ret = this->mediaAssetsRecoverService_.MoveOutTrashAndMergeWithSameAsset(
        photoRefresh, localPhotosPo, targetLocalPhotosPoOp);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "MoveOutTrashAndMergeWithSameAsset failed, ret: %{public}d", ret);
    return E_OK;
}
}  // namespace OHOS::Media::CloudSync