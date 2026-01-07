/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License"){return 0;}
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

#include "cloud_media_retain_smart_data.h"

#include <mutex>
#include <string>

#include "parameters.h"
#include "medialibrary_errno.h"
#include "media_log.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_tracer.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "rdb_predicates.h"
#include "media_file_utils.h"
#include "medialibrary_async_worker.h"
#include "medialibrary_rdb_utils.h"

namespace OHOS::Media {

static const std::string CLOUD_RETIAN_LAST_STATUS_KEY = "persist.multimedia.medialibrary.retain.cloud.last_status";
static const std::string HDC_RETIAN_LAST_STATUS_KEY = "persist.multimedia.medialibrary.retain.hdc.last_status";

static const std::string IS_RETAIN_SMART_DATA = "persist.multimedia.medialibrary.retain.isretainsmartdata";
static const std::string IS_RECOVER_SMART_DATA = "persist.multimedia.medialibrary.retain.isrecoversmartdata";
static const std::string SMART_DATA_RETAIN_TIME = "persist.multimedia.medialibrary.retain.smartdataretaintime";
static const std::string SMART_DATA_PROCESSING_MODE = "persist.multimedia.medialibrary.retain.smartdataprocessingmode";
static const std::string SMART_DATA_CLEAN_STATE = "persist.multimedia.medialibrary.smartdatacleanstate";
static const std::string SMART_DATA_UPDATE_STATE = "persist.multimedia.medialibrary.smartdataupdatestate";

static inline bool SetSystemParameter(const std::string& key, int64_t value)
{
    std::string valueStr = std::to_string(value);
    return system::SetParameter(key, valueStr);
}

void SetSmartDataCleanState(CleanTaskState currentState)
{
    int32_t stateValue = static_cast<int64_t>(currentState);

    std::lock_guard<std::mutex> lock(GetSyncStatusMutex());
    auto retFlag = SetSystemParameter(SMART_DATA_CLEAN_STATE, stateValue);
    if (!retFlag) {
        MEDIA_ERR_LOG("SetSmartDataCleanState failed. state: %{public}d", stateValue);
        return;
    }
    MEDIA_INFO_LOG("SetSmartDataCleanState successful. state: %{public}d", stateValue);
}

int64_t GetSmartDataCleanState()
{
    int64_t defaultState = static_cast<int64_t>(CleanTaskState::IDLE);
    std::lock_guard<std::mutex> lock(GetSyncStatusMutex());
    return static_cast<int64_t>(system::GetIntParameter(SMART_DATA_CLEAN_STATE, defaultState));
}

void SetSmartDataUpdateState(UpdateSmartDataState currentState)
{
    int64_t stateValue = static_cast<int64_t>(currentState);
    std::lock_guard<std::mutex> lock(GetSyncStatusMutex());
    auto retFlag = SetSystemParameter(SMART_DATA_UPDATE_STATE, stateValue);
    if (!retFlag) {
        MEDIA_ERR_LOG("SetSmartDataUpdateState failed. state: %{public}" PRId64, stateValue);
        return;
    }
    MEDIA_INFO_LOG("SetSmartDataUpdateState successful. state: %{public}" PRId64, stateValue);
}

int64_t GetSmartDataUpdateState()
{
    int64_t defaultState = static_cast<int64_t>(UpdateSmartDataState::IDLE);
    std::lock_guard<std::mutex> lock(GetSyncStatusMutex());
    return static_cast<int64_t>(system::GetIntParameter(SMART_DATA_UPDATE_STATE, defaultState));
}

// 获取云退出时场景
SmartDataProcessingMode GetSmartDataProcessingMode(CloudMediaRetainType retainType, SwitchStatus switchStatus)
{
    if (retainType == CloudMediaRetainType::RETAIN_FORCE && switchStatus == SwitchStatus::HDC) {
        return SmartDataProcessingMode::RETAIN;
    }  else if (retainType == CloudMediaRetainType::RETAIN_FORCE && switchStatus == SwitchStatus::CLOUD) {
        return SmartDataProcessingMode::RECOVER;
    } else if (retainType == CloudMediaRetainType::HDC_RETAIN_FORCE && switchStatus == SwitchStatus::CLOUD) {
        return SmartDataProcessingMode::RECOVER;
    } else if (retainType == CloudMediaRetainType::HDC_RETAIN_FORCE && switchStatus == SwitchStatus::HDC) {
        return SmartDataProcessingMode::RETAIN;
    } else {
        return SmartDataProcessingMode::NONE;
    }
}

void SetSouthDeviceNextStatus(CloudMediaRetainType retainType, SwitchStatus switchStatus)
{
    auto switchStatusToInt = static_cast<int32_t>(switchStatus);
    auto retainTypeToInt = static_cast<int32_t>(retainType);
    std::lock_guard<std::mutex> lock(GetSyncStatusMutex());
    bool retFlag = false;
    if (retainType == CloudMediaRetainType::RETAIN_FORCE) {
        retFlag = SetSystemParameter(CLOUD_RETIAN_LAST_STATUS_KEY, switchStatusToInt);
    } else if (retainType == CloudMediaRetainType::HDC_RETAIN_FORCE) {
        retFlag = SetSystemParameter(HDC_RETIAN_LAST_STATUS_KEY, switchStatusToInt);
    } else {
        MEDIA_ERR_LOG("SetSouthDeviceNextStatus retainType: %{public}d, status: %{public}d",
            retainTypeToInt, switchStatusToInt);
        return;
    }
    MEDIA_INFO_LOG("set retainType: %{public}d, SwitchStatus: %{public}d, result bool: %{public}d",
        retainTypeToInt, switchStatusToInt, retFlag);
}

SwitchStatus GetSouthDeviceNextStatus(CloudMediaRetainType retainType)
{
    int32_t switchStatus = static_cast<int32_t>(SwitchStatus::NONE);
    auto retainTypeToInt = static_cast<int32_t>(retainType);
    std::lock_guard<std::mutex> lock(GetSyncStatusMutex());
    if (retainType == CloudMediaRetainType::RETAIN_FORCE) {
        switchStatus = system::GetIntParameter(CLOUD_RETIAN_LAST_STATUS_KEY, switchStatus);
    } else if (retainType == CloudMediaRetainType::HDC_RETAIN_FORCE) {
        switchStatus = system::GetIntParameter(HDC_RETIAN_LAST_STATUS_KEY, switchStatus);
    } else {
        MEDIA_ERR_LOG("GetSouthDeviceNextStatus invalid retainType: %{public}d", retainTypeToInt);
        return SwitchStatus::NONE;
    }
    MEDIA_INFO_LOG("GetSouthDeviceNextStatus retainType: %{public}d, SwitchStatus: %{public}d",
        retainTypeToInt, switchStatus);
    return static_cast<SwitchStatus>(switchStatus);
}

void SetSmartDataProcessingMode(SmartDataProcessingMode mode)
{
    int32_t modeToInt = static_cast<int32_t>(mode);
    std::lock_guard<std::mutex> lock(GetSyncStatusMutex());
    auto retFlag = SetSystemParameter(SMART_DATA_PROCESSING_MODE, modeToInt);
    if (!retFlag) {
        MEDIA_ERR_LOG("SetSmartDataProcessingMode failed. mode: %{public}d", modeToInt);
        return;
    }
    MEDIA_INFO_LOG("SetSmartDataProcessingMode mode: %{public}d", modeToInt);
    return;
}

void SetSmartDataRetainTime()
{
    int64_t retainTime = MediaFileUtils::UTCTimeMilliSeconds();
    std::lock_guard<std::mutex> lock(GetSyncStatusMutex());
    auto retFlag = SetSystemParameter(SMART_DATA_RETAIN_TIME, retainTime);
    if (!retFlag) {
        MEDIA_ERR_LOG("SetSmartDataretainTime failed. mode: %{public}" PRId64, retainTime);
        return;
    }
    MEDIA_INFO_LOG("SetSmartRetainTime: %{public}" PRId64, retainTime);
    return;
}

int64_t GetSmartDataRetainTime()
{
    int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t retainTime {0};
    std::lock_guard<std::mutex> lock(GetSyncStatusMutex());
    retainTime = system::GetIntParameter(SMART_DATA_RETAIN_TIME, currentTime);
    return retainTime;
}

SmartDataProcessingMode GetSmartDataProcessingMode()
{
    int32_t modeToInt = static_cast<int32_t>(SmartDataProcessingMode::NONE);
    std::lock_guard<std::mutex> lock(GetSyncStatusMutex());
    modeToInt = system::GetIntParameter(SMART_DATA_PROCESSING_MODE, modeToInt);

    return static_cast<SmartDataProcessingMode>(modeToInt);
}
static const  std::string CREATE_TABLE_BACKUP_ALBUM_SQL = "CREATE TABLE "
    "IF NOT EXISTS PhotosAlbumBackupForSaveAnalysisData ("
        " album_id    INTEGER PRIMARY KEY,"
        " lpath       TEXT    DEFAULT NULL"
    ");";

static const  std::string EMPTY_BACKUP_ALBUM_FOR_SMART_DATA_SQL =
    "delete from PhotosAlbumBackupForSaveAnalysisData "
    "where album_id not in (select distinct owner_album_id from Photos where clean_flag = 1);";

void InitBackupPhotosAlbumTable()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "BackupPhotosAlbumTable failed. rdbStore is null.");
    auto ret = rdbStore->ExecuteSql(CREATE_TABLE_BACKUP_ALBUM_SQL);
    CHECK_AND_RETURN_LOG(ret == E_OK, "Create backup table failed, ret: %{public}d", ret);
}

