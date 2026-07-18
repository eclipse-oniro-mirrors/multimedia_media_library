/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#define MLOG_TAG "Media_Reverse_Restore"

#include "photo_count_strategy.h"
#include "media_log.h"
#include "backup_database_utils.h"

namespace OHOS {
namespace Media {

int32_t StandardCountStrategy::GetOldCount(std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
    bool isCloudRestoreSatisfied)
{
    // 旧机是否统计根据云图克隆条件决定
    return isCloudRestoreSatisfied ? BackupDatabaseUtils::GetDbPhotoCount(mediaRdb, false) :
        BackupDatabaseUtils::GetDbPhotoCount(mediaRdb, true);
}

int32_t StandardCountStrategy::GetNewCount(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    bool isCloudRestoreSatisfied)
{
    // 新机云图不吸收场景，新机不统计云图
    (void)isCloudRestoreSatisfied;
    return BackupDatabaseUtils::GetDbPhotoCount(mediaLibraryRdb, true);
}

int32_t CloudAbsorbCountStrategy::GetOldCount(std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
    bool isCloudRestoreSatisfied)
{
    // 旧机是否统计根据云图克隆条件决定
    return isCloudRestoreSatisfied ? BackupDatabaseUtils::GetDbPhotoCount(mediaRdb, false) :
        BackupDatabaseUtils::GetDbPhotoCount(mediaRdb, true);
}

int32_t CloudAbsorbCountStrategy::GetNewCount(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    bool isCloudRestoreSatisfied)
{
    // 新机云图吸收场景，必统计云图
    (void)isCloudRestoreSatisfied;
    return BackupDatabaseUtils::GetDbPhotoCount(mediaLibraryRdb, false);
}

int32_t CloudAbsorbCountStrategy::GetCloudPositionPhotoCount(
    std::shared_ptr<NativeRdb::RdbStore> rdbStore)
{
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("GetCloudPositionPhotoCount: rdb is nullptr");
        return -1;
    }

    const std::string querySql = "SELECT COUNT(1) FROM Photos"
        " WHERE position = 2 AND sync_status = 0 AND clean_flag = 0 AND time_pending = 0 AND is_temp = 0";

    auto resultSet = BackupDatabaseUtils::QuerySql(rdbStore, querySql, {});
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("GetCloudPositionPhotoCount: query failed");
        return -1;
    }

    int32_t err = resultSet->GoToFirstRow();
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("GetCloudPositionPhotoCount: query error");
        resultSet->Close();
        return err == NativeRdb::E_SQLITE_CORRUPT ? -NativeRdb::E_SQLITE_CORRUPT : -1;
    }

    int32_t count = 0;
    err = resultSet->GetInt(0, count);
    resultSet->Close();

    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("GetCloudPositionPhotoCount: get int failed, err=%{public}d", err);
        return -1;
    }

    MEDIA_INFO_LOG("GetCloudPositionPhotoCount: %{public}d", count);
    return count;
}

} // namespace Media
} // namespace OHOS