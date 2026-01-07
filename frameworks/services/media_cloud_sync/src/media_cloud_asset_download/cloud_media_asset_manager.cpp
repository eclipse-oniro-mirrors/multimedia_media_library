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

#define MLOG_TAG "CloudMediaAssetManager"

#include "cloud_media_asset_manager.h"
#include <cinttypes>
#include "album_accurate_refresh.h"
#include "asset_accurate_refresh.h"
#include "cloud_sync_notify_handler.h"
#include "cloud_sync_helper.h"
#include "media_analysis_helper.h"
#include "medialibrary_album_fusion_utils.h"
#include "medialibrary_asset_operations.h"
#ifdef META_RECOVERY_SUPPORT
#include "medialibrary_meta_recovery.h"
#endif
#include "medialibrary_tracer.h"
#include "medialibrary_notify.h"
#include "parameters.h"
#include "rdb_utils.h"
#include "result_set_utils.h"
#include "thumbnail_service.h"
#include "dfx_const.h"
#include "medialibrary_subscriber.h"
#include "background_cloud_batch_selected_file_processor.h"
#include "settings_data_manager.h"
#include "media_album_order_back.h"
#include "settings_data_manager.h"
#include "cloud_media_retain_smart_data.h"
#include "cloud_media_sync_mutex.h"
#include "medialibrary_bundle_manager.h"
#include "hi_audit.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
using namespace FileManagement::CloudSync;
using namespace OHOS::DataShare;
using namespace OHOS::RdbDataShareAdapter;

static const std::string UNKNOWN_VALUE = "NA";
// batch limit count of update cloud data
constexpr int32_t BATCH_UPDATE_LIMIT_COUNT = 500;
// batch limit count of delete cloud data
constexpr int32_t BATCH_DELETE_LIMIT_COUNT = 300;
static const int32_t CYCLE_NUMBER = 5000;
static const int32_t SLEEP_FOR_DELETE = 600;
static const int32_t BATCH_NOTIFY_CLOUD_FILE = 2000;
static const std::string DELETE_DISPLAY_NAME = "cloud_media_asset_deleted";
static const int32_t ALBUM_FROM_CLOUD = 2;
static const int32_t ALBUM_IS_CLOUD = 2;
static const int32_t ZERO_ASSET_OF_ALBUM = 0;
static const int32_t MAX_BATCH_DOWNLOAD_TASK_SIZE = 500;
const std::string START_QUERY_ZERO = "0";

const std::string SQL_CONDITION_EMPTY_CLOUD_ALBUMS = "FROM " + PhotoAlbumColumns::TABLE + " WHERE " +
    "( " + PhotoAlbumColumns::ALBUM_IS_LOCAL + " = " + to_string(ALBUM_FROM_CLOUD) + " AND " +
    PhotoAlbumColumns::ALBUM_ID + " NOT IN ( " +
        "SELECT DISTINCT " + PhotoColumn::PHOTO_OWNER_ALBUM_ID +
        " FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " +
        PhotoColumn::PHOTO_CLEAN_FLAG + " = " + to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN)) + " ))" +
    " OR " + PhotoAlbumColumns::ALBUM_DIRTY + " = " + to_string(static_cast<int32_t>(DirtyType::TYPE_DELETED));
const std::string SQL_QUERY_EMPTY_CLOUD_ALBUMS = "SELECT * " + SQL_CONDITION_EMPTY_CLOUD_ALBUMS;
const std::string SQL_DELETE_EMPTY_CLOUD_ALBUMS = "DELETE " + SQL_CONDITION_EMPTY_CLOUD_ALBUMS;

const std::string SQL_DELETE_ALL_ALBUM_ORDER_BACK =
    "DELETE FROM " + ALBUM_ORDER_BACK_TABLE;

// 持久化清除状态, 防止重启后清除错误服务端的图片数据: 时间戳|0
static const std::string CLOUD_RETIAN_STATUS_KEY = "persist.multimedia.medialibrary.retain.cloud.status";
static const std::string HDC_RETIAN_STATUS_KEY = "persist.multimedia.medialibrary.retain.hdc.status";

static const std::string MEIDA_RESTORE_FLAG = "multimedia.medialibrary.restoreFlag";
static const std::string MEIDA_BACKUP_FLAG = "multimedia.medialibrary.backupFlag";

// 同步服务模块使用: 时间戳|0，时间戳表征任务正在清理中，0表示无清理任务
static const std::string CLOUDSYNC_SWITCH_STATUS_KEY = "persist.kernel.cloudsync.switch_status"; // ms

// 超时时间 ForceRetainDownloadCloudMedia
static constexpr int64_t SOUTH_DEVICE_CLEAN_DATA_TIMEOUT_MILLISECOND = 12 * 60 * 60 * 1000;
static constexpr int64_t FORCE_RETAIN_CLOUD_MEDIA_WAIT_RESTORE_TIMEOUT_SECOND = 6 * 60 * 60;
static constexpr int64_t FORCE_RETAIN_CLOUD_MEDIA_WAIT_BACKUP_TIMEOUT_SECOND = 2 * 60 * 60;
static constexpr int64_t FORCE_RETAIN_CLOUD_MEDIA_WAIT_BACKUP_OR_RESTORE_SLEEP_TIME_MILLISECOND = 15000;

static bool SetSystemParameter(const std::string& key, int64_t value)
{
    std::string valueStr = std::to_string(value);
    return system::SetParameter(key, valueStr);
}

static void SetSouthDeviceSyncSwitchStatus(CloudSyncStatus status)
{
    std::lock_guard<std::mutex> lock(GetSyncStatusMutex());
    bool retFlag = false;
    if (status == CloudSyncStatus::CLOUD_CLEANING) {
        auto timeStamp = MediaFileUtils::UTCTimeMilliSeconds();
        retFlag = SetSystemParameter(CLOUDSYNC_SWITCH_STATUS_KEY, timeStamp);
    } else {
        retFlag = SetSystemParameter(CLOUDSYNC_SWITCH_STATUS_KEY, 0);
    }
    MEDIA_INFO_LOG("set CloudSyncStatus: %{public}d, result bool: %{public}d", static_cast<int32_t>(status), retFlag);
}

static void SetSouthDeviceCleanStatus(CloudMediaRetainType retainType, CloudSyncStatus statusKey)
{
    auto retainTypeToInt = static_cast<int32_t>(retainType);
    // 防止一直无法恢复, 使用时间戳代替开关
    int64_t timeStamp = 0;
    if (statusKey ==  CloudSyncStatus::CLOUD_CLEANING) {
        timeStamp = MediaFileUtils::UTCTimeMilliSeconds();
    }

    std::lock_guard<std::mutex> lock(GetSyncStatusMutex());
    bool retFlag = false;
    if (retainType == CloudMediaRetainType::RETAIN_FORCE) {
        retFlag = SetSystemParameter(CLOUD_RETIAN_STATUS_KEY, timeStamp);
    } else if (retainType == CloudMediaRetainType::HDC_RETAIN_FORCE) {
        retFlag = SetSystemParameter(HDC_RETIAN_STATUS_KEY, timeStamp);
    } else {
        MEDIA_ERR_LOG("invalid retainType: %{public}d, status: %{public}d", retainTypeToInt, statusKey);
        return;
    }
    MEDIA_INFO_LOG("set retainType: %{public}d, status: %{public}d, result bool: %{public}d",
        retainTypeToInt, statusKey, retFlag);
}

static bool IsSouthDeviceSyncCleaning(CloudMediaRetainType retainType, bool checkTimeout = true)
{
    std::lock_guard<std::mutex> lock(GetSyncStatusMutex());
    int64_t timeStamp = 0;
    if (retainType == CloudMediaRetainType::RETAIN_FORCE) {
        timeStamp = system::GetIntParameter(CLOUD_RETIAN_STATUS_KEY, timeStamp);
    } else if (retainType == CloudMediaRetainType::HDC_RETAIN_FORCE) {
        timeStamp = system::GetIntParameter(HDC_RETIAN_STATUS_KEY, timeStamp);
    } else {
        MEDIA_ERR_LOG("get invalid retainType: %{public}d", static_cast<int32_t>(retainType));
    }
    if (timeStamp == 0) {
        return false;
    }

    if (!checkTimeout) {
        return true;
    }

    auto nowTime = MediaFileUtils::UTCTimeSeconds();
    return ((nowTime - timeStamp) < SOUTH_DEVICE_CLEAN_DATA_TIMEOUT_MILLISECOND);
}

