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

#define MLOG_TAG "CloudMediaAssetOperations"

#include "cloud_media_asset_download_operation.h"
#include <sys/statvfs.h>

#include "common_event_utils.h"
#include "cloud_sync_utils.h"
#include "iservice_registry.h"
#include "media_file_utils.h"
#include "media_file_uri.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_tracer.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "net_conn_client.h"
#include "userfile_manager_types.h"
#include "medialibrary_related_system_state_manager.h"

using namespace std;
using namespace OHOS::DataShare;
using namespace OHOS::NativeRdb;
using namespace OHOS::NetManagerStandard;
namespace OHOS {
namespace Media {
using namespace FileManagement::CloudSync;
using Status = CloudMediaAssetDownloadOperation::Status;
std::mutex CloudMediaAssetDownloadOperation::mutex_;
std::mutex CloudMediaAssetDownloadOperation::callbackMutex_;
std::shared_ptr<CloudMediaAssetDownloadOperation> CloudMediaAssetDownloadOperation::instance_ = nullptr;
static const int32_t PROPER_DEVICE_TEMPERATURE_LEVEL_HOT = 3;
static const int32_t BATCH_DOWNLOAD_CLOUD_FILE = 10;
static constexpr int CLOUD_MANAGER_MANAGER_ID = 5204;
static const int64_t DOWNLOAD_ID_DEFAULT = -1;
static const std::string TOTAL_COUNT = "COUNT(1)";
static const std::string TOTAL_SIZE = "SUM(size)";
static const bool NEED_CLEAN = true;
static const int32_t EXIT_TASK = 1;
// CLOUD_E_PATH_NOT_FOUND and CLOUD_E_RDB corresponds to the E_PATH and E_RDB of dfs_error.h
static const int32_t CLOUD_E_PATH_NOT_FOUND = 28;
static const int32_t CLOUD_E_RDB = 22;
static const int32_t SLEEP_FOR_LOCK = 100;
static const int32_t STATUS_CHANGE_ARG_SIZE = 3;
static const int32_t INDEX_ZERO = 0;
static const int32_t INDEX_ONE = 1;
static const int32_t INDEX_TWO = 2;
static constexpr double PROPER_DEVICE_STORAGE_CAPACITY_RATIO = 0.1;
static const std::string STORAGE_PATH = "/data/storage/el2/database/";
static const uint32_t MAX_DOWNLOAD_TRY_TIMES = 3;

static const std::map<Status, std::vector<int32_t>> STATUS_MAP = {
    { Status::FORCE_DOWNLOADING, {0, 0, 0} },
    { Status::GENTLE_DOWNLOADING, {1, 0, 0} },
    { Status::PAUSE_FOR_TEMPERATURE_LIMIT, {-1, 1, 1} },
    { Status::PAUSE_FOR_ROM_LIMIT, {-1, 1, 2} },
    { Status::PAUSE_FOR_NETWORK_FLOW_LIMIT, {-1, 1, 3} },
    { Status::PAUSE_FOR_WIFI_UNAVAILABLE, {-1, 1, 4} },
    { Status::PAUSE_FOR_POWER_LIMIT, {-1, 1, 5} },
    { Status::PAUSE_FOR_BACKGROUND_TASK_UNAVAILABLE, {1, 1, 6} },
    { Status::PAUSE_FOR_FREQUENT_USER_REQUESTS, {-1, 1, 7} },
    { Status::PAUSE_FOR_CLOUD_ERROR, {-1, 1, 8} },
    { Status::PAUSE_FOR_USER_PAUSE, {-1, 1, 9} },
    { Status::RECOVER_FOR_MANAUL_ACTIVE, {0, 0, 0} },
    { Status::RECOVER_FOR_PASSIVE_STATUS, {-1, 0, 0} },
    { Status::IDLE, {-1, 2, 0} },
};

static const std::map<CloudMediaTaskRecoverCause, CloudMediaTaskPauseCause> RECOVER_RELATIONSHIP_MAP = {
    { CloudMediaTaskRecoverCause::FOREGROUND_TEMPERATURE_PROPER, CloudMediaTaskPauseCause::TEMPERATURE_LIMIT },
    { CloudMediaTaskRecoverCause::STORAGE_NORMAL, CloudMediaTaskPauseCause::ROM_LIMIT },
    { CloudMediaTaskRecoverCause::NETWORK_FLOW_UNLIMIT, CloudMediaTaskPauseCause::NETWORK_FLOW_LIMIT },
    { CloudMediaTaskRecoverCause::BACKGROUND_TASK_AVAILABLE, CloudMediaTaskPauseCause::BACKGROUND_TASK_UNAVAILABLE },
    { CloudMediaTaskRecoverCause::RETRY_FOR_FREQUENT_REQUESTS, CloudMediaTaskPauseCause::FREQUENT_USER_REQUESTS },
    { CloudMediaTaskRecoverCause::RETRY_FOR_CLOUD_ERROR, CloudMediaTaskPauseCause::CLOUD_ERROR },
};

void CloudDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    MEDIA_INFO_LOG("enter.");
    CHECK_AND_RETURN_LOG(object.promote() != nullptr, "remote object is nullptr");
    object->RemoveDeathRecipient(this);
    CHECK_AND_RETURN_LOG(operation_, "operation is nullptr");
    operation_->HandleOnRemoteDied();
}

std::shared_ptr<CloudMediaAssetDownloadOperation> CloudMediaAssetDownloadOperation::GetInstance()
{
    if (instance_ == nullptr) {
        instance_ = std::make_shared<CloudMediaAssetDownloadOperation>();
        MEDIA_INFO_LOG("create cloud media asset task.");
    }
    return instance_;
}

bool CloudMediaAssetDownloadOperation::IsProperFgTemperature()
{
    return CommonEventUtils::GetThermalLevel() <= PROPER_DEVICE_TEMPERATURE_LEVEL_HOT;
}

void CloudMediaAssetDownloadOperation::SetTaskStatus(Status status)
{
    std::vector<int32_t> statusChangeVec = STATUS_MAP.at(status);
    CHECK_AND_RETURN_LOG(static_cast<int32_t>(statusChangeVec.size()) == STATUS_CHANGE_ARG_SIZE,
        "change status failed.");
    if (statusChangeVec[INDEX_ZERO] >= 0) {
        downloadType_ = static_cast<CloudMediaDownloadType>(statusChangeVec[INDEX_ZERO]);
    }
    taskStatus_ = static_cast<CloudMediaAssetTaskStatus>(statusChangeVec[INDEX_ONE]);
    pauseCause_ = static_cast<CloudMediaTaskPauseCause>(statusChangeVec[INDEX_TWO]);
    MEDIA_INFO_LOG("SetTaskStatus, downloadType_: %{public}d, taskStatus_: %{public}d, pauseCause_: %{public}d",
        statusChangeVec[INDEX_ZERO], statusChangeVec[INDEX_ONE], statusChangeVec[INDEX_TWO]);
}

void CloudMediaAssetDownloadOperation::ClearData(CloudMediaAssetDownloadOperation::DownloadFileData &data)
{
    data.pathVec.clear();
    data.fileDownloadMap.Clear();
    data.batchFileIdNeedDownload.clear();
    data.batchSizeNeedDownload = 0;
    data.batchCountNeedDownload = 0;
}

bool CloudMediaAssetDownloadOperation::IsDataEmpty(CloudMediaAssetDownloadOperation::DownloadFileData &data)
{
    return data.fileDownloadMap.IsEmpty();
}

std::shared_ptr<NativeRdb::ResultSet> CloudMediaAssetDownloadOperation::QueryDownloadFilesNeeded(
    const bool &isQueryInfo)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, nullptr, "QueryDownloadFilesNeeded failed. rdbStore is null");

    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_SYNC_STATUS, to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE)));
    predicates.EqualTo(PhotoColumn::PHOTO_CLEAN_FLAG, to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN)));
    predicates.EqualTo(MediaColumn::MEDIA_TIME_PENDING, "0");
    predicates.EqualTo(PhotoColumn::PHOTO_IS_TEMP, "0");
    predicates.IsNotNull(MediaColumn::MEDIA_FILE_PATH);
    predicates.EqualTo(PhotoColumn::PHOTO_POSITION, to_string(static_cast<int32_t>(PhotoPositionType::CLOUD)));
    predicates.BeginWrap();
    predicates.EqualTo(MediaColumn::MEDIA_TYPE, to_string(static_cast<int32_t>(MEDIA_TYPE_IMAGE)));
    predicates.Or();
    predicates.EqualTo(MediaColumn::MEDIA_TYPE, to_string(static_cast<int32_t>(MEDIA_TYPE_VIDEO)));
    predicates.EndWrap();
    if (isQueryInfo) {
        predicates.EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL,
            to_string(static_cast<int32_t>(BurstCoverLevelType::COVER)));
        const std::vector<std::string> columns = {
            TOTAL_COUNT,
            TOTAL_SIZE
        };
        return rdbStore->Query(predicates, columns);
    }
    if (static_cast<int32_t>(dataForDownload_.batchFileIdNeedDownload.size()) > 0) {
        predicates.NotIn(PhotoColumn::MEDIA_ID, dataForDownload_.batchFileIdNeedDownload);
    }
    predicates.OrderByDesc(MediaColumn::MEDIA_DATE_MODIFIED);
    predicates.Limit(BATCH_DOWNLOAD_CLOUD_FILE);
    const std::vector<std::string> columns = {
        MediaColumn::MEDIA_ID,
        MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_SIZE,
        PhotoColumn::PHOTO_BURST_COVER_LEVEL,
        MediaColumn::MEDIA_NAME
    };
    return rdbStore->Query(predicates, columns);
}