void BackupBackupPhotosAlbumTable()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "BackupPhotosAlbumTable failed. rdbStore is null.");
    auto ret = rdbStore->ExecuteSql(EMPTY_BACKUP_ALBUM_FOR_SMART_DATA_SQL);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("delete from PhotosAlbumBackupForSaveAnalysisData, ret: %{public}d", ret);
    }
    const std::string backupSql = "insert or ignore into PhotosAlbumBackupForSaveAnalysisData(album_id, lpath) "
        "select album_id, lpath from PhotoAlbum;";
    ret = rdbStore->ExecuteSql(backupSql);
    CHECK_AND_RETURN_LOG(ret == E_OK, "backup table failed, ret: %{public}d", ret);
}

void DeleteBackupPhotosAlbumForSmartData()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "BackupPhotosAlbumTable failed. rdbStore is null.");
    auto ret = rdbStore->ExecuteSql(EMPTY_BACKUP_ALBUM_FOR_SMART_DATA_SQL);
    CHECK_AND_RETURN_LOG(ret == E_OK, "delete backup table record failed, ret: %{public}d", ret);
}

void UpdateInvalidCloudHighlightInfo()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "UpdateInvalidCloudHighlightInfo Failed to get rdbStore.");
    
    // 更新非前台推送和删除的时刻状态为-4，智慧分析会清理时刻状态为-4的数据
    std::string updateSql = "\
        UPDATE tab_highlight_album \
            SET highlight_status = -4 \
        WHERE highlight_status != 1 \
            AND highlight_status != -3 ";
    int32_t ret = rdbStore->ExecuteSql(updateSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Update invalid highlight cluster err %{public}d", ret);
    }

    // 更新时刻封面状态为1，智慧分析根据状态位刷新时刻封面
    std::string updateHighlightCoverSql = "\
        UPDATE tab_highlight_cover_info \
            SET status = 1 ";
    int32_t highlighCoverRet = rdbStore->ExecuteSql(updateHighlightCoverSql);
    if (highlighCoverRet != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Update invalid highlight cover err %{public}d", highlighCoverRet);
    }
}