static void WaitIfBackUpingOrRestoring(const std::string& key, int64_t waitTimeout, const std::string& info = "unknown")
{
    constexpr int64_t defaultValueTime = 0;
    int64_t startTimeClone = system::GetIntParameter(key, defaultValueTime);
    int64_t startTimeWait = MediaFileUtils::UTCTimeSeconds();
    MEDIA_INFO_LOG("Wait for %{public}s to exit. startTimeWait: %{public}" PRId64
        ", startTimeClone: %{public}" PRId64, info.c_str(), startTimeWait, startTimeClone);
    while (startTimeClone > 0) {
        auto nowTime = MediaFileUtils::UTCTimeSeconds();
        if ((nowTime - startTimeWait) > waitTimeout) {
            MEDIA_WARN_LOG("[%{public}s] timeout: now: %{public}" PRId64", startTimeWait: %{public}" PRId64
                ", startTimeClone: %{public}" PRId64, info.c_str(), nowTime, startTimeWait, startTimeClone);
            break;
        }
        MEDIA_DEBUG_LOG("[%{public}s] waiting: now: %{public}" PRId64", startTimeWait: %{public}" PRId64
            ", startTimeClone: %{public}" PRId64, info.c_str(), nowTime, startTimeWait, startTimeClone);
        std::this_thread::sleep_for(chrono::milliseconds(
            FORCE_RETAIN_CLOUD_MEDIA_WAIT_BACKUP_OR_RESTORE_SLEEP_TIME_MILLISECOND));
        startTimeClone = system::GetIntParameter(key, defaultValueTime);
    }

    MEDIA_INFO_LOG("the %{public}s has exited, currtime: %{public}" PRId64,
        info.c_str(), MediaFileUtils::UTCTimeSeconds());
}

static void WaitIfBackUpingOrRestoring()
{
    WaitIfBackUpingOrRestoring(MEIDA_RESTORE_FLAG,
        FORCE_RETAIN_CLOUD_MEDIA_WAIT_RESTORE_TIMEOUT_SECOND, "db restore");

    WaitIfBackUpingOrRestoring(MEIDA_BACKUP_FLAG,
        FORCE_RETAIN_CLOUD_MEDIA_WAIT_BACKUP_TIMEOUT_SECOND, "db backup");
}

CloudMediaAssetManager& CloudMediaAssetManager::GetInstance()
{
    static CloudMediaAssetManager instance;
    return instance;
}

int32_t CloudMediaAssetManager::CheckDownloadTypeOfTask(const CloudMediaDownloadType &type)
{
    if (static_cast<int32_t>(type) < static_cast<int32_t>(CloudMediaDownloadType::DOWNLOAD_FORCE) ||
        static_cast<int32_t>(type) > static_cast<int32_t>(CloudMediaDownloadType::DOWNLOAD_GENTLE)) {
        MEDIA_ERR_LOG("Invalid CloudMediaDownloadType. but received: %{public}d", static_cast<int32_t>(type));
        return E_ERR;
    }
    return E_OK;
}

int32_t CloudMediaAssetManager::StartDownloadCloudAsset(const CloudMediaDownloadType &type)
{
    HiAudit::GetInstance().WriteForCloudDownload(
        MediaLibraryBundleManager::GetInstance()->GetClientBundleName(), static_cast<int32_t>(type), "start");
    if (operation_ == nullptr) {
        CloudMediaAssetDownloadOperation taskOperator;
        operation_ = taskOperator.GetInstance();
    }
    if (CheckDownloadTypeOfTask(type) != E_OK) {
        return E_ERR;
    }
    operation_->ResetDownloadTryTime();
    switch (operation_->GetTaskStatus()) {
        case CloudMediaAssetTaskStatus::IDLE: {
            return operation_->StartDownloadTask(static_cast<int32_t>(type));
        }
        case CloudMediaAssetTaskStatus::PAUSED: {
            return operation_->ManualActiveRecoverTask(static_cast<int32_t>(type));
        }
        case CloudMediaAssetTaskStatus::DOWNLOADING: {
            if (type == operation_->GetDownloadType()) {
                MEDIA_WARN_LOG("No status changed.");
                return E_OK;
            }
            if (type == CloudMediaDownloadType::DOWNLOAD_GENTLE) {
                return operation_->PauseDownloadTask(CloudMediaTaskPauseCause::BACKGROUND_TASK_UNAVAILABLE);
            }
            return E_ERR;
        }
        default: {
            MEDIA_ERR_LOG("StartDownloadCloudAsset failed. now: taskStatus_, %{public}d; \
                downloadType_, %{public}d. input: type, %{public}d;",
                static_cast<int32_t>(operation_->GetTaskStatus()), static_cast<int32_t>(operation_->GetDownloadType()),
                static_cast<int32_t>(type));
            return E_ERR;
        }
    }
}

int32_t CloudMediaAssetManager::RecoverDownloadCloudAsset(const CloudMediaTaskRecoverCause &cause)
{
    bool cond = (operation_ == nullptr || operation_->GetTaskStatus() == CloudMediaAssetTaskStatus::IDLE);
    CHECK_AND_RETURN_RET(!cond, E_ERR);

    CHECK_AND_EXECUTE(cause != CloudMediaTaskRecoverCause::NETWORK_NORMAL, operation_->ResetDownloadTryTime());
    MEDIA_INFO_LOG("enter RecoverDownloadCloudAsset, RecoverCause: %{public}d", static_cast<int32_t>(cause));
    CHECK_AND_RETURN_RET_LOG(operation_->GetTaskStatus() != CloudMediaAssetTaskStatus::DOWNLOADING, E_OK,
        "The task status is download, no need to recover.");
    int32_t ret = operation_->PassiveStatusRecoverTask(cause);
    MEDIA_INFO_LOG("end to RecoverDownloadCloudAsset, status: %{public}s, ret: %{public}d.",
        GetCloudMediaAssetTaskStatus().c_str(), ret);
    return ret;
}

void CloudMediaAssetManager::CheckStorageAndRecoverDownloadTask()
{
    bool cond = (operation_ == nullptr || operation_->GetTaskStatus() != CloudMediaAssetTaskStatus::PAUSED ||
        operation_->GetTaskPauseCause() != CloudMediaTaskPauseCause::ROM_LIMIT);
    CHECK_AND_RETURN(!cond);
    MEDIA_INFO_LOG("begin to check storage and recover downloadTask.");
    operation_->CheckStorageAndRecoverDownloadTask();
}

int32_t CloudMediaAssetManager::PauseDownloadCloudAsset(const CloudMediaTaskPauseCause &pauseCause)
{
    if (operation_ == nullptr || operation_->GetTaskStatus() == CloudMediaAssetTaskStatus::IDLE) {
        MEDIA_INFO_LOG("no need to pause");
        return E_OK;
    }
    int32_t ret = operation_->PauseDownloadTask(pauseCause);
    MEDIA_INFO_LOG("end to PauseDownloadCloudAsset, status: %{public}s, ret: %{public}d.",
        GetCloudMediaAssetTaskStatus().c_str(), ret);
    return ret;
}

int32_t CloudMediaAssetManager::CancelDownloadCloudAsset()
{
    if (operation_ == nullptr || operation_->GetTaskStatus() == CloudMediaAssetTaskStatus::IDLE) {
        MEDIA_INFO_LOG("no need to cancel");
        return E_OK;
    }
    HiAudit::GetInstance().WriteForCloudDownload(
        MediaLibraryBundleManager::GetInstance()->GetClientBundleName(), -1, "cancel");
    int32_t ret = operation_->CancelDownloadTask();
    operation_.reset();
    return ret;
}

void CloudMediaAssetManager::StartDeleteCloudMediaAssets()
{
    TaskDeleteState expect = TaskDeleteState::IDLE;
    if (doDeleteTask_.compare_exchange_strong(expect, TaskDeleteState::BACKGROUND_DELETE)) {
        MEDIA_INFO_LOG("start delete cloud media assets task.");
        DeleteAllCloudMediaAssetsAsync();
    }
}

void CloudMediaAssetManager::StopDeleteCloudMediaAssets()
{
    TaskDeleteState expect = TaskDeleteState::BACKGROUND_DELETE;
    if (!doDeleteTask_.compare_exchange_strong(expect, TaskDeleteState::IDLE)) {
        MEDIA_INFO_LOG("current status is not suitable for stop delete cloud media assets task.");
    }
}

int32_t CloudMediaAssetManager::DeleteBatchCloudFile(const std::vector<std::string> &fileIds)
{
    MediaLibraryTracer tracer;
    tracer.Start("DeleteBatchCloudFile");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "DeleteBatchCloudFile failed. rdbStore is null");
    AbsRdbPredicates deletePredicates(PhotoColumn::PHOTOS_TABLE);
    deletePredicates.In(MediaColumn::MEDIA_ID, fileIds);
    int32_t deletedRows = E_HAS_DB_ERROR;
    int32_t ret = rdbStore->Delete(deletedRows, deletePredicates);
    if (ret != NativeRdb::E_OK || deletedRows <= 0) {
        MEDIA_ERR_LOG("Delete db operation failed. ret %{public}d. Deleted %{public}d", ret, deletedRows);
        return E_ERR;
    }
    MEDIA_INFO_LOG("Delete db operation successful. ret %{public}d. Deleted %{public}d", ret, deletedRows);
    return E_OK;
}

