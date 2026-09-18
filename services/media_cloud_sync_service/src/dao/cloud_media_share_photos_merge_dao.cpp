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

#define MLOG_TAG "Media_Cloud_Dao"

#include "cloud_media_share_photos_merge_dao.h"

#include <string>
#include <utime.h>
#include <vector>

#include "cloud_media_sync_utils.h"
#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "photos_po_writer.h"
#include "result_set_reader.h"

namespace OHOS::Media::CloudSync {
// LCOV_EXCL_START
int32_t CloudMediaSharePhotosMergeDao::ResetContext()
{
    this->photoInfoList_.clear();
    this->displayNameSet_.clear();
    return E_OK;
}

int32_t CloudMediaSharePhotosMergeDao::LoadCacheDataByDisplayName(
    const std::vector<CloudMediaPullDataDto> &pullDataList)
{
    CHECK_AND_RETURN_RET(!pullDataList.empty(), E_OK);

    int32_t rowCount = 0;
    auto resultSet = this->photosDao_.BatchQueryLocal(pullDataList, {}, rowCount);
    int32_t ret = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(this->photoInfoList_);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK,
        ret,
        "db error, ret: %{public}d, size: %{public}s",
        ret,
        std::to_string(pullDataList.size()).c_str());
    CHECK_AND_RETURN_RET(!this->photoInfoList_.empty(), E_OK);

    for (const auto &photoInfo : this->photoInfoList_) {
        this->displayNameSet_.emplace(photoInfo.displayName.value_or(""));
    }

    MEDIA_INFO_LOG("LoadCacheDataByDisplayName completed, pullData size: %{public}s, data size: %{public}s",
        std::to_string(pullDataList.size()).c_str(),
        std::to_string(this->photoInfoList_.size()).c_str());
    return E_OK;
}

int32_t CloudMediaSharePhotosMergeDao::FindLocalAssetInCacheWithDisplayName(
    const std::string &displayName, std::vector<PhotosPo> &photoInfoList)
{
    CHECK_AND_RETURN_RET(!displayName.empty(), E_OK);
    CHECK_AND_RETURN_RET_LOG(!this->photoInfoList_.empty(), E_OK, "this->photoInfoList_ is empty");
    CHECK_AND_RETURN_RET(this->displayNameSet_.count(displayName) > 0, E_OK);

    photoInfoList.clear();
    bool isNotSameDisplayName = false;
    for (const auto &photoInfo : this->photoInfoList_) {
        isNotSameDisplayName = photoInfo.displayName.value_or("") != displayName;
        CHECK_AND_EXECUTE(isNotSameDisplayName, photoInfoList.emplace_back(photoInfo));
    }
    return E_OK;
}

bool CloudMediaSharePhotosMergeDao::IsSameAsset(const CloudMediaPullDataDto &pullData, const PhotosPo &photoInfo)
{
    CHECK_AND_RETURN_RET(photoInfo.albumInfoOp.has_value(), false);

    const PhotoAlbumPo &albumInfo = photoInfo.albumInfoOp.value();
    const std::string cloudAlbumLocalPath = CloudMediaSyncUtils::GetLpath(pullData);
    const std::string localAlbumLocalPath = albumInfo.lpath.value_or("");
    const bool isSameAlbum = cloudAlbumLocalPath == localAlbumLocalPath;
    CHECK_AND_RETURN_RET(isSameAlbum, false);

    const bool isSameDisplayName = pullData.basicFileName == photoInfo.displayName.value_or("");
    CHECK_AND_RETURN_RET(isSameDisplayName, false);

    const bool isSameSize = pullData.basicSize == photoInfo.size;
    CHECK_AND_RETURN_RET(isSameSize, false);

    int32_t exifRotateValue = ORIENTATION_NORMAL;
    if (pullData.propertiesRotate != -1) {
        exifRotateValue = pullData.propertiesRotate;
    }
    const bool isSameRotate =
        pullData.basicFileType == FILE_TYPE_VIDEO ? true : exifRotateValue == photoInfo.orientation.value_or(0);
    CHECK_AND_RETURN_RET_LOG(
        isSameRotate,
        false,
        "rotate not same, basicFileType:%{public}d, exifRotateValue:%{public}d, orientation:%{public}d",
        pullData.basicFileType,
        exifRotateValue,
        photoInfo.orientation.value_or(0));

    const bool isSameCreatorId = pullData.mediaCreateId == photoInfo.shareOwnerInfo.value_or("");
    CHECK_AND_RETURN_RET_LOG(
        isSameCreatorId,
        false,
        "createId not same, mediaCreateId:%{public}s, shareOwnerInfo:%{public}s",
        pullData.mediaCreateId.c_str(),
        photoInfo.shareOwnerInfo.value_or("").c_str());

    return true;
}

int32_t CloudMediaSharePhotosMergeDao::FindLocalAssetInSameDisplayNamePhotoList(
    CloudMediaPullDataDto &pullData, std::vector<PhotosPo> &photoInfoList)
{
    CHECK_AND_RETURN_RET(!photoInfoList.empty(), E_OK);

    bool isSameAsset = false;
    for (auto &photoInfo : photoInfoList) {
        this->commonDao_.QueryPhotoAlbumByAlbumId(photoInfo.ownerAlbumId.value_or(0), photoInfo.albumInfoOp);
        CHECK_AND_CONTINUE(photoInfo.albumInfoOp.has_value());

        isSameAsset = this->IsSameAsset(pullData, photoInfo);
        CHECK_AND_EXECUTE(!isSameAsset, pullData.localPhotosPoOp = photoInfo);
        CHECK_AND_RETURN_RET(!isSameAsset, E_OK);
    }
    return E_OK;
}

int32_t CloudMediaSharePhotosMergeDao::BatchFindLocalAsset(std::vector<CloudMediaPullDataDto> &pullDataList)
{
    CHECK_AND_RETURN_RET(!pullDataList.empty(), E_OK);

    this->ResetContext();
    this->LoadCacheDataByDisplayName(pullDataList);
    CHECK_AND_RETURN_RET(!this->photoInfoList_.empty(), E_OK);

    for (auto &pullData : pullDataList) {
        this->FindLocalAsset(pullData);
    }
    return E_OK;
}

int32_t CloudMediaSharePhotosMergeDao::FindLocalAsset(CloudMediaPullDataDto &pullData)
{
    const std::string cloudDisplayName = pullData.basicFileName;
    CHECK_AND_RETURN_RET(!cloudDisplayName.empty(), E_OK);

    std::vector<PhotosPo> sameDisplayNamePhotoList;
    this->FindLocalAssetInCacheWithDisplayName(cloudDisplayName, sameDisplayNamePhotoList);
    CHECK_AND_RETURN_RET(!sameDisplayNamePhotoList.empty(), E_OK);

    this->FindLocalAssetInSameDisplayNamePhotoList(pullData, sameDisplayNamePhotoList);
    return E_OK;
}
// LCOV_EXCL_STOP
}  // namespace OHOS::Media::CloudSync