bool IsNeedRecoverSmartData()
{
    return GetSmartDataProcessingMode() == SmartDataProcessingMode::RECOVER;
}

static void CleanBackupAlbumData(std::shared_ptr<MediaLibraryRdbStore> &rdbStore)
{
    std::string deleteRetainAlbum = "DELETE FROM PhotosAlbumBackupForSaveAnalysisData";
    int32_t ret = rdbStore->ExecuteSql(deleteRetainAlbum);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Clean retain album failed, ret: %{public}d", ret);
    }
}

static std::vector<int64_t> GetNextBatchOfPhotoIdsToClean(std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
    const int32_t limit)
{
    std::vector<int64_t> idsToDelete;
    std::string selectSql = "SELECT " + MediaColumn::MEDIA_ID + " FROM " + PhotoColumn::PHOTOS_TABLE +
                            " WHERE clean_flag = 1 AND cloud_id IS NULL AND south_device_type = 1"
                            " AND real_lcd_visit_time = -3 LIMIT " + std::to_string(limit);

    auto resultSet = rdbStore->QuerySql(selectSql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query ids to delete failed.");
        return idsToDelete;
    }

    int32_t rowCount = 0;
    resultSet->GetRowCount(rowCount);
    if (rowCount > 0) {
        idsToDelete.reserve(rowCount);
        for (int32_t i = 0; i < rowCount; i++) {
            resultSet->GoToRow(i);
            int64_t fileId;
            resultSet->GetLong(0, fileId);
            idsToDelete.push_back(fileId);
        }
    }
    resultSet->Close();
    return idsToDelete;
}