int32_t CloudMediaAssetManager::ReadyDataForDelete(std::vector<std::string> &fileIds, std::vector<std::string> &paths,
    std::vector<std::string> &dateTakens, std::vector<int64_t>& lcdVisitTimes, std::vector<int32_t> &subTypes)
{
    MediaLibraryTracer tracer;
    tracer.Start("ReadyDataForDelete");
    MEDIA_INFO_LOG("enter ReadyDataForDelete");
    AbsRdbPredicates queryPredicates(PhotoColumn::PHOTOS_TABLE);
    // don't care about smart data processing mode
    queryPredicates.EqualTo(MediaColumn::MEDIA_NAME, DELETE_DISPLAY_NAME);
    queryPredicates.Or();
    queryPredicates.BeginWrap();
        queryPredicates.EqualTo(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_NEED_CLEAN));
        queryPredicates.EqualTo(PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, REAL_LCD_VISIT_TIME_INVALID);
        queryPredicates.EqualTo(PhotoColumn::PHOTO_POSITION,
            to_string(static_cast<int32_t>(PhotoPositionType::CLOUD)));
    queryPredicates.EndWrap();

    queryPredicates.Limit(BATCH_DELETE_LIMIT_COUNT);
    vector<string> columns = {MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH, MediaColumn::MEDIA_DATE_TAKEN,
        PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME,
        PhotoColumn::PHOTO_SUBTYPE};

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "ReadyDataForDelete failed. rdbStorePtr is null");
    auto resultSet = rdbStore->Query(queryPredicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "ReadyDataForDelete failed. resultSet is null");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        string path = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        if (path.empty()) {
            MEDIA_WARN_LOG("Failed to get path!");
            continue;
        }
        MEDIA_DEBUG_LOG("get path: %{public}s.", MediaFileUtils::DesensitizePath(path).c_str());
        fileIds.emplace_back(GetStringVal(MediaColumn::MEDIA_ID, resultSet));
        paths.emplace_back(path);
        dateTakens.emplace_back(GetStringVal(MediaColumn::MEDIA_DATE_TAKEN, resultSet));
        lcdVisitTimes.emplace_back(GetInt64Val(PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, resultSet));
        subTypes.emplace_back(GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet));
    }
    resultSet->Close();
    return E_OK;
}

std::string CloudMediaAssetManager::GetEditDataDirPath(const std::string &path)
{
    CHECK_AND_RETURN_RET(path.length() >= ROOT_MEDIA_DIR.length(), "");
    return MEDIA_EDIT_DATA_DIR + path.substr(ROOT_MEDIA_DIR.length());
}

int32_t CloudMediaAssetManager::DeleteEditdata(const std::string &path)
{
    string editDataDirPath = GetEditDataDirPath(path);
    CHECK_AND_RETURN_RET_LOG(!editDataDirPath.empty(), E_ERR, "Cannot get editPath, path: %{private}s", path.c_str());
    if (MediaFileUtils::IsFileExists(editDataDirPath)) {
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::DeleteDir(editDataDirPath), E_ERR,
            "Failed to delete edit data, path: %{private}s", editDataDirPath.c_str());
    }
    return E_OK;
}

void CloudMediaAssetManager::CleanUpFileData(const std::string& path)
{
    CHECK_AND_PRINT_LOG(DeleteEditdata(path) == OHOS::Media::E_OK, "DeleteEditdata error.");
#ifdef META_RECOVERY_SUPPORT
    CHECK_AND_PRINT_LOG(MediaLibraryMetaRecovery::DeleteMetaDataByPath(path) == E_OK,
        "DeleteMetaDataByPath error.");
#endif
    CloudSyncManager::GetInstance().CleanGalleryDentryFile(path);
}

bool CloudMediaAssetManager::ProcessDeleteBatch(const std::vector<std::string> &fileIds,
    const std::vector<std::string> &paths, const std::vector<std::string> &dateTakens,
    const std::vector<int64_t> &lcdVisitTimes, std::vector<int32_t> &subTypes)
{
    std::vector<std::string> deleteDbFileIds;
    std::vector<std::string> updateDbFileIds;

    for (size_t i = 0; i < fileIds.size(); i++) {
        if (lcdVisitTimes[i] == REAL_LCD_VISIT_TIME_INVALID) {
            updateDbFileIds.push_back(fileIds[i]);
        } else if (lcdVisitTimes[i] == REAL_LCD_VISIT_TIME_DELETED) {
            continue; // Skip items already marked as deleted
        } else {
            deleteDbFileIds.push_back(fileIds[i]);
        }
        CleanUpFileData(paths[i]);
    }

    // Perform batch database operations
    if (!deleteDbFileIds.empty()) {
        int32_t ret = DeleteBatchCloudFile(deleteDbFileIds);
        CHECK_AND_RETURN_RET_LOG(ret == OHOS::Media::E_OK, false, "DeleteBatchCloudFile failed!");
    }

    if (!updateDbFileIds.empty()) {
        int32_t ret = UpdatePhotosLcdVisitTime(updateDbFileIds);
        CHECK_AND_RETURN_RET_LOG(ret == OHOS::Media::E_OK, false, "UpdateCloudFile failed!");
    }

    // Delete thumbnails
    MEDIA_INFO_LOG("delete thumb files.");
    CHECK_AND_PRINT_LOG(ThumbnailService::GetInstance()->BatchDeleteThumbnailDirAndAstc(PhotoColumn::PHOTOS_TABLE,
        fileIds, paths, dateTakens), "DeleteThumbnailDirAndAstc error.");
    MediaLibraryAssetOperations::SaveDeletedFile(fileIds, paths, PhotoColumn::PHOTOS_TABLE, dateTakens, subTypes);
    return true;
}

void CloudMediaAssetManager::DeleteAllCloudMediaAssetsOperation(AsyncTaskData *data)
{
    std::lock_guard<std::mutex> lock(deleteMutex_);
    MEDIA_INFO_LOG("enter DeleteAllCloudMediaAssetsOperation");
    MediaLibraryTracer tracer;
    tracer.Start("DeleteAllCloudMediaAssetsOperation");

    std::vector<std::string> fileIds;
    std::vector<std::string> paths;
    std::vector<std::string> dateTakens;
    std::vector<int64_t> lcdVisitTimes;
    std::vector<int32_t> subTypes;

    int32_t cycleNumber = 0;
    while (doDeleteTask_.load() > TaskDeleteState::IDLE && cycleNumber++ <= CYCLE_NUMBER) {
        int32_t ret = ReadyDataForDelete(fileIds, paths, dateTakens, lcdVisitTimes, subTypes);
        if (ret != OHOS::Media::E_OK || fileIds.empty()) {
            MEDIA_WARN_LOG("ReadyDataForDelete failed or no assets left, ret: %{public}d", ret);
            break;
        }

        if (!ProcessDeleteBatch(fileIds, paths, dateTakens, lcdVisitTimes, subTypes)) {
            MEDIA_ERR_LOG("ProcessDeleteBatch failed, exiting operation.");
            break;
        }

        MEDIA_INFO_LOG("Processed batch. loop: %{public}d, asset count: %{public}zu",
            cycleNumber, fileIds.size());

        fileIds.clear();
        paths.clear();
        dateTakens.clear();
        lcdVisitTimes.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_FOR_DELETE));
    }

    doDeleteTask_.store(TaskDeleteState::IDLE);
    MEDIA_INFO_LOG("exit DeleteAllCloudMediaAssetsOperation");
}

void CloudMediaAssetManager::DeleteAllCloudMediaAssetsAsync()
{
    shared_ptr<MediaLibraryAsyncWorker> asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_LOG(asyncWorker != nullptr, "Can not get asyncWorker");

    shared_ptr<MediaLibraryAsyncTask> deleteAsyncTask =
        make_shared<MediaLibraryAsyncTask>(DeleteAllCloudMediaAssetsOperation, nullptr);
    CHECK_AND_RETURN_LOG(deleteAsyncTask != nullptr, "Can not get deleteAsyncTask");

    asyncWorker->AddTask(deleteAsyncTask, true);
}

bool CloudMediaAssetManager::HasDataForUpdate(CloudMediaRetainType retainType,
    std::vector<std::string> &updateFileIds, const std::string &lastFileId, SmartDataProcessingMode mode)
{
    updateFileIds.clear();
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "HasDataForUpdate failed. rdbStore is null.");
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.GreaterThan(MediaColumn::MEDIA_ID, lastFileId);
    predicates.EqualTo(PhotoColumn::PHOTO_POSITION, to_string(static_cast<int32_t>(PhotoPositionType::CLOUD)));
    predicates.NotEqualTo(MediaColumn::MEDIA_NAME, DELETE_DISPLAY_NAME);
    if (mode == SmartDataProcessingMode::RECOVER || mode == SmartDataProcessingMode::RETAIN) {
        // recover and retain just handle not_clean asserts
        predicates.EqualTo(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN));
    }
    predicates.OrderByAsc(MediaColumn::MEDIA_ID);
    predicates.Limit(BATCH_UPDATE_LIMIT_COUNT);
    std::vector<std::string> columns = { MediaColumn::MEDIA_ID };
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "HasDataForUpdate failed. resultSet is null.");

    updateFileIds.reserve(BATCH_UPDATE_LIMIT_COUNT);
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string fileId = GetStringVal(MediaColumn::MEDIA_ID, resultSet);
        updateFileIds.emplace_back(fileId);
    }
    resultSet->Close();
    CHECK_AND_RETURN_RET_LOG(updateFileIds.size() > 0, false, "the size of updateFileIds 0.");
    return true;
}