int32_t CloudMediaAssetDownloadOperation::InitDownloadTaskInfo()
{
    if (!isThumbnailUpdate_) {
        MEDIA_INFO_LOG("No need to update InitDownloadTaskInfo.");
        return E_OK;
    }
    std::shared_ptr<NativeRdb::ResultSet> resultSetForInfo = QueryDownloadFilesNeeded(true);
    bool cond = (resultSetForInfo == nullptr || resultSetForInfo->GoToNextRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_RET_LOG(!cond, E_ERR, "queryResult is invalid!");
    int32_t count = GetInt32Val(TOTAL_COUNT, resultSetForInfo);
    CHECK_AND_RETURN_RET_LOG(count != 0, E_ERR, "no cloud media asset need to download");
    int64_t size = GetInt64Val(TOTAL_SIZE, resultSetForInfo);

    int64_t hasDownloadNum_ = totalCount_ - remainCount_;
    int64_t hasDownloadSize_ = totalSize_ - remainSize_;
    remainCount_ = count;
    remainSize_ = size;
    totalCount_ = remainCount_ + hasDownloadNum_;
    totalSize_ = remainSize_ + hasDownloadSize_;
    isThumbnailUpdate_ = false;
    resultSetForInfo->Close();
    MEDIA_INFO_LOG("GetTaskInfo: %{public}s", GetTaskInfo().c_str());
    return E_OK;
}

CloudMediaAssetDownloadOperation::DownloadFileData CloudMediaAssetDownloadOperation::ReadyDataForBatchDownload()
{
    MEDIA_INFO_LOG("enter ReadyDataForBatchDownload");
    InitDownloadTaskInfo();

    CloudMediaAssetDownloadOperation::DownloadFileData data;
    std::shared_ptr<NativeRdb::ResultSet> resultSetForDownload = QueryDownloadFilesNeeded(false);
    CHECK_AND_RETURN_RET_LOG(resultSetForDownload != nullptr, data, "resultSetForDownload is nullptr.");

    while (resultSetForDownload->GoToNextRow() == NativeRdb::E_OK) {
        std::string fileId = GetStringVal(MediaColumn::MEDIA_ID, resultSetForDownload);
        std::string path = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSetForDownload);
        std::string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSetForDownload);
        std::string fileUri = MediaFileUri::GetPhotoUri(fileId, path, displayName);
        if (fileUri.empty()) {
            MEDIA_ERR_LOG("Failed to get fileUri, fileId: %{public}s, filePath: %{public}s, displayName: %{public}s.",
                fileId.c_str(), MediaFileUtils::DesensitizePath(path).c_str(), displayName.c_str());
            continue;
        }
        int64_t fileSize = GetInt64Val(PhotoColumn::MEDIA_SIZE, resultSetForDownload);

        data.pathVec.push_back(fileUri);
        int32_t burstCoverLevel = GetInt32Val(PhotoColumn::PHOTO_BURST_COVER_LEVEL, resultSetForDownload);
        if (burstCoverLevel == static_cast<int32_t>(BurstCoverLevelType::COVER)) {
            data.fileDownloadMap.EnsureInsert(fileUri, fileSize);
            data.batchSizeNeedDownload += fileSize;
            data.batchCountNeedDownload++;
        } else {
            data.fileDownloadMap.EnsureInsert(fileUri, 0);
        }
        data.batchFileIdNeedDownload.push_back(fileId);
    }
    resultSetForDownload->Close();
    MEDIA_INFO_LOG("end ReadyDataForBatchDownload");
    return data;
}

