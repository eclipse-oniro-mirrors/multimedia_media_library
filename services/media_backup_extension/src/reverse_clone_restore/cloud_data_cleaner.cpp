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

#include "cloud_data_cleaner.h"
#include "media_column.h"
#include "result_set_utils.h"
#include "medialibrary_errno.h"
#include "settings_data_manager.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
// LCOV_EXCL_START
CloudDataCleaner::CloudDataCleaner(std::shared_ptr<NativeRdb::RdbStore> rdbStore)
    : rdbStore_(rdbStore)
{
    MEDIA_INFO_LOG("CloudDataCleaner constructed");
}

int32_t CloudDataCleaner::ExecuteSql(const string& sql, const vector<ValueObject>& args)
{
    if (rdbStore_ == nullptr) {
        MEDIA_ERR_LOG("CloudDataCleaner: ExecuteSql failed, rdbStore_ is null");
        return E_ERR;
    }

    int32_t ret = rdbStore_->ExecuteSql(sql, args);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CloudDataCleaner: ExecuteSql failed, ret=%{public}d, sql=%{public}s", ret, sql.c_str());
    }
    return ret;
}

bool CloudDataCleaner::HasDataForUpdate()
{
    if (rdbStore_ == nullptr) {
        MEDIA_ERR_LOG("CloudDataCleaner: HasDataForUpdate failed, rdbStore_ is null");
        return false;
    }

    vector<ValueObject> args;
    args.emplace_back(POSITION_CLOUD);
    args.emplace_back(string(DELETE_DISPLAY_NAME));

    auto resultSet = rdbStore_->QuerySql(SQL_CHECK_HAS_DATA_FOR_UPDATE, args);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("CloudDataCleaner: HasDataForUpdate query failed");
        return false;
    }

    bool hasData = (resultSet->GoToNextRow() == E_OK);
    resultSet->Close();

    return hasData;
}

bool CloudDataCleaner::HasLocalAndCloudAssets()
{
    if (rdbStore_ == nullptr) {
        MEDIA_ERR_LOG("CloudDataCleaner: HasLocalAndCloudAssets failed, rdbStore_ is null");
        return false;
    }

    vector<ValueObject> args;
    args.emplace_back(POSITION_LOCAL_AND_CLOUD);    // position = 3
    args.emplace_back(POSITION_LOCAL);              // position = 1
    args.emplace_back(0);                           // cloud_version != 0
    args.emplace_back(DIRTY_TYPE_NEW);              // dirty != 1
    args.emplace_back(DIRTY_TYPE_DELETED);          // dirty != -1
    args.emplace_back(0);                           // south_device_type != 0

    auto resultSet = rdbStore_->QuerySql(SQL_CHECK_HAS_LOCAL_AND_CLOUD_ASSETS, args);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("CloudDataCleaner: HasLocalAndCloudAssets query failed");
        return false;
    }

    bool hasData = (resultSet->GoToNextRow() == E_OK);
    resultSet->Close();

    return hasData;
}

bool CloudDataCleaner::HasNegativeVisitTimeData()
{
    if (rdbStore_ == nullptr) {
        MEDIA_ERR_LOG("CloudDataCleaner: HasNegativeVisitTimeData failed, rdbStore_ is null");
        return false;
    }

    vector<ValueObject> args;
    args.emplace_back(REAL_LCD_VISIT_TIME_INVALID);     // real_lcd_visit_time = -3
    args.emplace_back(string(DELETE_DISPLAY_NAME));     // display_name !=

    auto resultSet = rdbStore_->QuerySql(SQL_CHECK_HAS_NEGATIVE_VISIT_TIME_DATA, args);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("CloudDataCleaner: HasNegativeVisitTimeData query failed");
        return false;
    }

    bool hasData = (resultSet->GoToNextRow() == E_OK);
    resultSet->Close();

    return hasData;
}

bool CloudDataCleaner::UpdateCloudMediaAssets()
{
    MEDIA_INFO_LOG("CloudDataCleaner: Enter UpdateCloudMediaAssets");

    if (rdbStore_ == nullptr) {
        MEDIA_ERR_LOG("CloudDataCleaner: UpdateCloudMediaAssets failed, rdbStore_ is null");
        return false;
    }

    int cycle = 0;
    while (cycle++ < MAX_CYCLES && HasDataForUpdate()) {
        vector<ValueObject> args;
        args.emplace_back(DIRTY_TYPE_DELETED);              // dirty
        args.emplace_back(0);                               // cloud_version
        args.emplace_back(string(DELETE_DISPLAY_NAME));     // display_name
        args.emplace_back(0);                               // real_lcd_visit_time
        args.emplace_back(POSITION_CLOUD);                  // position
        args.emplace_back(string(DELETE_DISPLAY_NAME));     // display_name !=
        args.emplace_back(BATCH_SIZE);                      // LIMIT

        int32_t ret = ExecuteSql(SQL_UPDATE_CLOUD_MEDIA_ASSETS_BATCH, args);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("UpdateCloudMediaAssets failed at cycle %{public}d, ret=%{public}d, retrying...",
                cycle, ret);
            ret = ExecuteSql(SQL_UPDATE_CLOUD_MEDIA_ASSETS_BATCH, args);  // Retry once
            if (ret != E_OK) {
                MEDIA_ERR_LOG("CloudDataCleaner: UpdateCloudMediaAssets failed after retry, ret=%{public}d", ret);
                continue;   // 与云退出一致, 失败后继续处理下一批次
            }
        }
        MEDIA_INFO_LOG("CloudDataCleaner: UpdateCloudMediaAssets cycle %{public}d executed", cycle);
    }

    MEDIA_INFO_LOG("CloudDataCleaner: UpdateCloudMediaAssets completed, cycles=%{public}d", cycle);
    return true;
}