int32_t CloudMediaAssetManager::UpdateCloudAssets(const std::vector<std::string> &updateFileIds,
    SmartDataProcessingMode mode)
{
    MediaLibraryTracer tracer;
    int32_t modeToInt = static_cast<int32_t>(mode);
    tracer.Start("UpdateCloudAssets" + std::to_string(modeToInt));
    CHECK_AND_RETURN_RET_LOG(!updateFileIds.empty(), E_ERR, "updateFileIds is null.");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "UpdateCloudAssets failed. rdbStore is null.");
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_ID, updateFileIds);

    ValuesBucket values;
    if (mode == SmartDataProcessingMode::RETAIN) {
        values.PutInt(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, 0);
        values.PutInt(PhotoColumn::PHOTO_THUMB_STATUS, static_cast<int32_t>(CloudSync::ThumbState::TO_DOWNLOAD));
        values.PutInt(PhotoColumn::PHOTO_THUMBNAIL_READY, 0);
        values.PutInt(PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, REAL_LCD_VISIT_TIME_INVALID); // specail invalid;
    } else {
        values.PutString(MediaColumn::MEDIA_NAME, DELETE_DISPLAY_NAME);
        values.PutInt(PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, 0);
    }

    values.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_NEED_CLEAN));
    values.PutInt(PhotoColumn::PHOTO_DIRTY, -1);
    values.PutLong(PhotoColumn::PHOTO_CLOUD_VERSION, 0);
    values.PutNull(PhotoColumn::PHOTO_CLOUD_ID);

    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG((ret == OHOS::Media::E_OK && changedRows > 0), E_ERR,
        "Failed to UpdateCloudAssets, ret: %{public}d, updateRows: %{public}d, mode:%{public}d",
        ret, changedRows, modeToInt);
    MEDIA_INFO_LOG("UpdateCloudAssets successfully. ret: %{public}d, updateRows: %{public}d, mode:%{public}d",
        ret, changedRows, modeToInt);
    return OHOS::Media::E_OK;
}

void CloudMediaAssetManager::NotifyUpdateAssetsChange(const std::vector<std::string> &notifyFileIds)
{
    AccurateRefresh::AssetAccurateRefresh::NotifyForReCheck();
    CHECK_AND_RETURN_LOG(!notifyFileIds.empty(), "notifyFileIds is null.");
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "watch is null.");
    for (size_t i = 0; i < notifyFileIds.size(); i++) {
        watch->Notify(MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, notifyFileIds[i]),
            NotifyType::NOTIFY_REMOVE);
    }
}

int32_t CloudMediaAssetManager::UpdateCloudMediaAssets(CloudMediaRetainType retainType, SmartDataProcessingMode mode)
{
    MediaLibraryTracer tracer;
    int32_t modeToInt = static_cast<int32_t>(mode);
    tracer.Start("UpdateCloudMediaAssets" + std::to_string(modeToInt));

    int32_t cycleNumber = 0;
    std::string lastFileId = START_QUERY_ZERO;
    std::vector<std::string> notifyFileIds;
    notifyFileIds.reserve(BATCH_NOTIFY_CLOUD_FILE);
    std::vector<std::string> updateFileIds;
    int32_t actualRet = OHOS::Media::E_OK;
    MEDIA_INFO_LOG("begin UpdateCloudMediaAssets, mode: %{public}d", modeToInt);
    while (HasDataForUpdate(retainType, updateFileIds, lastFileId, mode) && cycleNumber++ <= CYCLE_NUMBER) {
        int32_t ret = UpdateCloudAssets(updateFileIds, mode);
        if (ret != OHOS::Media::E_OK) {
            MEDIA_WARN_LOG("UpdateCloudAssets failed, and try again ret: %{public}d, mode: %{public}d", ret, modeToInt);
            ret = UpdateCloudAssets(updateFileIds, mode);
        }
        if (ret != OHOS::Media::E_OK) {
            MEDIA_ERR_LOG("UpdateCloudAssets failed, ret: %{public}d, mode: %{public}d", ret, modeToInt);
            actualRet = ret;
            continue;
        }
        lastFileId = updateFileIds.back();
        notifyFileIds.insert(notifyFileIds.end(), updateFileIds.begin(), updateFileIds.end());
        if (notifyFileIds.size() >= BATCH_NOTIFY_CLOUD_FILE) {
            NotifyUpdateAssetsChange(notifyFileIds);
            notifyFileIds.clear();
        }
    }
    MEDIA_INFO_LOG("end UpdateCloudMediaAssets, mode: %{public}d", modeToInt);
    if (notifyFileIds.size() > 0) {
        NotifyUpdateAssetsChange(notifyFileIds);
        notifyFileIds.clear();
    }
    if (cycleNumber > 0) {
        MEDIA_INFO_LOG("begin to refresh all albums.");
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_OK, "UpdateAllAlbums failed. rdbStore is null.");
        MediaLibraryRdbUtils::UpdateAllAlbums(rdbStore);
    }
    return actualRet;
}

std::vector<int32_t> CloudMediaAssetManager::QueryEmptyAlbumsAndBackup()
{
    std::vector<int32_t> albumIds;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, albumIds, "QueryEmptyAlbumsAndBackup failed. rdbStore is null.");

    auto resultSet = rdbStore->QueryByStep(SQL_QUERY_EMPTY_CLOUD_ALBUMS);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, albumIds, "QueryEmptyAlbumsAndBackup failed. resultSet is null.");

    std::vector<NativeRdb::ValuesBucket> values;
    int32_t rowCount = 0;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        ValuesBucket value;

        std::string lpath = get<std::string>(
            ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_LPATH, resultSet, TYPE_STRING));
        int32_t order = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUMS_ORDER, resultSet, TYPE_INT32));
        int32_t type = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ORDER_TYPE, resultSet, TYPE_INT32));
        int32_t section = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ORDER_SECTION, resultSet, TYPE_INT32));
        int32_t order2 = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::STYLE2_ALBUMS_ORDER, resultSet, TYPE_INT32));
        int32_t type2 = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::STYLE2_ORDER_TYPE, resultSet, TYPE_INT32));
        int32_t section2 = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::STYLE2_ORDER_SECTION, resultSet, TYPE_INT32));
        int32_t albumId = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_ID, resultSet, TYPE_INT32));

        value.PutString("lpath", lpath);
        value.PutInt("albums_order", order);
        value.PutInt("order_type", type);
        value.PutInt("order_section", section);
        value.PutInt("style2_albums_order", order2);
        value.PutInt("style2_order_type", type2);
        value.PutInt("style2_order_section", section2);

        values.emplace_back(value);
        albumIds.emplace_back(albumId);
        rowCount++;
    }
    resultSet->Close();

    if (rowCount != 0) {
        int64_t insertNum = 0;
        int32_t ret = rdbStore->ExecuteSql(SQL_DELETE_ALL_ALBUM_ORDER_BACK);
        CHECK_AND_PRINT_LOG(ret == E_OK, "delete album_order_back data failed. ret %{public}d.", ret);
        ret = rdbStore->BatchInsert(insertNum, ALBUM_ORDER_BACK_TABLE, values);
        CHECK_AND_PRINT_LOG(ret == E_OK, "insert album_order_back data failed. ret %{public}d.", ret);
    } else {
        MEDIA_INFO_LOG("No data to backup");
    }
    return albumIds;
}

std::string CloudMediaAssetManager::BuildEmptyAlbumsWhereClause(const std::vector<int32_t>& albumIds)
{
    std::string whereClause = PhotoAlbumColumns::ALBUM_ID + " IN (";
    for (size_t i = 0; i < albumIds.size(); ++i) {
        whereClause += std::to_string(albumIds[i]);
        /* Check if current albumId is not the last one */
        if (i != albumIds.size() - 1) {
            whereClause += ",";
        }
    }
    whereClause += ")";

    return whereClause;
}

int32_t CloudMediaAssetManager::DeleteEmptyCloudAlbums()
{
    MEDIA_INFO_LOG("start DeleteEmptyCloudAlbums.");
    MediaLibraryTracer tracer;
    tracer.Start("DeleteEmptyCloudAlbums");

    std::vector<int32_t> albumIds = QueryEmptyAlbumsAndBackup();

    if (albumIds.empty()) {
        MEDIA_INFO_LOG("No empty cloud albums to delete.");
        return E_OK;
    }

    std::string emptyAlbumsWhereClause = BuildEmptyAlbumsWhereClause(albumIds);
    std::string querySql = "SELECT * FROM " + PhotoAlbumColumns::TABLE + " WHERE " + emptyAlbumsWhereClause;
    std::string deleteSql = "DELETE FROM " + PhotoAlbumColumns::TABLE + " WHERE " + emptyAlbumsWhereClause;

    std::shared_ptr<AccurateRefresh::AlbumAccurateRefresh> albumRefresh =
        std::make_shared<AccurateRefresh::AlbumAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(albumRefresh != nullptr, E_ERR, "DeleteEmptyCloudAlbums failed. albumRefresh is null");
    int32_t ret = albumRefresh->Init(querySql, std::vector<NativeRdb::ValueObject>());
    CHECK_AND_PRINT_LOG(ret == AccurateRefresh::ACCURATE_REFRESH_RET_OK, "Failed to init albumRefresh");

    ret = albumRefresh->ExecuteSql(deleteSql, AccurateRefresh::RdbOperation::RDB_OPERATION_REMOVE);
    CHECK_AND_RETURN_RET_LOG(ret == AccurateRefresh::ACCURATE_REFRESH_RET_OK, E_ERR,
        "Failed to delete. ret %{public}d.", ret);
    albumRefresh->Notify();
    MEDIA_INFO_LOG("end DeleteEmptyCloudAlbums. ret %{public}d.", ret);
    return E_OK;
}