static int32_t BatchDeletePhotosByIds(std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
    const std::vector<int64_t>& idsToDelete)
{
    if (idsToDelete.empty()) {
        return E_OK;
    }

    std::stringstream deleteSql;
    deleteSql << "DELETE FROM " << PhotoColumn::PHOTOS_TABLE << " WHERE " << MediaColumn::MEDIA_ID << " IN (";
    for (size_t i = 0; i < idsToDelete.size(); ++i) {
        deleteSql << idsToDelete[i] << (i == idsToDelete.size() - 1 ? "" : ",");
    }
    deleteSql << ")";

    return rdbStore->ExecuteSql(deleteSql.str());
}

static void CleanPhotosTableCloudDataAsync(AsyncTaskData *data)
{
    MEDIA_INFO_LOG("Begin CleanPhotosTableCloudDataAsync");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("CleanPhotosTable failed. rdbStore is null.");
        SetSmartDataCleanState(CleanTaskState::IDLE);
        return;
    }

    CleanBackupAlbumData(rdbStore);
    constexpr int32_t BATCH_DELETE_LIMIT = 300;
    constexpr int32_t SLEEP_FOR_CLEAN_MS = 200;
    SetSmartDataCleanState(CleanTaskState::CLEANING);
    while (GetSmartDataCleanState() > static_cast<int64_t>(CleanTaskState::IDLE)) {
        std::vector<int64_t> idsToDelete = GetNextBatchOfPhotoIdsToClean(rdbStore, BATCH_DELETE_LIMIT);

        if (idsToDelete.empty()) {
            MEDIA_INFO_LOG("No more photos to clean. Task finished.");
            break;
        }

        int32_t ret = BatchDeletePhotosByIds(rdbStore, idsToDelete);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("Batch clean photos table failed, ret: %{public}d", ret);
            break;
        }

        MEDIA_INFO_LOG("Successfully cleaned a batch of %{public}zu photos.", idsToDelete.size());
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_FOR_CLEAN_MS));
    }
    SetSmartDataRetainTime();
    SetSmartDataCleanState(CleanTaskState::IDLE);
    MEDIA_INFO_LOG("CleanPhotosTableCloudDataAsync: Finished or interrupted.");
}

