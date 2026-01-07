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
#ifndef OHOS_MEDIA_BACKUP_DATABASE_REPORT_H
#define OHOS_MEDIA_BACKUP_DATABASE_REPORT_H

#include <string>
#include <vector>

#include "rdb_store.h"
#include "media_backup_report_data_type.h"

namespace OHOS::Media {
class DatabaseReport {
public:
    DatabaseReport &SetSceneCode(int32_t sceneCode)
    {
        this->sceneCode_ = sceneCode;
        return *this;
    }
    DatabaseReport &SetTaskId(const std::string &taskId)
    {
        this->taskId_ = taskId;
        return *this;
    }
    DatabaseReport &ReportGallery(std::shared_ptr<NativeRdb::RdbStore> galleryRdb, bool shouldIncludeSd);
    DatabaseReport &ReportExternal(std::shared_ptr<NativeRdb::RdbStore> externalRdb,
        std::shared_ptr<NativeRdb::RdbStore> galleryRdb);
    DatabaseReport &ReportMedia(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, int32_t period);
    DatabaseReport &ReportAudio(const uint64_t audioCount);

public:
    enum { PERIOD_OLD = -1, PERIOD_BEFORE = 0, PERIOD_AFTER = 1 };

private:
    std::vector<AlbumMediaStatisticInfo> LoadGallery(
        std::shared_ptr<NativeRdb::RdbStore> galleryRdb, bool shouldIncludeSd);
    std::vector<AlbumMediaStatisticInfo> LoadExternal(std::shared_ptr<NativeRdb::RdbStore> externalRdb,
        std::shared_ptr<NativeRdb::RdbStore> galleryRdb);
    std::vector<AlbumMediaStatisticInfo> LoadMedia(
        std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, int32_t period);
    std::vector<AlbumMediaStatisticInfo> LoadAudio(const uint64_t audioCount);
    int32_t Report(std::vector<AlbumMediaStatisticInfo> statisticInfos);
    int32_t PostInfoDfx(const AlbumMediaStatisticInfo &info);
    int32_t PostInfoAuditLog(const AlbumMediaStatisticInfo &info);

private:
    int32_t sceneCode_;
    std::string taskId_;
    bool shouldIncludeSd_;
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_BACKUP_DATABASE_REPORT_H