bool CloudMediaAssetManager::HasLocalAndCloudAssets(CloudMediaRetainType retainType,
    std::vector<std::string> &updateFileIds, const string &lastFileId)
{
    updateFileIds.clear();
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "HasLocalAndCloudAssets failed. rdbStore is null.");

    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.GreaterThan(MediaColumn::MEDIA_ID, lastFileId);
    predicates.BeginWrap();
    predicates.EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    predicates.Or();
    predicates.BeginWrap();
    predicates.EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL));
    predicates.BeginWrap();
    predicates.IsNotNull(PhotoColumn::PHOTO_CLOUD_ID);
    predicates.Or();
    predicates.NotEqualTo(PhotoColumn::PHOTO_CLOUD_VERSION, 0);
    predicates.Or();
    predicates.BeginWrap();
    predicates.NotEqualTo(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
    predicates.NotEqualTo(PhotoColumn::PHOTO_DIRTY, -1);
    predicates.EndWrap();
    predicates.Or();
    predicates.NotEqualTo(PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE,
        static_cast<int32_t>(SouthDeviceType::SOUTH_DEVICE_NULL));
    predicates.EndWrap();
    predicates.EndWrap();
    predicates.EndWrap();
    predicates.OrderByAsc(MediaColumn::MEDIA_ID);
    predicates.Limit(BATCH_UPDATE_LIMIT_COUNT);

    std::vector<std::string> columns = { MediaColumn::MEDIA_ID };
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false,
        "HasLocalAndCloudAssets failed. resultSet is null.");

    updateFileIds.reserve(BATCH_UPDATE_LIMIT_COUNT);
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string fileId = GetStringVal(MediaColumn::MEDIA_ID, resultSet);
        updateFileIds.emplace_back(fileId);
    }
    resultSet->Close();
    CHECK_AND_RETURN_RET_LOG(updateFileIds.size() > 0, false, "the size of updateFileIds 0.");
    return true;
}

int32_t CloudMediaAssetManager::UpdateLocalAndCloudAssets(const std::vector<std::string> &updateFileIds)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateLocalAndCloudAssets");
    CHECK_AND_RETURN_RET_LOG(!updateFileIds.empty(), E_ERR, "updateFileIds is null.");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "UpdateLocalAndCloudAssets failed. rdbStore is null.");
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_ID, updateFileIds);

    ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
    values.PutLong(PhotoColumn::PHOTO_CLOUD_VERSION, 0);
    values.PutNull(PhotoColumn::PHOTO_CLOUD_ID);
    values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL));
    values.PutInt(PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE, static_cast<int32_t>(SouthDeviceType::SOUTH_DEVICE_NULL));

    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG((ret == E_OK && changedRows >= 0), E_ERR,
        "Failed to UpdateLocalAndCloudAssets, ret: %{public}d, updateRows: %{public}d", ret, changedRows);
    MEDIA_INFO_LOG("UpdateLocalAndCloudAssets successfully. ret: %{public}d, updateRows: %{public}d", ret, changedRows);
    return E_OK;
}

int32_t CloudMediaAssetManager::ClearDeletedDbData()
{
    MEDIA_INFO_LOG("start ClearDeletedDbData.");
    MediaLibraryTracer tracer;
    tracer.Start("ClearDeletedDbData");

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "ClearDeletedDbData failed. rdbStore is null.");

    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_DIRTY, to_string(static_cast<int32_t>(DirtyType::TYPE_DELETED)));

    int32_t deletedRows = -1;
    auto ret = rdbStore->Delete(deletedRows, predicates);
    CHECK_AND_RETURN_RET_LOG((ret == E_OK && deletedRows >= 0), E_ERR,
        "Failed to ClearDeletedDbData, ret: %{public}d, deletedRows: %{public}d", ret, deletedRows);
    MEDIA_INFO_LOG("ClearDeletedDbData successfully. ret: %{public}d, deletedRows: %{public}d", ret, deletedRows);
    return OHOS::Media::E_OK;
}

int32_t CloudMediaAssetManager::UpdateBothLocalAndCloudAssets(CloudMediaRetainType retainType)
{
    MEDIA_INFO_LOG("start UpdateBothLocalAndCloudAssets.");
    MediaLibraryTracer tracer;
    tracer.Start("UpdateBothLocalAndCloudAssets");

    int32_t deleteRet = ClearDeletedDbData();
    CHECK_AND_PRINT_LOG(deleteRet == E_OK, "ClearDeletedDbData failed. ret %{public}d.", deleteRet);

    int32_t cycleNumber = 0;
    std::string lastFileId = START_QUERY_ZERO;
    std::vector<std::string> updateFileIds;
    while (HasLocalAndCloudAssets(retainType, updateFileIds, lastFileId) && cycleNumber++ <= CYCLE_NUMBER) {
        int32_t ret = UpdateLocalAndCloudAssets(updateFileIds);
        CHECK_AND_BREAK_ERR_LOG(ret == E_OK, "UpdateBothLocalAndCloudAssets failed, ret: %{public}d", ret);
        lastFileId = updateFileIds.back();
    }
    MEDIA_INFO_LOG("end UpdateBothLocalAndCloudAssets.");
    return E_OK;
}

int32_t CloudMediaAssetManager::UpdateLocalAlbums()
{
    MEDIA_INFO_LOG("start UpdateLocalAlbums.");
    MediaLibraryTracer tracer;
    tracer.Start("UpdateLocalAlbums");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "UpdateLocalAlbums failed. rdbStore is null");
    std::vector<std::string> sqls = {
        this->SQL_CLEAE_CLOUD_INFO_OF_ALBUM,
        this->SQL_RESET_UPLOAD_STATUS,
    };
    int32_t ret = E_OK;
    for (const auto &sql : sqls) {
        ret = rdbStore->ExecuteSql(sql);
        CHECK_AND_PRINT_LOG(
            ret == NativeRdb::E_OK, "Failed to execute sql, ret: %{public}d, sql: %{public}s", ret, sql.c_str());
    }
    MEDIA_INFO_LOG("UpdateLocalAlbums successfully. ret %{public}d.", ret);
    // Need to update settingData.
    SettingsDataManager::UpdateOrInsertAllPhotosAlbumUpload();
    return E_OK;
}

int32_t CloudMediaAssetManager::ForceRetainDownloadCloudMedia(CloudMediaRetainType retainType,
    bool needAvoidRepeatedDoing)
{
    MediaLibraryTracer tracer;
    auto retainTypeInt = static_cast<int32_t>(retainType);
    tracer.Start(std::string("ForceRetainDownloadCloudMedia") + std::to_string(retainTypeInt));
    MEDIA_INFO_LOG("enter. retainType: %{public}d", retainTypeInt);

    AuditLog auditLog = { true, "DFX", "CLOUD_EXIT", "0", 0, "success", "ForceRetainDownloadCloudMedia" };
    auditLog.id = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    auditLog.type = static_cast<int32_t>(retainType);
    HiAudit::GetInstance().Write(auditLog);

    if (needAvoidRepeatedDoing && IsSouthDeviceSyncCleaning(retainType)) {
        MEDIA_INFO_LOG("this south device is cleaning, retainType: %{public}d", retainTypeInt);
        return E_OK;
    }

    // 判断是不是已经有个南向设备端在cleaning, 如果有也直接返回
    bool isCloudCleaning = IsSouthDeviceSyncCleaning(CloudMediaRetainType::RETAIN_FORCE);
    bool isHdcCleaning = IsSouthDeviceSyncCleaning(CloudMediaRetainType::HDC_RETAIN_FORCE);
    auto switchStatus = SettingsDataManager::GetPhotosSyncSwitchStatus();
    SetSouthDeviceCleanStatus(retainType, CloudSyncStatus::CLOUD_CLEANING);
    SetSouthDeviceNextStatus(retainType, switchStatus);
    auto smartDataMode = GetSmartDataProcessingMode(retainType, switchStatus);
    MEDIA_INFO_LOG("cloud: %{public}d, hdc: %{public}d switchStatus is %{public}d smartDataMode is %{public}d",
        isCloudCleaning, isHdcCleaning, static_cast<int32_t>(switchStatus), static_cast<int32_t>(smartDataMode));
    SetSmartDataProcessingMode(smartDataMode);
    if (needAvoidRepeatedDoing && (isCloudCleaning || isHdcCleaning)) {
        MEDIA_INFO_LOG("some south devices are being cleaned. cloud: %{public}d, hdc: %{public}d",
            isCloudCleaning, isHdcCleaning);
        return E_OK;
    }
    SetSouthDeviceSyncSwitchStatus(CloudSyncStatus::CLOUD_CLEANING);

    // 主动停止端云同步
    MEDIA_INFO_LOG("ForceRetainDownloadCloudMedia StopSync bundleName:%{public}s", BUNDLE_NAME.c_str());
    CloudSyncManager::GetInstance().StopSync(BUNDLE_NAME);

    // 备份/恢复需要特殊处理，等待备份和恢复完成再清除;
    WaitIfBackUpingOrRestoring();
    auto ret = ForceRetainDownloadCloudMediaEx(retainType, smartDataMode);
    MEDIA_INFO_LOG("ForceRetainDownloadCloudMediaEx ret: %{public}d", ret);
    SetSouthDeviceCleanStatus(retainType, CloudSyncStatus::SYNC_SWITCHED_OFF);
    isCloudCleaning = IsSouthDeviceSyncCleaning(CloudMediaRetainType::RETAIN_FORCE);
    isHdcCleaning = IsSouthDeviceSyncCleaning(CloudMediaRetainType::HDC_RETAIN_FORCE);

    if (isCloudCleaning) {
        switchStatus = GetSouthDeviceNextStatus(CloudMediaRetainType::RETAIN_FORCE);
        smartDataMode = GetSmartDataProcessingMode(CloudMediaRetainType::RETAIN_FORCE, switchStatus);
        int32_t tempRet = ForceRetainDownloadCloudMediaEx(CloudMediaRetainType::RETAIN_FORCE, smartDataMode);
        SetSouthDeviceCleanStatus(CloudMediaRetainType::RETAIN_FORCE, CloudSyncStatus::SYNC_SWITCHED_OFF);
        CHECK_AND_PRINT_LOG(tempRet == E_OK, "cloud force retain. ret %{public}d.", tempRet);
        CHECK_AND_EXECUTE(tempRet == E_OK, ret = tempRet);
    }

    if (isHdcCleaning) {
        switchStatus = GetSouthDeviceNextStatus(CloudMediaRetainType::HDC_RETAIN_FORCE);
        smartDataMode = GetSmartDataProcessingMode(CloudMediaRetainType::HDC_RETAIN_FORCE, switchStatus);
        int32_t tempRet = ForceRetainDownloadCloudMediaEx(CloudMediaRetainType::HDC_RETAIN_FORCE, smartDataMode);
        SetSouthDeviceCleanStatus(CloudMediaRetainType::HDC_RETAIN_FORCE, CloudSyncStatus::SYNC_SWITCHED_OFF);
        CHECK_AND_PRINT_LOG(tempRet == E_OK, "hdc force retain. ret %{public}d.", tempRet);
        CHECK_AND_EXECUTE(tempRet == E_OK, ret = tempRet);
    }

    if (CloudSyncHelper::GetInstance()->IsSyncSwitchOpen()) {
        MEDIA_INFO_LOG("cloud sync manager start reset cursor");
        FileManagement::CloudSync::CloudSyncManager::GetInstance().ResetCursor(true);
        MEDIA_INFO_LOG("cloud sync manager end reset cursor");
    }

    SetSouthDeviceSyncSwitchStatus(CloudSyncStatus::SYNC_SWITCHED_OFF);

    TryToStartSync();

    MEDIA_INFO_LOG("exit. retainType: %{public}d", retainTypeInt);
    return ret;
}

static void SetSmartDataRetainStates()
{
    SetSmartDataCleanState(CleanTaskState::IDLE);
    SetSmartDataUpdateState(UpdateSmartDataState::UPDATE_SMART_DATA);
    SetSmartDataRetainTime();
    InitBackupPhotosAlbumTable();
    BackupBackupPhotosAlbumTable();
    UpdateInvalidCloudHighlightInfo();
    std::vector<std::string> idToUpdateIndex;
    MediaAnalysisHelper::AsyncStartMediaAnalysisService(
        static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::UPDATE_MEDIA_INDEX_FOR_CLOUD),
        idToUpdateIndex);
}

int32_t CloudMediaAssetManager::ForceRetainDownloadCloudMediaEx(CloudMediaRetainType retainType,
    SmartDataProcessingMode mode)
{
    auto retainTypeInt = static_cast<int32_t>(retainType);
    MEDIA_INFO_LOG("enter ForceRetainDownloadCloudMediaEx. retainType: %{public}d", retainTypeInt);
    std::unique_lock<std::mutex> lock(updateMutex_, std::defer_lock);
    CHECK_AND_RETURN_RET_WARN_LOG(lock.try_lock(), E_ERR, // 此处的作用貌似和避免重复的逻辑一致
        "retainType: %{public}d, data is cleaning, skipping this operation", retainTypeInt);

    MediaLibraryTracer tracer;
    tracer.Start(std::string("ForceRetainDownloadCloudMediaEx, retainType:") + std::to_string(retainTypeInt));
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    // 检查点 批量下载 关闭端云开关和退账号适配 清理下载任务表 通知应用 notify type 6
    MEDIA_INFO_LOG("BatchSelectFileDownload ForceRetainDownloadCloudMedia CleanDownloadTasksTable");
    CleanDownloadTasksTable();
#endif
    // 停止后台异步清理云图任务，待本次云上信息标记完后重新开启
    doDeleteTask_.store(TaskDeleteState::IDLE);
    int32_t updateRet = UpdateCloudMediaAssets(retainType, mode);
    CHECK_AND_PRINT_LOG(updateRet == OHOS::Media::E_OK, "UpdateCloudMediaAssets failed. ret %{public}d.", updateRet);
    if (mode == SmartDataProcessingMode::RETAIN) {
        SetSmartDataRetainStates();
    } else if (mode == SmartDataProcessingMode::RECOVER) {
        SetSmartDataCleanState(CleanTaskState::IDLE);
        SetSmartDataRetainTime();
    } else {
        DeleteBackupPhotosAlbumForSmartData();
    }
    int32_t ret = DeleteEmptyCloudAlbums();
    CHECK_AND_PRINT_LOG(ret == OHOS::Media::E_OK, "DeleteEmptyCloudAlbums failed. ret %{public}d.", ret);
    auto watch = MediaLibraryNotify::GetInstance();
    if (watch != nullptr) {
        MEDIA_INFO_LOG("begin to notify album update.");
        watch->Notify(PhotoAlbumColumns::ALBUM_URI_PREFIX, NotifyType::NOTIFY_UPDATE);
    }

    ret = UpdateBothLocalAndCloudAssets(retainType);
    CHECK_AND_PRINT_LOG(ret == E_OK, "UpdateBothLocalAndCloudAssets failed. ret %{public}d.", ret);
    ret = UpdateLocalAlbums();
    CHECK_AND_PRINT_LOG(ret == E_OK, "UpdateLocalAlbums failed. ret %{public}d.", ret);

    MEDIA_INFO_LOG("start delete cloud media assets task.");
    doDeleteTask_.store(TaskDeleteState::ACTIVE_DELETE);
    DeleteAllCloudMediaAssetsAsync();

    CHECK_AND_RETURN_RET_WARN_LOG(updateRet == E_OK, updateRet,
        "exit ForceRetainDownloadCloudMediaEx, Type: %{public}d, updateRet: %{public}d.", retainTypeInt, updateRet);
    MEDIA_INFO_LOG("exit ForceRetainDownloadCloudMediaEx, retainType: %{public}d", retainTypeInt);
    return ret;
}

std::string CloudMediaAssetManager::GetCloudMediaAssetTaskStatus()
{
    if (operation_ == nullptr || operation_->GetTaskStatus() == CloudMediaAssetTaskStatus::IDLE) {
        MEDIA_ERR_LOG("cloud media download task not exit.");
        return to_string(static_cast<int32_t>(CloudMediaAssetTaskStatus::IDLE)) + ",0,0,0,0,0";
    }
    return to_string(static_cast<int32_t>(operation_->GetTaskStatus())) + "," + operation_->GetTaskInfo() + "," +
        to_string(static_cast<int32_t>(operation_->GetTaskPauseCause()));
}

int32_t CloudMediaAssetManager::UpdateAddTaskStatus(const std::vector<std::string> &fileIds,
    const CloudMediaTaskDownloadCloudAssetCode &status, std::map<std::string, int32_t> &uriStatusMap)
{
    if (fileIds.empty()) {
        return E_OK;
    }
    //set status for add task
    for (auto &fileId : fileIds) {
        auto result = uriStatusMap.insert({fileId, static_cast<int32_t>(status)});
        if (!result.second) {
            MEDIA_ERR_LOG("Element with fileId: %{public}s already in map", result.first->first.c_str());
        }
    }
    return E_OK;
}

int32_t CloudMediaAssetManager::BuildTaskValuesAndBatchInsert(
    int64_t &insertCount, std::vector<DownloadResourcesTaskPo> &newTaskPos)
{
    std::vector<NativeRdb::ValuesBucket> batchValues;
    for (auto &po : newTaskPos) {
        NativeRdb::ValuesBucket values;
        values.PutInt(DownloadResourcesColumn::MEDIA_ID, po.fileId.value_or(-1));
        values.PutString(DownloadResourcesColumn::MEDIA_NAME, po.fileName.value_or(""));
        values.PutLong(DownloadResourcesColumn::MEDIA_SIZE, po.fileSize.value_or(0));
        values.PutString(DownloadResourcesColumn::MEDIA_URI, po.fileUri.value_or(""));
        values.PutLong(DownloadResourcesColumn::MEDIA_DATE_ADDED, po.dateAdded.value_or(0));
        values.PutLong(DownloadResourcesColumn::MEDIA_DATE_FINISH, po.dateFinish.value_or(0));
        values.PutInt(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS, po.downloadStatus.value_or(0));
        values.PutInt(DownloadResourcesColumn::MEDIA_PERCENT, po.percent.value_or(-1));
        values.PutInt(DownloadResourcesColumn::MEDIA_AUTO_PAUSE_REASON, po.autoPauseReason.value_or(0));
        values.PutInt(DownloadResourcesColumn::MEDIA_COVER_LEVEL, po.coverLevel.value_or(1));
        batchValues.push_back(values);
    }
    MEDIA_INFO_LOG("BatchSelectFileDownload Insert Bucket Size:%{public}zu", batchValues.size());
    return this->batchDownloadResourcesTaskDao_.BatchInsert(insertCount, DownloadResourcesColumn::TABLE, batchValues);
}

