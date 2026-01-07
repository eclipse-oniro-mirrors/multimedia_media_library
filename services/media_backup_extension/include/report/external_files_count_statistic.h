/*
 * Copyright (C) 2024-2025 Huawei Device Co., Ltd.
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
#ifndef OHOS_MEDIA_BACKUP_EXTERNAL_FILES_COUNT_STATISTIC_H
#define OHOS_MEDIA_BACKUP_EXTERNAL_FILES_COUNT_STATISTIC_H

#include <string>
#include <vector>

#include "rdb_store.h"
#include "result_set_utils.h"
#include "media_backup_report_data_type.h"

namespace OHOS::Media {
class ExternalFilesCountStatistic {
public:
    ExternalFilesCountStatistic &SetExternalRdb(std::shared_ptr<NativeRdb::RdbStore> externalRdb)
    {
        this->externalRdb_ = externalRdb;
        return *this;
    }
    ExternalFilesCountStatistic &SetGalleryRdb(std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
    {
        this->galleryRdb_ = galleryRdb;
        return *this;
    }
    ExternalFilesCountStatistic &SetSceneCode(int32_t sceneCode)
    {
        this->sceneCode_ = sceneCode;
        return *this;
    }
    ExternalFilesCountStatistic &SetTaskId(const std::string &taskId)
    {
        this->taskId_ = taskId;
        return *this;
    }
    std::vector<AlbumMediaStatisticInfo> Load();

private:
    int32_t QueryExternalAudioCount();
    std::unordered_map<int32_t, int32_t> QueryExternalImageAndVideoCount();
    std::unordered_map<int32_t, int32_t> QueryGalleryNotSyncMedia(const int32_t maxId);
    AlbumMediaStatisticInfo GetMediaStatInfo();
    AlbumMediaStatisticInfo GetAudioStatInfo();
    AlbumMediaStatisticInfo GetGalleryNotSyncMediaStatInfo();
    void GetMediaStatInfoFromMap(const std::unordered_map<int32_t, int32_t>& mediaTypeCountMap,
        AlbumMediaStatisticInfo& info);
private:
    std::shared_ptr<NativeRdb::RdbStore> externalRdb_;
    std::shared_ptr<NativeRdb::RdbStore> galleryRdb_;
    int32_t sceneCode_;
    std::string taskId_;
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_BACKUP_EXTERNAL_FILES_COUNT_STATISTIC_H