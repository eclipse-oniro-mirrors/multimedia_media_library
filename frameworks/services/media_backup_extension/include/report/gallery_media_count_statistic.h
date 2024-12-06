/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#ifndef OHOS_MEDIA_BACKUP_GALLERY_MEDIA_COUNT_STATISTIC_H
#define OHOS_MEDIA_BACKUP_GALLERY_MEDIA_COUNT_STATISTIC_H

#include <string>
#include <vector>

#include "rdb_store.h"
#include "result_set_utils.h"
#include "media_backup_report_data_type.h"

namespace OHOS::Media {
class GalleryMediaCountStatistic {
public:
    GalleryMediaCountStatistic &SetGalleryRdb(std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
    {
        this->galleryRdb_ = galleryRdb;
        return *this;
    }
    GalleryMediaCountStatistic &SetSceneCode(int32_t sceneCode)
    {
        this->sceneCode_ = sceneCode;
        return *this;
    }
    GalleryMediaCountStatistic &SetShouldIncludeSd(bool shouldIncludeSd)
    {
        this->shouldIncludeSd_ = shouldIncludeSd;
        return *this;
    }
    GalleryMediaCountStatistic &SetTaskId(const std::string &taskId)
    {
        this->taskId_ = taskId;
        return *this;
    }
    std::vector<AlbumMediaStatisticInfo> Load();

private:
    int32_t QueryGalleryAllCount();
    int32_t QueryGalleryImageCount();
    int32_t QueryGalleryVideoCount();
    int32_t QueryGalleryHiddenCount();
    int32_t QueryGalleryTrashedCount();
    int32_t QueryGalleryCloneCount();
    int32_t QueryGallerySdCardCount();
    int32_t QueryGalleryScreenVideoCount();
    int32_t QueryGalleryCloudCount();
    int32_t QueryGalleryFavoriteCount();
    int32_t QueryGalleryImportsCount();
    int32_t QueryGalleryBurstCoverCount();
    int32_t QueryGalleryBurstTotalCount();
    bool HasLowQualityImage();
    int32_t GetGalleryMediaCount();
    int32_t QueryGalleryAppTwinDataCount();
    AlbumMediaStatisticInfo GetAllStatInfo();
    AlbumMediaStatisticInfo GetSdCardStatInfo();
    AlbumMediaStatisticInfo GetScreenStatInfo();
    AlbumMediaStatisticInfo GetImportsStatInfo();
    AlbumMediaStatisticInfo GetAllRestoreStatInfo();
    AlbumMediaStatisticInfo GetDuplicateStatInfo();
    AlbumMediaStatisticInfo GetAppTwinStatInfo();

private:
    std::shared_ptr<NativeRdb::RdbStore> galleryRdb_;
    int32_t sceneCode_;
    bool shouldIncludeSd_{false};
    std::string taskId_;
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_BACKUP_GALLERY_MEDIA_COUNT_STATISTIC_H