void CloudMediaAssetDownloadOperation::StartFileCacheFailed()
{
    MEDIA_ERR_LOG("enter StartFileCacheFailed");
    SetTaskStatus(Status::PAUSE_FOR_CLOUD_ERROR);
    downloadId_ = DOWNLOAD_ID_DEFAULT;
    if (isCache_) {
        ClearData(cacheForDownload_);
    }
    ClearData(readyForDownload_);
    ClearData(dataForDownload_);
    isThumbnailUpdate_ = true;
    // prepare for data consumption time, don't move this line
    CloudMediaAssetDownloadOperation::DownloadFileData data = ReadyDataForBatchDownload();
    if  (IsDataEmpty(data)) {
        CancelDownloadTask();
        return;
    }
    readyForDownload_ = data;
}

void CloudMediaAssetDownloadOperation::StartBatchDownload()
{
    std::thread([this]() {
        int32_t ret = cloudSyncManager_.get().StartFileCache(dataForDownload_.pathVec, downloadId_,
            FieldKey::FIELDKEY_CONTENT, downloadCallback_);
        if (ret != E_OK || downloadId_ == DOWNLOAD_ID_DEFAULT) {
            MEDIA_ERR_LOG("failed to StartFileCache, ret: %{public}d, downloadId_: %{public}s.",
                ret, to_string(downloadId_).c_str());
            StartFileCacheFailed();
            return;
        }
        MEDIA_INFO_LOG("Success, downloadId: %{public}d, downloadNum: %{public}d, isCache: %{public}d.",
            static_cast<int32_t>(downloadId_), static_cast<int32_t>(dataForDownload_.fileDownloadMap.Size()),
            static_cast<int32_t>(isCache_));
        if (isCache_) {
            ClearData(cacheForDownload_);
            return;
        }
        CloudMediaAssetDownloadOperation::DownloadFileData data = ReadyDataForBatchDownload();
        CHECK_AND_RETURN_INFO_LOG(taskStatus_ != CloudMediaAssetTaskStatus::IDLE, "taskStatus_ is IDLE.");

        ClearData(readyForDownload_);
        readyForDownload_ = data;
    }).detach();
}

int32_t CloudMediaAssetDownloadOperation::SubmitBatchDownload(
    CloudMediaAssetDownloadOperation::DownloadFileData &data, const bool &isCache)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (taskStatus_ != CloudMediaAssetTaskStatus::DOWNLOADING || downloadId_ != DOWNLOAD_ID_DEFAULT) {
        MEDIA_INFO_LOG("SubmitBatchDownload permission denied, taskStatus_: %{public}d.",
            static_cast<int32_t>(taskStatus_));
        return E_ERR;
    }
    if (cloudRemoteObject_ == nullptr) {
        CHECK_AND_PRINT_LOG(SetDeathRecipient() == E_OK, "failed to register death recipient.");
    }
    isCache_ = isCache;
    if (IsDataEmpty(data)) {
        MEDIA_ERR_LOG("No data need to submit.");
        if (!isCache_) {
            CancelDownloadTask();
            return EXIT_TASK;
        }
        return E_OK;
    }
    dataForDownload_ = data;

    StartBatchDownload();
    return E_OK;
}

void CloudMediaAssetDownloadOperation::InitStartDownloadTaskStatus(const bool &isForeground)
{
    if (!isForeground && !MedialibraryRelatedSystemStateManager::GetInstance()->IsWifiConnected()) {
        MEDIA_WARN_LOG("Failed to init startDownloadTaskStatus, wifi is not connected.");
        SetTaskStatus(Status::PAUSE_FOR_BACKGROUND_TASK_UNAVAILABLE);
        return;
    }

    if (isForeground && !IsProperFgTemperature()) {
        SetTaskStatus(Status::PAUSE_FOR_TEMPERATURE_LIMIT);
        MEDIA_ERR_LOG("Temperature is not suitable for foreground downloads.");
        return;
    }
    if (!MedialibraryRelatedSystemStateManager::GetInstance()->IsNetAvailableWithUnlimitCondition()) {
        Status status = MedialibraryRelatedSystemStateManager::GetInstance()->IsCellularNetConnected() ?
            Status::PAUSE_FOR_WIFI_UNAVAILABLE : Status::PAUSE_FOR_NETWORK_FLOW_LIMIT;
        SetTaskStatus(status);
        MEDIA_ERR_LOG("No wifi and no cellular data.");
        return;
    }
}

int32_t CloudMediaAssetDownloadOperation::SetDeathRecipient()
{
    auto saMgr = OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHECK_AND_RETURN_RET_LOG(saMgr != nullptr, E_ERR, "Failed to get SystemAbilityManagerClient.");

    cloudRemoteObject_ = saMgr->CheckSystemAbility(CLOUD_MANAGER_MANAGER_ID);
    if (cloudRemoteObject_ == nullptr) {
        MEDIA_INFO_LOG("try to load CloudFilesService SystemAbility");
        int32_t minTimeout = 4;
        cloudRemoteObject_ = saMgr->LoadSystemAbility(CLOUD_MANAGER_MANAGER_ID, minTimeout);
        CHECK_AND_RETURN_RET_LOG(cloudRemoteObject_ != nullptr, E_ERR, "cloudRemoteObject_ is null.");
    }
    
    CHECK_AND_RETURN_RET_LOG(cloudRemoteObject_->AddDeathRecipient(sptr(new CloudDeathRecipient(instance_))),
        E_ERR, "Failed to add death recipient.");
    return E_OK;
}

int32_t CloudMediaAssetDownloadOperation::RegisterNetObserver()
{
    netObserver_ = new (std::nothrow) NetConnectObserver();
    CHECK_AND_RETURN_RET_LOG(netObserver_ != nullptr, E_ERR, "Failed to get netObserver.");
    int32_t ret = NetConnClient::GetInstance().RegisterNetConnCallback(netObserver_);
    CHECK_AND_RETURN_RET_LOG(ret == NETMANAGER_SUCCESS, E_ERR, "Failed to register netObserver.");
    return E_OK;
}

int32_t CloudMediaAssetDownloadOperation::DoRelativedRegister()
{
    // register unlimit traffic status
    CreateOptions options;
    options.enabled_ = true;
    cloudHelper_ = DataShare::DataShareHelper::Creator(CLOUD_DATASHARE_URI, options);
    CHECK_AND_RETURN_RET_LOG(cloudHelper_ != nullptr, E_ERR, "cloudHelper_ is null.");

    cloudMediaAssetObserver_ = std::make_shared<CloudMediaAssetObserver>(instance_);
    CHECK_AND_RETURN_RET_LOG(cloudMediaAssetObserver_ != nullptr, E_ERR, "cloudMediaAssetObserver_ is null.");
    // observer more than 50, failed to register
    cloudHelper_->RegisterObserverExt(Uri(CLOUD_URI), cloudMediaAssetObserver_, true);

    // observer download callback
    downloadCallback_ = std::make_shared<MediaCloudDownloadCallback>(instance_);
    CHECK_AND_RETURN_RET_LOG(downloadCallback_ != nullptr, E_ERR, "downloadCallback_ is null.");

    int32_t ret = SetDeathRecipient();
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "failed to register death recipient, ret: %{public}d.", ret);

    ret = RegisterNetObserver();
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "failed to register netObserver");

    MEDIA_INFO_LOG("success to register");
    return ret;
}

int32_t CloudMediaAssetDownloadOperation::DoForceTaskExecute()
{
    CHECK_AND_RETURN_RET_LOG(taskStatus_ != CloudMediaAssetTaskStatus::IDLE, E_ERR,
        "DoForceTaskExecute permission denied");
    if (taskStatus_ == CloudMediaAssetTaskStatus::PAUSED) {
        MEDIA_INFO_LOG("pause cause is %{public}d", static_cast<int32_t>(pauseCause_));
        readyForDownload_ = ReadyDataForBatchDownload();
        if (IsDataEmpty(readyForDownload_)) {
            MEDIA_ERR_LOG("no data need to download, cancel download task");
            CancelDownloadTask();
        }
        return E_OK;
    }
    CloudMediaAssetDownloadOperation::DownloadFileData data = ReadyDataForBatchDownload();
    return SubmitBatchDownload(data, false);
}

int32_t CloudMediaAssetDownloadOperation::StartDownloadTask(int32_t cloudMediaDownloadType)
{
    MediaLibraryTracer tracer;
    tracer.Start("StartDownloadTask");
    CHECK_AND_RETURN_RET_LOG(taskStatus_ == CloudMediaAssetTaskStatus::IDLE, E_ERR,
        "StartDownloadTask permission denied");
    MEDIA_INFO_LOG("enter, download type: %{public}d", cloudMediaDownloadType);
    int32_t ret = DoRelativedRegister();
    CHECK_AND_RETURN_RET(ret == E_OK, ret);

    if (cloudMediaDownloadType == static_cast<int32_t>(CloudMediaDownloadType::DOWNLOAD_FORCE)) {
        SetTaskStatus(Status::FORCE_DOWNLOADING);
        InitStartDownloadTaskStatus(true);
        return DoForceTaskExecute();
    }
    SetTaskStatus(Status::PAUSE_FOR_BACKGROUND_TASK_UNAVAILABLE);
    InitStartDownloadTaskStatus(false);
    InitDownloadTaskInfo();
    readyForDownload_ = ReadyDataForBatchDownload();
    if (IsDataEmpty(readyForDownload_)) {
        MEDIA_ERR_LOG("no data need to download, cancel download task");
        CancelDownloadTask();
    }
    return E_OK;
}

int32_t CloudMediaAssetDownloadOperation::DoRecoverExecute()
{
    CHECK_AND_RETURN_RET_LOG(IsDataEmpty(dataForDownload_), E_ERR, "callback for cache is still alive.");
    CHECK_AND_RETURN_RET(!IsDataEmpty(cacheForDownload_), SubmitBatchDownload(readyForDownload_, false));
    return SubmitBatchDownload(cacheForDownload_, true);
}

int32_t CloudMediaAssetDownloadOperation::ManualActiveRecoverTask(int32_t cloudMediaDownloadType)
{
    MEDIA_INFO_LOG("enter ManualActiveRecoverTask.");
    CHECK_AND_RETURN_RET_LOG(taskStatus_ == CloudMediaAssetTaskStatus::PAUSED, E_ERR,
        "ManualActiveRecoverTask permission denied");

    if (cloudMediaDownloadType == static_cast<int32_t>(CloudMediaDownloadType::DOWNLOAD_FORCE)) {
        SetTaskStatus(Status::RECOVER_FOR_MANAUL_ACTIVE);
        InitStartDownloadTaskStatus(true);
        return DoRecoverExecute();
    }
    SetTaskStatus(Status::PAUSE_FOR_BACKGROUND_TASK_UNAVAILABLE);
    return E_OK;
}

int32_t CloudMediaAssetDownloadOperation::PassiveStatusRecover()
{
    if (downloadType_ == CloudMediaDownloadType::DOWNLOAD_GENTLE && !isBgDownloadPermission_) {
        SetTaskStatus(Status::PAUSE_FOR_BACKGROUND_TASK_UNAVAILABLE);
        return E_OK;
    }
    SetTaskStatus(Status::RECOVER_FOR_PASSIVE_STATUS);
    if (downloadType_ == CloudMediaDownloadType::DOWNLOAD_FORCE) {
        InitStartDownloadTaskStatus(true);
    } else {
        InitStartDownloadTaskStatus(false);
    }
    return DoRecoverExecute();
}

int32_t CloudMediaAssetDownloadOperation::PassiveStatusRecoverTask(const CloudMediaTaskRecoverCause &recoverCause)
{
    bool cond = (taskStatus_ != CloudMediaAssetTaskStatus::PAUSED ||
                pauseCause_ == CloudMediaTaskPauseCause::USER_PAUSED);
    CHECK_AND_RETURN_RET_LOG(!cond, E_ERR,
        "PassiveStatusRecoverTask permission denied, taskStatus: %{public}d, pauseCause: %{public}d,",
            static_cast<int32_t>(taskStatus_), static_cast<int32_t>(pauseCause_));

    if (recoverCause == CloudMediaTaskRecoverCause::NETWORK_NORMAL &&
        (pauseCause_ == CloudMediaTaskPauseCause::WIFI_UNAVAILABLE ||
        pauseCause_ == CloudMediaTaskPauseCause::NETWORK_FLOW_LIMIT ||
        pauseCause_ == CloudMediaTaskPauseCause::BACKGROUND_TASK_UNAVAILABLE)) {
        downloadId_ = DOWNLOAD_ID_DEFAULT; // wifi recovery, submit
        return PassiveStatusRecover();
    }

    if (RECOVER_RELATIONSHIP_MAP.find(recoverCause) == RECOVER_RELATIONSHIP_MAP.end() ||
        pauseCause_ != RECOVER_RELATIONSHIP_MAP.at(recoverCause)) {
        MEDIA_INFO_LOG("recoverCause is error, recoverCause: %{public}d", static_cast<int32_t>(recoverCause));
        return E_ERR;
    }
    return PassiveStatusRecover();
}

static bool IsStorageSufficient()
{
    struct statvfs diskInfo;
    int ret = statvfs(STORAGE_PATH.c_str(), &diskInfo);
    CHECK_AND_RETURN_RET_LOG(ret == 0, false, "Get file system status information failed, err: %{public}d", ret);

    double totalSize = static_cast<double>(diskInfo.f_bsize) * static_cast<double>(diskInfo.f_blocks);
    CHECK_AND_RETURN_RET_LOG(totalSize >= 1e-9, false,
        "Get file system total size failed, totalSize=%{public}f", totalSize);

    double freeSize = static_cast<double>(diskInfo.f_bsize) * static_cast<double>(diskInfo.f_bfree);
    double freeRatio = freeSize / totalSize;
    MEDIA_INFO_LOG("Get freeRatio, freeRatio= %{public}f", freeRatio);

    return freeRatio > PROPER_DEVICE_STORAGE_CAPACITY_RATIO;
}

void CloudMediaAssetDownloadOperation::CheckStorageAndRecoverDownloadTask()
{
    if (IsStorageSufficient()) {
        MEDIA_INFO_LOG("storage is sufficient, begin to recover downloadTask.");
        PassiveStatusRecoverTask(CloudMediaTaskRecoverCause::STORAGE_NORMAL);
    }
}

int32_t CloudMediaAssetDownloadOperation::PauseDownloadTask(const CloudMediaTaskPauseCause &pauseCause)
{
    MediaLibraryTracer tracer;
    tracer.Start("PauseDownloadTask");

    bool cond = (taskStatus_ == CloudMediaAssetTaskStatus::IDLE ||
                pauseCause_ == CloudMediaTaskPauseCause::USER_PAUSED);
    CHECK_AND_RETURN_RET_LOG(!cond, E_ERR, "PauseDownloadTask permission denied");

    cond = (pauseCause_ == CloudMediaTaskPauseCause::BACKGROUND_TASK_UNAVAILABLE &&
            pauseCause != CloudMediaTaskPauseCause::USER_PAUSED);
    CHECK_AND_RETURN_RET_LOG(!cond, E_ERR,
        "PauseDownloadTask permission denied, pauseCause_ is BACKGROUND_TASK_UNAVAILABLE");
    MEDIA_INFO_LOG("enter PauseDownloadTask, taskStatus_: %{public}d, pauseCause_: %{public}d, pauseCause: %{public}d",
        static_cast<int32_t>(taskStatus_), static_cast<int32_t>(pauseCause_), static_cast<int32_t>(pauseCause));

    pauseCause_ = pauseCause;
    if (taskStatus_ == CloudMediaAssetTaskStatus::DOWNLOADING) {
        taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
        if (downloadId_ != DOWNLOAD_ID_DEFAULT) {
            cloudSyncManager_.get().StopFileCache(downloadId_, !NEED_CLEAN);
            MEDIA_INFO_LOG("success StopFileCache.");
        }
    }
    return E_OK;
}

void CloudMediaAssetDownloadOperation::ResetParameter()
{
    ClearData(readyForDownload_);
    ClearData(notFoundForDownload_);
    ClearData(cacheForDownload_);
    downloadId_ = DOWNLOAD_ID_DEFAULT;
    ClearData(dataForDownload_);

    isThumbnailUpdate_ = true;
    isBgDownloadPermission_ = false;

    totalCount_ = 0;
    totalSize_ = 0;
    remainCount_ = 0;
    remainSize_ = 0;
    downloadTryTime_ = 0;
}

int32_t CloudMediaAssetDownloadOperation::CancelDownloadTask()
{
    CHECK_AND_RETURN_RET_LOG(taskStatus_ != CloudMediaAssetTaskStatus::IDLE, E_ERR,
        "CancelDownloadTask permission denied");
    MEDIA_ERR_LOG("the number of not found assets: %{public}d",
        static_cast<int32_t>(notFoundForDownload_.fileDownloadMap.Size()));
    SetTaskStatus(Status::IDLE);
    if (downloadId_ != DOWNLOAD_ID_DEFAULT) {
        cloudSyncManager_.get().StopFileCache(downloadId_, NEED_CLEAN);
    }
    ResetParameter();
    downloadCallback_ = nullptr;
    cloudRemoteObject_ = nullptr;
    if (cloudHelper_ != nullptr) {
        cloudHelper_->UnregisterObserverExt(Uri(CLOUD_URI), cloudMediaAssetObserver_);
        cloudHelper_ = nullptr;
    }
    cloudMediaAssetObserver_ = nullptr;
    if (netObserver_ != nullptr) {
        NetConnClient::GetInstance().UnregisterNetConnCallback(netObserver_);
        netObserver_ = nullptr;
    }
    return E_OK;
}

int32_t CloudMediaAssetDownloadOperation::SubmitBatchDownloadAgain()
{
    CHECK_AND_RETURN_RET(IsDataEmpty(dataForDownload_), E_ERR);
    MEDIA_INFO_LOG("Submit batchDownload again.");
    downloadId_ = DOWNLOAD_ID_DEFAULT;
    CHECK_AND_RETURN_RET(!IsDataEmpty(cacheForDownload_), SubmitBatchDownload(readyForDownload_, false));
    return SubmitBatchDownload(cacheForDownload_, true);
}

void CloudMediaAssetDownloadOperation::HandleSuccessCallback(const DownloadProgressObj& progress)
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    MediaLibraryTracer tracer;
    tracer.Start("HandleSuccessCallback");
    int64_t size = 0;
    if (progress.downloadId != downloadId_ || !dataForDownload_.fileDownloadMap.Find(progress.path, size)) {
        MEDIA_WARN_LOG("this path is unknown, path: %{public}s, downloadId: %{public}s, downloadId_: %{public}s.",
            MediaFileUtils::DesensitizeUri(progress.path).c_str(), to_string(progress.downloadId).c_str(),
            to_string(downloadId_).c_str());
        return;
    }
    
    if (size != 0) {
        remainCount_--;
        remainSize_ -= size;
        if (remainCount_ < 0 || remainSize_ < 0) {
            MEDIA_INFO_LOG("Update count and size of download cloud media asset.");
            isThumbnailUpdate_ = true;
            InitDownloadTaskInfo();
        }
    }
    dataForDownload_.fileDownloadMap.Erase(progress.path);

    MEDIA_INFO_LOG("success, path: %{public}s, size: %{public}s, batchSuccNum: %{public}s.",
        MediaFileUtils::DesensitizeUri(progress.path).c_str(), to_string(size).c_str(),
        to_string(progress.batchSuccNum).c_str());

    SubmitBatchDownloadAgain();
}

void CloudMediaAssetDownloadOperation::MoveDownloadFileToCache(const DownloadProgressObj& progress,
    const bool tryDownload)
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    int64_t downloadSize = 0;
    if (progress.downloadId != downloadId_ || !dataForDownload_.fileDownloadMap.Find(progress.path, downloadSize)) {
        MEDIA_WARN_LOG("This file is unknown, path: %{public}s, downloadId: %{public}s, downloadId_: %{public}s.",
            MediaFileUtils::DesensitizeUri(progress.path).c_str(), to_string(progress.downloadId).c_str(),
            to_string(downloadId_).c_str());
        return;
    }
    int64_t cacheSize = 0;
    CHECK_AND_RETURN_INFO_LOG(!cacheForDownload_.fileDownloadMap.Find(progress.path, cacheSize),
        "file is in fileDownloadCacheMap_, path: %{public}s.", MediaFileUtils::DesensitizeUri(progress.path).c_str());

    cacheForDownload_.pathVec.push_back(progress.path);
    cacheForDownload_.fileDownloadMap.EnsureInsert(progress.path, downloadSize);
    dataForDownload_.fileDownloadMap.Erase(progress.path);
    MEDIA_INFO_LOG("success, path: %{public}s.", MediaFileUtils::DesensitizeUri(progress.path).c_str());
    if (IsDataEmpty(dataForDownload_) && tryDownload) {
        downloadTryTime_++;
        MEDIA_INFO_LOG("try to download, downloadTryTime_: %{public}u", downloadTryTime_);
    }
    SubmitBatchDownloadAgain();
}

void CloudMediaAssetDownloadOperation::MoveDownloadFileToNotFound(const DownloadProgressObj& progress)
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    int64_t downloadSize = 0;
    bool cond = (progress.downloadId != downloadId_ ||
        !dataForDownload_.fileDownloadMap.Find(progress.path, downloadSize));
    CHECK_AND_RETURN_LOG(!cond, "This file is unknown, path: %{public}s, downloadId: %{public}s,"
        " downloadId_: %{public}s.", MediaFileUtils::DesensitizeUri(progress.path).c_str(),
        to_string(progress.downloadId).c_str(), to_string(downloadId_).c_str());

    int64_t notFoundSize = 0;
    CHECK_AND_RETURN_INFO_LOG(!notFoundForDownload_.fileDownloadMap.Find(progress.path, notFoundSize),
        "file is in notFoundForDownload_, path: %{public}s.", MediaFileUtils::DesensitizeUri(progress.path).c_str());

    notFoundForDownload_.fileDownloadMap.EnsureInsert(progress.path, downloadSize);
    dataForDownload_.fileDownloadMap.Erase(progress.path);
    MEDIA_INFO_LOG("success, path: %{public}s.", MediaFileUtils::DesensitizeUri(progress.path).c_str());
    SubmitBatchDownloadAgain();
}

void CloudMediaAssetDownloadOperation::HandleFailedCallback(const DownloadProgressObj& progress)
{
    MediaLibraryTracer tracer;
    tracer.Start("HandleFailedCallback");
    bool cond = (taskStatus_ == CloudMediaAssetTaskStatus::PAUSED &&
        pauseCause_ == CloudMediaTaskPauseCause::USER_PAUSED);
    CHECK_AND_RETURN_INFO_LOG(!cond, "pauseCause_ is USER_PAUSED");

    MEDIA_INFO_LOG("Download error type: %{public}d, path: %{public}s.", progress.downloadErrorType,
        MediaFileUtils::DesensitizeUri(progress.path).c_str());
    switch (progress.downloadErrorType) {
        case static_cast<int32_t>(DownloadProgressObj::DownloadErrorType::UNKNOWN_ERROR): {
            PauseDownloadTask(CloudMediaTaskPauseCause::CLOUD_ERROR);
            MoveDownloadFileToCache(progress);
            break;
        }
        case static_cast<int32_t>(DownloadProgressObj::DownloadErrorType::NETWORK_UNAVAILABLE): {
            if (!MedialibraryRelatedSystemStateManager::GetInstance()->IsNetAvailableWithUnlimitCondition()
                || downloadTryTime_ >= MAX_DOWNLOAD_TRY_TIMES) {
                PauseDownloadTask(CloudMediaTaskPauseCause::NETWORK_FLOW_LIMIT);
            }
            MoveDownloadFileToCache(progress, true);
            break;
        }
        case static_cast<int32_t>(DownloadProgressObj::DownloadErrorType::LOCAL_STORAGE_FULL): {
            PauseDownloadTask(CloudMediaTaskPauseCause::ROM_LIMIT);
            MoveDownloadFileToCache(progress);
            break;
        }
        case static_cast<int32_t>(DownloadProgressObj::DownloadErrorType::CONTENT_NOT_FOUND): {
            MoveDownloadFileToNotFound(progress);
            break;
        }
        case static_cast<int32_t>(DownloadProgressObj::DownloadErrorType::FREQUENT_USER_REQUESTS): {
            PauseDownloadTask(CloudMediaTaskPauseCause::FREQUENT_USER_REQUESTS);
            MoveDownloadFileToCache(progress);
            break;
        }
        default: {
            MEDIA_WARN_LOG("download error type not exit.");
            break;
        }
    }
}

void CloudMediaAssetDownloadOperation::HandleStoppedCallback(const DownloadProgressObj& progress)
{
    MediaLibraryTracer tracer;
    tracer.Start("HandleStoppedCallback");
    MEDIA_INFO_LOG("enter DownloadStopped, path: %{public}s.", MediaFileUtils::DesensitizeUri(progress.path).c_str());
    MoveDownloadFileToCache(progress);
}

CloudMediaDownloadType CloudMediaAssetDownloadOperation::GetDownloadType()
{
    return downloadType_;
}

CloudMediaAssetTaskStatus CloudMediaAssetDownloadOperation::GetTaskStatus()
{
    return taskStatus_;
}

CloudMediaTaskPauseCause CloudMediaAssetDownloadOperation::GetTaskPauseCause()
{
    return pauseCause_;
}

std::string CloudMediaAssetDownloadOperation::GetTaskInfo()
{
    return to_string(totalCount_) + "," + to_string(totalSize_) + "," +
        to_string(remainCount_) + "," + to_string(remainSize_);
}

void CloudMediaAssetDownloadOperation::ResetDownloadTryTime()
{
    downloadTryTime_ = 0;
}

void CloudMediaAssetDownloadOperation::HandleOnRemoteDied()
{
    cloudRemoteObject_ = nullptr;
    CHECK_AND_RETURN_LOG(taskStatus_ == CloudMediaAssetTaskStatus::DOWNLOADING, "taskStatus is not DOWNLOADING");
    CancelDownloadTask();
}

} // namespace Media
} // namespace OHOS