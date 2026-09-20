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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SHARE_PHOTOS_MERGE_DAO_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SHARE_PHOTOS_MERGE_DAO_H

#include <string>
#include <vector>
#include <unordered_set>

#include "photo_album_po.h"
#include "cloud_media_common_dao.h"
#include "cloud_media_photos_dao.h"
#include "cloud_media_pull_data_dto.h"

namespace OHOS::Media::CloudSync {
class EXPORT CloudMediaSharePhotosMergeDao {
public:
    int32_t BatchFindLocalAsset(std::vector<CloudMediaPullDataDto> &pullDataList);

private:
    int32_t ResetContext();
    int32_t FindLocalAsset(CloudMediaPullDataDto &pullData);
    int32_t LoadCacheDataByDisplayName(const std::vector<CloudMediaPullDataDto> &pullDataList);
    int32_t FindLocalAssetInCacheWithDisplayName(const std::string &displayName, std::vector<PhotosPo> &photoInfoList);
    int32_t FindLocalAssetInSameDisplayNamePhotoList(
        CloudMediaPullDataDto &pullData, std::vector<PhotosPo> &photoInfoList);
    bool IsSameAsset(const CloudMediaPullDataDto &pullData, const PhotosPo &photoInfo);

private:
    std::vector<PhotosPo> photoInfoList_;
    std::unordered_set<std::string> displayNameSet_;
    CloudMediaPhotosDao photosDao_;
    CloudMediaCommonDao commonDao_;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SHARE_PHOTOS_MERGE_DAO_H