/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SHARE_PHOTOS_SERVICE_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SHARE_PHOTOS_SERVICE_H

#include <set>
#include <vector>

#include "asset_accurate_refresh.h"
#include "cloud_media_common_dao.h"
#include "cloud_media_define.h"
#include "cloud_media_photos_dao.h"
#include "cloud_media_photos_service.h"
#include "cloud_media_photos_delete_service.h"
#include "cloud_media_pull_data_dto.h"
#include "medialibrary_notify.h"
#include "photos_dto.h"
#include "photos_po.h"
// LCOV_EXCL_START

namespace OHOS::Media::CloudSync {

struct CloudMediaPullDataHandleDto {
    std::vector<PhotosDto> newData;
    std::vector<PhotosDto> fdirtyData;
    std::vector<int32_t> stats;
    std::vector<std::string> failedRecords;
    std::set<std::string> refreshAlbums;
};

class EXPORT CloudMediaSharePhotosService {
public:
    int32_t OnFetchRecords(std::vector<CloudMediaPullDataDto> &pullDataList, std::vector<PhotosDto> &newData,
        std::vector<PhotosDto> &fdirtyData, std::vector<int32_t> &stats, std::vector<std::string> &failedRecords);
    int32_t OnDentryFileInsert(
        const std::vector<CloudMediaPullDataDto> &pullDatas, std::vector<std::string> &failedRecords);

private:
    int32_t HandleRecords(std::vector<CloudMediaPullDataDto> &pullDataList, CloudMediaPullDataHandleDto &handleDto);
    int32_t HandleUpdateOrDeleteRecord(
        const CloudMediaPullDataDto &pullData, CloudMediaPullDataHandleDto &handleDto, NotifyType &notifyType);
    int32_t HandleMergeOrNewRecords(const std::vector<CloudMediaPullDataDto> &pullDataList,
        CloudMediaPullDataHandleDto &handleDto, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t HandleMergeRecords(std::vector<CloudMediaPullDataDto> &pullDataList,
        CloudMediaPullDataHandleDto &handleDto, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t HandleNewRecords(std::vector<CloudMediaPullDataDto> &pullDataList,
        CloudMediaPullDataHandleDto &handleDto, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t HandleCloudDeleteRecord(std::vector<CloudMediaPullDataDto> &pullDataList);
    int32_t GetAllCloudIds(std::vector<CloudMediaPullDataDto> &pullDataList, std::vector<std::string> &cloudIds);
    int32_t MergePhotoInfoIntoPullData(
        std::vector<CloudMediaPullDataDto> &pullDataList, const std::vector<PhotosPo> &photoInfoList);
    int32_t FindLocalPhotoInfo(std::vector<CloudMediaPullDataDto> &pullDataList);
    int32_t PullUpdate(const CloudMediaPullDataDto &pullData, CloudMediaPullDataHandleDto &handleDto);
    int32_t PullDelete(const CloudMediaPullDataDto &pullData, CloudMediaPullDataHandleDto &handleDto);
    int32_t PullInsert(const std::vector<CloudMediaPullDataDto> &pullDatas, std::vector<std::string> &failedRecords);

private:
    CloudMediaPhotosDao photosDao_;
    CloudMediaCommonDao commonDao_;
    CloudMediaPhotosDeleteService photosDeleteService_;
    CloudMediaPhotosService photosService_;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SHARE_PHOTOS_SERVICE_H
// LCOV_EXCL_STOP