void CloudDataCleaner::DeleteAnalysisBackupAlbums()
{
    MEDIA_INFO_LOG("CloudDataCleaner: Enter DeleteAnalysisBackupAlbums");

    if (rdbStore_ == nullptr) {
        MEDIA_ERR_LOG("CloudDataCleaner: DeleteAnalysisBackupAlbums failed, rdbStore_ is null");
        return;
    }

    int32_t ret = ExecuteSql(SQL_DROP_ANALYSIS_BACKUP_TABLE);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CloudDataCleaner: DeleteAnalysisBackupAlbums failed, ret=%{public}d", ret);
    } else {
        MEDIA_INFO_LOG("CloudDataCleaner: DeleteAnalysisBackupAlbums completed successfully");
    }
}

int32_t CloudDataCleaner::DeleteEmptyCloudAlbums()
{
    MEDIA_INFO_LOG("CloudDataCleaner: Enter DeleteEmptyCloudAlbums");

    if (rdbStore_ == nullptr) {
        MEDIA_ERR_LOG("CloudDataCleaner: DeleteEmptyCloudAlbums failed, rdbStore_ is null");
        return E_ERR;
    }

    vector<ValueObject> args;
    args.emplace_back(ALBUM_FROM_CLOUD);          // album_is_local = 2
    args.emplace_back(CLEAN_FLAG_NOT_CLEAN);      // clean_flag = 0
    args.emplace_back(DIRTY_TYPE_DELETED);        // dirty = -1

    int32_t ret = ExecuteSql(SQL_DELETE_EMPTY_CLOUD_ALBUMS, args);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CloudDataCleaner: DeleteEmptyCloudAlbums failed, ret=%{public}d", ret);
    } else {
        MEDIA_INFO_LOG("CloudDataCleaner: DeleteEmptyCloudAlbums completed successfully");
    }
    return ret;
}

int32_t CloudDataCleaner::ClearDeletedDbData()
{
    MEDIA_INFO_LOG("CloudDataCleaner: Enter ClearDeletedDbData");

    if (rdbStore_ == nullptr) {
        MEDIA_ERR_LOG("CloudDataCleaner: ClearDeletedDbData failed, rdbStore_ is null");
        return E_ERR;
    }

    vector<ValueObject> args;
    args.emplace_back(static_cast<int32_t>(DirtyType::TYPE_DELETED));

    int32_t ret = ExecuteSql(SQL_CLEAR_DELETED_DB_DATA, args);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CloudDataCleaner: ClearDeletedDbData failed, ret=%{public}d", ret);
    } else {
        MEDIA_INFO_LOG("CloudDataCleaner: ClearDeletedDbData completed successfully");
    }
    return ret;
}

bool CloudDataCleaner::UpdateBothLocalAndCloudAssets()
{
    MEDIA_INFO_LOG("CloudDataCleaner: Enter UpdateBothLocalAndCloudAssets");

    if (rdbStore_ == nullptr) {
        MEDIA_ERR_LOG("CloudDataCleaner: UpdateBothLocalAndCloudAssets failed, rdbStore_ is null");
        return false;
    }

    // 先执行删除 dirty=-1 的记录（对齐原始流程，内部第一步就是 ClearDeletedDbData）
    int32_t deleteRet = ClearDeletedDbData();
    if (deleteRet != E_OK) {
        MEDIA_ERR_LOG("CloudDataCleaner: ClearDeletedDbData failed inside "
            "UpdateBothLocalAndCloudAssets, ret=%{public}d", deleteRet);
        // 原始流程只打印日志，不终止，这里保持一致
    }

    int cycle = 0;
    while (cycle < MAX_CYCLES && HasLocalAndCloudAssets()) {
        vector<ValueObject> args;
        args.emplace_back(DIRTY_TYPE_NEW);              // dirty = 1
        args.emplace_back(0);                           // cloud_version = 0
        args.emplace_back(POSITION_LOCAL);              // position = 1
        args.emplace_back(0);                           // south_device_type = 0
        args.emplace_back(POSITION_LOCAL_AND_CLOUD);    // 子查询 position = 3
        args.emplace_back(POSITION_LOCAL);              // 子查询 position = 1
        args.emplace_back(0);                           // 子查询 cloud_version != 0
        args.emplace_back(DIRTY_TYPE_NEW);              // 子查询 dirty != 1
        args.emplace_back(DIRTY_TYPE_DELETED);          // 子查询 dirty != -1
        args.emplace_back(0);                           // 子查询 south_device_type != 0
        args.emplace_back(BATCH_SIZE);                  // LIMIT

        int32_t ret = ExecuteSql(SQL_UPDATE_LOCAL_AND_CLOUD_ASSETS_BATCH, args);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("CloudDataCleaner: UpdateBothLocalAndCloudAssets failed at cycle %{public}d, "
                "ret=%{public}d, break.", cycle, ret);
            break;  // 对齐原始流程：失败直接跳出循环，不重试
        }
        cycle++;
        MEDIA_INFO_LOG("CloudDataCleaner: UpdateBothLocalAndCloudAssets cycle %{public}d executed", cycle);
    }

    MEDIA_INFO_LOG("CloudDataCleaner: UpdateBothLocalAndCloudAssets completed, cycles=%{public}d", cycle);
    return true;  // 对齐原始流程：失败不终止整体清理，始终返回成功
}