int32_t CloudMediaAssetManager::StartBatchDownloadCloudResources(StartBatchDownloadCloudResourcesReqBody &reqBody,
    StartBatchDownloadCloudResourcesRespBody &respBody)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    CHECK_AND_RETURN_RET_LOG(!reqBody.uris.empty(), E_INVALID_ARGS, "StartBatchDownload No uris");
    CHECK_AND_RETURN_RET_LOG(!(reqBody.uris.size() > MAX_BATCH_DOWNLOAD_TASK_SIZE), E_INVALID_ARGS,
        "StartBatchDownload uris is greater than %{public}d", MAX_BATCH_DOWNLOAD_TASK_SIZE);
    std::unique_lock<std::mutex> lock(batchDownloadMutex_);
    MediaLibraryTracer tracer;
    tracer.Start("StartBatchDownloadCloudResources");
    MEDIA_INFO_LOG("BatchSelectFileDownload Enter StartBatchDownloadCloudResources");
    respBody.uriStatusMap.clear();
    std::vector<std::string> allFileIds;
    this->batchDownloadResourcesTaskDao_.FromUriToAllFileIds(reqBody.uris, allFileIds);
    std::vector<std::string> newTaskFileIds;
    std::vector<std::string> existedFileIds;
    std::vector<std::string> invalidFileIds;
    this->batchDownloadResourcesTaskDao_.ClassifyExistedDownloadTasks(allFileIds, newTaskFileIds, existedFileIds);
    this->batchDownloadResourcesTaskDao_.ClassifyInvalidDownloadTasks(newTaskFileIds, invalidFileIds);
    this->batchDownloadResourcesTaskDao_.HandleAddExistedDownloadTasks(existedFileIds);

    // 查photos表 构建任务表记录
    std::vector<DownloadResourcesTaskPo> newTaskPos;
    int32_t ret = this->batchDownloadResourcesTaskDao_.QueryValidBatchDownloadPoFromPhotos(newTaskFileIds, newTaskPos);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "QueryValidBatchDownloadPoFromPhotos failed");
    int64_t insertCount = 0;
    int32_t result = BuildTaskValuesAndBatchInsert(insertCount, newTaskPos);
    MEDIA_INFO_LOG("BatchSelectFileDownload AddTask Res:%{public}d, Count:%{public}" PRId64, result, insertCount);
    UpdateAddTaskStatus(newTaskFileIds,
        CloudMediaTaskDownloadCloudAssetCode::ADD_DOWNLOAD_TASK_SUCC, respBody.uriStatusMap);
    UpdateAddTaskStatus(existedFileIds,
        CloudMediaTaskDownloadCloudAssetCode::ADD_DOWNLOAD_TASK_SUCC, respBody.uriStatusMap);
    UpdateAddTaskStatus(invalidFileIds,
        CloudMediaTaskDownloadCloudAssetCode::ADD_DOWNLOAD_ASSET_NOT_EXIST, respBody.uriStatusMap);

    MEDIA_INFO_LOG("BatchSelectFileDownload Start LaunchBatchDownloadProcessor");
    BackgroundCloudBatchSelectedFileProcessor::SetBatchDownloadAddedFlag(true);
    BackgroundCloudBatchSelectedFileProcessor::LaunchBatchDownloadProcessor(); // 触发启动检查
#endif
    return E_OK;
}

int32_t CloudMediaAssetManager::ResumeBatchDownloadCloudResources(ResumeBatchDownloadCloudResourcesReqBody &reqBody)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    std::unique_lock<std::mutex> lock(batchDownloadMutex_);
    MediaLibraryTracer tracer;
    CHECK_AND_RETURN_RET_LOG(!(reqBody.uris.size() > MAX_BATCH_DOWNLOAD_TASK_SIZE), E_INVALID_ARGS,
        "ResumeBatchDownload uris is greater than %{public}d", MAX_BATCH_DOWNLOAD_TASK_SIZE);
    tracer.Start("ResumeBatchDownloadCloudResources");
    MEDIA_INFO_LOG("BatchSelectFileDownload enter ResumeBatchDownloadCloudResources");
    if (reqBody.uris.empty()) {
        return this->batchDownloadResourcesTaskDao_.UpdateResumeAllDownloadResourcesInfo();
    }
    std::vector<std::string> allFileIds;
    this->batchDownloadResourcesTaskDao_.FromUriToAllFileIds(reqBody.uris, allFileIds);

    int32_t ret = this->batchDownloadResourcesTaskDao_.UpdateResumeDownloadResourcesInfo(allFileIds);
    MEDIA_INFO_LOG("BatchSelectFileDownload ResumeBatchDownloadCloudResources Resume ret:%{public}d", ret);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "UpdateResumeDownloadResourcesInfo failed");
    MEDIA_INFO_LOG("BatchSelectFileDownload Resume LaunchBatchDownloadProcessor");
    BackgroundCloudBatchSelectedFileProcessor::SetBatchDownloadAddedFlag(true);
    BackgroundCloudBatchSelectedFileProcessor::LaunchBatchDownloadProcessor(); // 触发启动检查
#endif
    return E_OK;
}

int32_t CloudMediaAssetManager::PauseBatchDownloadCloudResources(PauseBatchDownloadCloudResourcesReqBody &reqBody)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    CHECK_AND_RETURN_RET_LOG(!(reqBody.uris.size() > MAX_BATCH_DOWNLOAD_TASK_SIZE), E_INVALID_ARGS,
        "PauseBatchDownload uris is greater than %{public}d", MAX_BATCH_DOWNLOAD_TASK_SIZE);
    std::unique_lock<std::mutex> lock(batchDownloadMutex_);
    MediaLibraryTracer tracer;
    tracer.Start("PauseBatchDownloadCloudResources");
    MEDIA_INFO_LOG("BatchSelectFileDownload enter PauseBatchDownloadCloudResources");
    if (reqBody.uris.empty()) {
        BackgroundCloudBatchSelectedFileProcessor::TriggerStopBatchDownloadProcessor(false);
        this->batchDownloadResourcesTaskDao_.UpdateAllPauseDownloadResourcesInfo();
        return E_OK;
    }
    std::vector<std::string> allFileIds;
    this->batchDownloadResourcesTaskDao_.FromUriToAllFileIds(reqBody.uris, allFileIds);
    std::vector<std::string> fileIdsDownloading;
    std::vector<std::string> fileIdsNotInDownloading;
    this->batchDownloadResourcesTaskDao_.QueryPauseDownloadingStatusResources(allFileIds, fileIdsDownloading,
        fileIdsNotInDownloading);
    int32_t ret = this->batchDownloadResourcesTaskDao_.UpdatePauseDownloadResourcesInfo(fileIdsNotInDownloading);
    // 暂停 处理下载中文件
    BackgroundCloudBatchSelectedFileProcessor::TriggerPauseBatchDownloadProcessor(fileIdsDownloading);
    ret = this->batchDownloadResourcesTaskDao_.UpdatePauseDownloadResourcesInfo(fileIdsDownloading);
    MEDIA_INFO_LOG("BatchSelectFileDownload PauseBatchDownloadCloudResources Pause ret:%{public}d", ret);
#endif
    return E_OK;
}

#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
void CloudMediaAssetManager::CleanDownloadTasksTable()
{
    BackgroundCloudBatchSelectedFileProcessor::TriggerStopBatchDownloadProcessor(true);
    this->batchDownloadResourcesTaskDao_.DeleteAllDownloadResourcesInfo();
    BackgroundCloudBatchSelectedFileProcessor::NotifyRefreshProgressInfo();
}
#endif

int32_t CloudMediaAssetManager::CancelBatchDownloadCloudResources(CancelBatchDownloadCloudResourcesReqBody &reqBody)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    CHECK_AND_RETURN_RET_LOG(!(reqBody.uris.size() > MAX_BATCH_DOWNLOAD_TASK_SIZE), E_INVALID_ARGS,
        "CancelBatchDownload uris is greater than %{public}d", MAX_BATCH_DOWNLOAD_TASK_SIZE);
    std::unique_lock<std::mutex> lock(batchDownloadMutex_);
    MediaLibraryTracer tracer;
    tracer.Start("CancelBatchDownloadCloudResources");
    MEDIA_INFO_LOG("BatchSelectFileDownload enter CancelBatchDownloadCloudResources");
    if (reqBody.uris.empty()) {
        CleanDownloadTasksTable();
        return E_OK;
    }
    std::vector<std::string> allFileIds;
    this->batchDownloadResourcesTaskDao_.FromUriToAllFileIds(reqBody.uris, allFileIds);
    std::vector<std::string> fileIdsDownloading;
    std::vector<std::string> fileIdsNotInDownloading;
    this->batchDownloadResourcesTaskDao_.QueryCancelDownloadingStatusResources(allFileIds, fileIdsDownloading,
        fileIdsNotInDownloading);
    int32_t ret = this->batchDownloadResourcesTaskDao_.DeleteCancelStateDownloadResources(fileIdsNotInDownloading);
    // 取消 处理下载中文件
    BackgroundCloudBatchSelectedFileProcessor::TriggerCancelBatchDownloadProcessor(fileIdsDownloading, false);
    MEDIA_INFO_LOG("CancelBatchDownloadCloudResources ret:%{public}d", ret);
