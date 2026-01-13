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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_PHOTOS_DELETE_SERVICE_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_PHOTOS_DELETE_SERVICE_H

#include <string>
#include <vector>

#include "cloud_media_define.h"
#include "cloud_media_pull_data_dto.h"
#include "cloud_media_photos_dao.h"
#include "media_assets_delete_service.h"
#include "asset_accurate_refresh.h"
#include "media_assets_recover_service.h"

namespace OHOS::Media::CloudSync {
class EXPORT CloudMediaPhotosDeleteService {
public:
    int32_t PullClearCloudInfo(const CloudMediaPullDataDto &pullData, std::set<std::string> &refreshAlbums,
        std::vector<int32_t> &stats, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    bool IsClearCloudInfoOnly(CloudMediaPullDataDto &pullData);
    bool IsMoveOnlyCloudAssetIntoTrash(CloudMediaPullDataDto &pullData);
    int32_t CopyAndMoveCloudAssetToTrash(
        CloudMediaPullDataDto &pullData, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    bool IsMoveOutFromTrash(CloudMediaPullDataDto &pullData);
    int32_t MoveOutTrashAndMergeWithSameAsset(
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh, CloudMediaPullDataDto &pullData);

private:
    int32_t FindPhotoAlbum(CloudMediaPullDataDto &pullData);
    bool FindAlbumUploadStatus(CloudMediaPullDataDto &pullData);

private:
    CloudMediaPhotosDao photosDao_;
    Common::MediaAssetsDao mediaAssetsDao_;
    Common::MediaAssetsDeleteService mediaAssetsDeleteService_ = Common::MediaAssetsDeleteService(true);
    Common::MediaAssetsRecoverService mediaAssetsRecoverService_;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_PHOTOS_DELETE_SERVICE_H