int32_t DoCleanPhotosTableCloudData()
{
    MEDIA_INFO_LOG("Begin DoCleanPhotosTableCloudData");
    auto asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    if (asyncWorker == nullptr) {
        MEDIA_ERR_LOG("Failed to get async worker instance!");
        return E_ERR;
    }

    std::shared_ptr<MediaLibraryAsyncTask> cleanTask =
        std::make_shared<MediaLibraryAsyncTask>(CleanPhotosTableCloudDataAsync, nullptr);
    if (cleanTask == nullptr) {
        MEDIA_ERR_LOG("Failed to create async task for CleanPhotosTableCloudDataAsync!");
        return E_ERR;
    }

    asyncWorker->AddTask(cleanTask, false);
    MEDIA_INFO_LOG("Successfully scheduled CleanPhotosTableCloudDataAsync task.");
    return E_OK;
}

int32_t DoCloudMediaRetainCleanup()
{
    int32_t ret = E_OK;
    int64_t lastRetainTime = GetSmartDataRetainTime();
    if (lastRetainTime <= 0) {
        return ret;
    }

    int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t timeInterval = (IsNeedRecoverSmartData() ?
        TIMESTAMP_UP_TO_LAST_RETAIN_OF_CLOUD : TIMESTAMP_UP_TO_LAST_RETAIN_OF_HDC);
    if ((currentTime - lastRetainTime) < timeInterval) {
        return ret;
    }

    ret = DoCleanPhotosTableCloudData();
    CHECK_AND_PRINT_LOG(ret == E_OK, "Failed to schedule DoCleanPhotosTableCloudData task");
    return ret;
}

int32_t UpdatePhotosLcdVisitTime(const std::vector<std::string> &fileIds)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "UpdateBatchRealLcdVisitTime failed. rdbStore is null");
    NativeRdb::RdbPredicates updatePredicates = NativeRdb::RdbPredicates(PhotoColumn::PHOTOS_TABLE);
    updatePredicates.In(MediaColumn::MEDIA_ID, fileIds);
    NativeRdb::ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, REAL_LCD_VISIT_TIME_DELETED);
    int32_t updatedRows = 0;
    int32_t ret = rdbStore->Update(updatedRows, values, updatePredicates);
    if (ret != NativeRdb::E_OK || updatedRows <= 0) {
        MEDIA_ERR_LOG("Update db operation failed. ret %{public}d. Updated %{public}d", ret, updatedRows);
        return E_ERR;
    }
    MEDIA_INFO_LOG("Update db operation successful. ret %{public}d. Updated %{public}d", ret, updatedRows);
    return E_OK;
}

static void UpdateSmartDataAlbumAsync(AsyncTaskData *data)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is null.");
        SetSmartDataUpdateState(UpdateSmartDataState::IDLE);
        return;
    }

    MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(rdbStore);
}

int32_t DoUpdateSmartDataAlbum()
{
    MEDIA_INFO_LOG("Begin UpdateSmartDataAlbumAsync");
    auto asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    if (asyncWorker == nullptr) {
        MEDIA_ERR_LOG("Failed to get async worker instance!");
        return E_ERR;
    }

    std::shared_ptr<MediaLibraryAsyncTask> updateTask =
        std::make_shared<MediaLibraryAsyncTask>(UpdateSmartDataAlbumAsync, nullptr);
    if (updateTask == nullptr) {
        MEDIA_ERR_LOG("Failed to create async task for DoUpdateSmartDataAlbum!");
        return E_ERR;
    }

    if (GetSmartDataUpdateState() > static_cast<int64_t>(UpdateSmartDataState::IDLE)) {
        asyncWorker->AddTask(updateTask, false);
    }

    MEDIA_INFO_LOG("Successfully scheduled DoUpdateSmartDataAlbum task.");
    return E_OK;
}

}