#endif
    return E_OK;
}

int32_t CloudMediaAssetManager::GetCloudMediaBatchDownloadResourcesStatus(
    GetBatchDownloadCloudResourcesStatusReqBody &reqBody, GetBatchDownloadCloudResourcesStatusRespBody &respBody)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    std::unique_lock<std::mutex> lock(batchDownloadMutex_);
    MediaLibraryTracer tracer;
    tracer.Start("GetCloudMediaBatchDownloadResourcesStatus");
    MEDIA_INFO_LOG("BatchSelectFileDownload enter GetCloudMediaBatchDownloadResourcesStatus");
    DataShare::DataSharePredicates predicates = reqBody.predicates;
    NativeRdb::RdbPredicates rdbPredicates =
        RdbDataShareAdapter::RdbUtils::ToPredicates(predicates, DownloadResourcesColumn::TABLE);
    std::vector<DownloadResourcesTaskPo> downloadResourcesTasks;
    int32_t ret = this->batchDownloadResourcesTaskDao_.QueryCloudMediaBatchDownloadResourcesStatus(rdbPredicates,
        downloadResourcesTasks);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "QueryCloudMediaBatchDownloadResourcesStatus failed");
    MEDIA_INFO_LOG("GetCloudMediaBatchDownload Get after task size:%{public}zu", downloadResourcesTasks.size());
    // 组装 respBody.downloadResourcesStatus
    for (const DownloadResourcesTaskPo &downloadResourcesTask : downloadResourcesTasks) {
        std::ostringstream entryStream;
        entryStream << downloadResourcesTask.fileId.value_or(-1) << "|"
                    << downloadResourcesTask.fileName.value_or("") << "|"
                    << downloadResourcesTask.fileSize.value_or(0) << "|"
                    << downloadResourcesTask.fileUri.value_or("") << "|"
                    << downloadResourcesTask.dateAdded.value_or(0) << "|"
                    << downloadResourcesTask.dateFinish.value_or(0) << "|"
                    << downloadResourcesTask.downloadStatus.value_or(0) << "|"
                    << downloadResourcesTask.percent.value_or(0) << "|"
                    << downloadResourcesTask.autoPauseReason.value_or(0) << "|"
                    << downloadResourcesTask.coverLevel.value_or(1);

        std::string entry = entryStream.str();
        respBody.downloadResourcesStatus.emplace_back(std::move(entry));
    }
#endif
    return E_OK;
}

int32_t CloudMediaAssetManager::GetCloudMediaBatchDownloadResourcesCount(
    GetBatchDownloadCloudResourcesCountReqBody &reqBody, GetBatchDownloadCloudResourcesCountRespBody &respBody)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    std::unique_lock<std::mutex> lock(batchDownloadMutex_);
    MediaLibraryTracer tracer;
    tracer.Start("GetCloudMediaBatchDownloadResourcesCount");
    MEDIA_INFO_LOG("BatchSelectFileDownload enter GetCloudMediaBatchDownloadResourcesCount");
    DataShare::DataSharePredicates predicates = reqBody.predicates;
    NativeRdb::RdbPredicates rdbPredicates =
        RdbDataShareAdapter::RdbUtils::ToPredicates(predicates, DownloadResourcesColumn::TABLE);
    int32_t count = 0;
    int32_t ret = this->batchDownloadResourcesTaskDao_.QueryCloudMediaBatchDownloadResourcesCount(rdbPredicates,
        count);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "QueryCloudMediaBatchDownloadResourcesCount failed");
    respBody.count = count;
#endif
    return E_OK;
}

int32_t CloudMediaAssetManager::HandleCloudMediaAssetUpdateOperations(MediaLibraryCommand &cmd)
{
    switch (cmd.GetOprnType()) {
        case OperationType::CLOUD_MEDIA_ASSET_TASK_START_FORCE: {
            return StartDownloadCloudAsset(CloudMediaDownloadType::DOWNLOAD_FORCE);
        }
        case OperationType::CLOUD_MEDIA_ASSET_TASK_START_GENTLE: {
            return StartDownloadCloudAsset(CloudMediaDownloadType::DOWNLOAD_GENTLE);
        }
        case OperationType::CLOUD_MEDIA_ASSET_TASK_PAUSE: {
            return PauseDownloadCloudAsset(CloudMediaTaskPauseCause::USER_PAUSED);
        }
        case OperationType::CLOUD_MEDIA_ASSET_TASK_CANCEL: {
            return CancelDownloadCloudAsset();
        }
        case OperationType::CLOUD_MEDIA_ASSET_TASK_RETAIN_FORCE: {
            return ForceRetainDownloadCloudMedia(CloudMediaRetainType::RETAIN_FORCE);
        }
        default: {
            MEDIA_ERR_LOG("OprnType is not exit.");
            return E_ERR;
        }
    }
}

string CloudMediaAssetManager::HandleCloudMediaAssetGetTypeOperations(MediaLibraryCommand &cmd)
{
    switch (cmd.GetOprnType()) {
        case OperationType::CLOUD_MEDIA_ASSET_TASK_STATUS_QUERY: {
            return GetCloudMediaAssetTaskStatus();
        }
        default: {
            MEDIA_ERR_LOG("OprnType is not exit.");
            return "";
        }
    }
}

bool CloudMediaAssetManager::SetIsThumbnailUpdate()
{
    if (operation_ == nullptr || operation_->GetTaskStatus() == CloudMediaAssetTaskStatus::IDLE) {
        return false;
    }
    if (!operation_->isThumbnailUpdate_) {
        MEDIA_INFO_LOG("Success set isThumbnailUpdate.");
        operation_->isThumbnailUpdate_ = true;
    }
    MEDIA_INFO_LOG("Update count and size of download cloud media asset.");
    if (operation_->InitDownloadTaskInfo() != E_OK) {
        MEDIA_INFO_LOG("remainCount of download cloud media assets is 0.");
        operation_->CancelDownloadTask();
    }
    return true;
}

int32_t CloudMediaAssetManager::GetTaskStatus()
{
    CHECK_AND_RETURN_RET(operation_ != nullptr, static_cast<int32_t>(CloudMediaAssetTaskStatus::IDLE));
    return static_cast<int32_t>(operation_->GetTaskStatus());
}

int32_t CloudMediaAssetManager::GetDownloadType()
{
    if (operation_ == nullptr) {
        MEDIA_INFO_LOG("cloud media download task not exit.");
        return E_ERR;
    }
    return static_cast<int32_t>(operation_->GetDownloadType());
}

bool CloudMediaAssetManager::SetBgDownloadPermission(const bool &flag)
{
    bool cond = (operation_ == nullptr || operation_->GetTaskStatus() == CloudMediaAssetTaskStatus::IDLE);
    CHECK_AND_RETURN_RET(!cond, false);
    MEDIA_INFO_LOG("Success set isBgDownloadPermission, flag: %{public}d.", static_cast<int32_t>(flag));
    operation_->isBgDownloadPermission_ = flag;
    return true;
}

void CloudMediaAssetManager::RestartForceRetainCloudAssets()
{
    std::thread([&] {
        MEDIA_INFO_LOG("enter RestartForceRetainCloudAssets.");
        auto mode = GetSmartDataProcessingMode();
        auto isCleaning = IsSouthDeviceSyncCleaning(CloudMediaRetainType::RETAIN_FORCE, false);
        isCleaning = isCleaning || IsSouthDeviceSyncCleaning(CloudMediaRetainType::HDC_RETAIN_FORCE, false);
        if (isCleaning) {
            MEDIA_WARN_LOG("restart continue retain force, given the current design, any of hdc/cloud can be chosen");
            auto retainType = CloudMediaRetainType::RETAIN_FORCE;
            if (mode == SmartDataProcessingMode::RECOVER) {
                retainType = CloudMediaRetainType::HDC_RETAIN_FORCE;
            }
            ForceRetainDownloadCloudMedia(retainType, false);
        }
    }).detach();
}

void CloudMediaAssetManager::TryToStartSync()
{
    if (!CloudSyncHelper::GetInstance()->IsSyncSwitchOpen()) {
        MEDIA_INFO_LOG("syncSwitch is not open");
        return;
    }
    MEDIA_INFO_LOG("cloud sync manager start sync");
    int32_t ret = CloudSyncManager::GetInstance().StartSync(BUNDLE_NAME);
    CHECK_AND_PRINT_LOG(ret == E_OK, "cloud sync manager start sync err %{public}d", ret);
    MEDIA_INFO_LOG("cloud sync manager end sync");
}
} // namespace Media
} // namespace OHOS