int32_t CloudDataCleaner::UpdateLocalAlbums()
{
    MEDIA_INFO_LOG("CloudDataCleaner: Enter UpdateLocalAlbums");

    if (rdbStore_ == nullptr) {
        MEDIA_ERR_LOG("CloudDataCleaner: UpdateLocalAlbums failed, rdbStore_ is null");
        return E_ERR;
    }

    // SQL1: 清理所有相册的云信息，标记为已修改
    int32_t ret = ExecuteSql(SQL_UPDATE_LOCAL_ALBUMS_CLEAR_CLOUD_ID);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CloudDataCleaner: UpdateLocalAlbums clear cloud info failed, ret=%{public}d", ret);
    }

    MEDIA_INFO_LOG("CloudDataCleaner: UpdateLocalAlbums completed successfully");
    return ret;
}

bool CloudDataCleaner::CleanCloudDataWithNegativeVisitTime()
{
    MEDIA_INFO_LOG("CloudDataCleaner: Enter CleanCloudDataWithNegativeVisitTime");

    if (rdbStore_ == nullptr) {
        MEDIA_ERR_LOG("CloudDataCleaner: CleanCloudDataWithNegativeVisitTime failed, rdbStore_ is null");
        return false;
    }

    int cycle = 0;
    while (cycle++ < MAX_CYCLES && HasNegativeVisitTimeData()) {
        vector<ValueObject> args;
        args.emplace_back(string(DELETE_DISPLAY_NAME));     // display_name
        args.emplace_back(REAL_LCD_VISIT_TIME_INVALID);     // real_lcd_visit_time = -3
        args.emplace_back(string(DELETE_DISPLAY_NAME));     // display_name !=
        args.emplace_back(BATCH_SIZE);                      // LIMIT

        int32_t ret = ExecuteSql(SQL_UPDATE_NEGATIVE_VISIT_TIME_BATCH, args);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("CloudDataCleaner: CleanCloudDataWithNegativeVisitTime failed at cycle %{public}d, "
                "ret=%{public}d", cycle, ret);
            continue;
        }
        MEDIA_INFO_LOG("CloudDataCleaner: CleanCloudDataWithNegativeVisitTime cycle %{public}d executed", cycle);
    }

    MEDIA_INFO_LOG("CloudDataCleaner: CleanCloudDataWithNegativeVisitTime completed, cycles=%{public}d", cycle);
    return true;
}

bool CloudDataCleaner::CleanCloudData()
{
    MEDIA_INFO_LOG("CloudDataCleaner: Enter CleanCloudData");

    // Step 1: Update cloud media assets (pure cloud data)
    if (!UpdateCloudMediaAssets()) {
        MEDIA_ERR_LOG("CloudDataCleaner: UpdateCloudMediaAssets failed");
        return false;
    }

    // Step 2: Skip refresh albums (original does MediaLibraryRdbUtils::UpdateAllAlbums with notification)
    // We skip this as UpdateLocalAlbums will handle album updates later

    // Step 3: Delete analysis backup albums
    DeleteAnalysisBackupAlbums();

    // Step 4: Delete empty cloud albums
    DeleteEmptyCloudAlbums();

    // Step 5: Clear dirty deleted records, in UpdateBothLocalAndCloudAssets
    // Step 6: Update both local and cloud assets (内部会先调用 ClearDeletedDbData)
    if (!UpdateBothLocalAndCloudAssets()) {
        MEDIA_ERR_LOG("CloudDataCleaner: UpdateBothLocalAndCloudAssets failed");
        return false;
    }

    // Step 7: Update local albums
    UpdateLocalAlbums();


    MEDIA_INFO_LOG("CloudDataCleaner: CleanCloudData completed successfully");
    return true;
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS
