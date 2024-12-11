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

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>
#include <memory>
#include <chrono>
#include <algorithm>
#include <map>

#include "common_event_utils.h"
#include "cloud_sync_common.h"
#include "cloud_sync_constants.h"
#include "cloud_sync_manager.h"
#include "cloud_sync_utils.h"
#include "datashare_helper.h"
#include "iservice_registry.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_command.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_operation.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_tracer.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "rdb_store.h"
#include "result_set_utils.h"
#include "wifi_device.h"
#include "thermal_mgr_client.h"
#include "userfile_manager_types.h"

using namespace std;
using namespace OHOS::DataShare;
using namespace OHOS::NativeRdb;
namespace OHOS {
namespace Media {
using namespace FileManagement::CloudSync;
using Status = CloudMediaAssetDownloadOperation::Status;
std::mutex CloudMediaAssetDownloadOperation::mutex_;
std::mutex CloudMediaAssetDownloadOperation::callbackMutex_;
std::shared_ptr<CloudMediaAssetDownloadOperation> CloudMediaAssetDownloadOperation::instance_ = nullptr;
static const int32_t PROPER_DEVICE_TEMPERATURE_LEVEL_HOT = 3;
static const int32_t BATCH_DOWNLOAD_CLOUD_FILE = 10;
static constexpr int STORAGE_MANAGER_MANAGER_ID = 5003;
static constexpr int CLOUD_MANAGER_MANAGER_ID = 5204;
static const std::string CLOUD_DATASHARE_URI = "datashareproxy://generic.cloudstorage/cloud_sp?Proxy=true";
static const std::string CLOUD_URI = CLOUD_DATASHARE_URI + "&key=useMobileNetworkData";
static const int64_t DOWNLOAD_ID_DEFAULT = -1;
static const std::string TOTAL_COUNT = "COUNT(1)";
static const std::string TOTAL_SIZE = "SUM(size)";
static const bool NEED_CLEAN = true;
static const int32_t EXIT_TASK = 1;
// E_PATH_NOT_FOUND corresponds to the E_PATH of dfs_error.h
static const int32_t E_PATH_NOT_FOUND = 28;
static const int32_t SLEEP_FOR_LOCK = 100;
static const int32_t STATUS_CHANGE_ARG_SIZE = 3;
static const int32_t INDEX_ZERO = 0;
static const int32_t INDEX_ONE = 1;
static const int32_t INDEX_TWO = 2;

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
    { CloudMediaTaskRecoverCause::NETWORK_FLOW_UNLIMIT, CloudMediaTaskPauseCause::NETWORK_FLOW_LIMIT },
    { CloudMediaTaskRecoverCause::BACKGROUND_TASK_AVAILABLE, CloudMediaTaskPauseCause::BACKGROUND_TASK_UNAVAILABLE },
    { CloudMediaTaskRecoverCause::RETRY_FOR_FREQUENT_REQUESTS, CloudMediaTaskPauseCause::FREQUENT_USER_REQUESTS },
    { CloudMediaTaskRecoverCause::RETRY_FOR_CLOUD_ERROR, CloudMediaTaskPauseCause::CLOUD_ERROR },
};

void CloudDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    MEDIA_INFO_LOG("enter.");
    if (!operation_ || operation_->GetTaskStatus() != CloudMediaAssetTaskStatus::DOWNLOADING) {
        MEDIA_ERR_LOG("operation is nullptr or taskStatus is not DOWNLOADING");
        return;
    }
    if (object == nullptr) {
        MEDIA_ERR_LOG("remote object is nullptr");
        return;
    }
    object->RemoveDeathRecipient(this);
    operation_->CancelDownloadTask();
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
    if (static_cast<int32_t>(statusChangeVec.size()) != STATUS_CHANGE_ARG_SIZE) {
        MEDIA_ERR_LOG("change status failed.");
        return;
    }
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
    data.fileDownloadMap.clear();
    data.batchFileIdNeedDownload.clear();
    data.batchSizeNeedDownload = 0;
    data.batchCountNeedDownload = 0;
}

bool CloudMediaAssetDownloadOperation::IsDataEmpty(const CloudMediaAssetDownloadOperation::DownloadFileData &data)
{
    return data.fileDownloadMap.empty();
}

std::shared_ptr<NativeRdb::ResultSet> CloudMediaAssetDownloadOperation::QueryDownloadFilesNeeded(
    const bool &isQueryInfo)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("QueryDownloadFilesNeeded failed. rdbStore is null");
        return nullptr;
    }
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_CLEAN_FLAG, to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN)));
    predicates.EqualTo(PhotoColumn::PHOTO_POSITION, to_string(static_cast<int32_t>(PhotoPositionType::CLOUD)));
    predicates.EqualTo(MediaColumn::MEDIA_TIME_PENDING, "0");
    predicates.EqualTo(PhotoColumn::PHOTO_IS_TEMP, "0");
    predicates.IsNotNull(MediaColumn::MEDIA_FILE_PATH);
    if (static_cast<int32_t>(dataForDownload_.batchFileIdNeedDownload.size()) > 0) {
        predicates.NotIn(PhotoColumn::MEDIA_ID, dataForDownload_.batchFileIdNeedDownload);
    }
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
    predicates.OrderByDesc(MediaColumn::MEDIA_DATE_MODIFIED);
    predicates.Limit(BATCH_DOWNLOAD_CLOUD_FILE);
    const std::vector<std::string> columns = {
        MediaColumn::MEDIA_ID,
        MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_SIZE,
        PhotoColumn::PHOTO_BURST_COVER_LEVEL
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
    if (resultSetForInfo == nullptr || resultSetForInfo->GoToNextRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("queryResult is invalid!");
        return E_ERR;
    }
    int32_t count = GetInt32Val(TOTAL_COUNT, resultSetForInfo);
    if (count == 0) {
        MEDIA_ERR_LOG("no cloud media asset need to download");
        return E_ERR;
    }
    int64_t size = GetInt64Val(TOTAL_SIZE, resultSetForInfo);
    totalCount_ = count + batchDownloadTotalNum_;
    totalSize_ = size + batchDownloadTotalSize_;
    remainCount_ = totalCount_ - hasDownloadNum_;
    remainSize_ = totalSize_ - hasDownloadSize_;

    isThumbnailUpdate_ = false;
    resultSetForInfo->Close();
    MEDIA_INFO_LOG("success InitDownloadTaskInfo.");
    return E_OK;
}

CloudMediaAssetDownloadOperation::DownloadFileData CloudMediaAssetDownloadOperation::ReadyDataForBatchDownload()
{
    MEDIA_INFO_LOG("enter ReadyDataForBatchDownload");
    InitDownloadTaskInfo();

    CloudMediaAssetDownloadOperation::DownloadFileData data;
    std::shared_ptr<NativeRdb::ResultSet> resultSetForDownload = QueryDownloadFilesNeeded(false);
    if (resultSetForDownload == nullptr) {
        MEDIA_ERR_LOG("resultSetForDownload is nullptr.");
        return data;
    }

    while (resultSetForDownload->GoToNextRow() == NativeRdb::E_OK) {
        std::string fileId = GetStringVal(MediaColumn::MEDIA_ID, resultSetForDownload);
        std::string path = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSetForDownload);
        if (fileId.empty() || path.empty()) {
            MEDIA_ERR_LOG("empty fileId or filePath, fileId: %{public}s, filePath: %{public}s.",
                fileId.c_str(), MediaFileUtils::DesensitizePath(path).c_str());
            continue;
        }
        int64_t fileSize = GetInt64Val(PhotoColumn::MEDIA_SIZE, resultSetForDownload);

        data.pathVec.push_back(path);
        int32_t burstCoverLevel = GetInt32Val(PhotoColumn::PHOTO_BURST_COVER_LEVEL, resultSetForDownload);
        if (burstCoverLevel == static_cast<int32_t>(BurstCoverLevelType::COVER)) {
            data.fileDownloadMap[path] = fileSize;
            data.batchSizeNeedDownload += fileSize;
            data.batchCountNeedDownload++;
        } else {
            data.fileDownloadMap[path] = 0;
        }
        data.batchFileIdNeedDownload.push_back(fileId);
    }
    resultSetForDownload->Close();
    MEDIA_INFO_LOG("end ReadyDataForBatchDownload");
    return data;
}

void CloudMediaAssetDownloadOperation::StartFileCacheFailed(const int64_t batchNum, const int64_t batchSize)
{
    downloadId_ = DOWNLOAD_ID_DEFAULT;
    downloadNum_ = 0;
    if (isCache_) {
        ClearData(cacheForDownload_);
    }
    ClearData(readyForDownload_);
    ClearData(dataForDownload_);
    batchDownloadTotalNum_ -= batchNum;
    batchDownloadTotalSize_ -= batchSize;
    if (!isThumbnailUpdate_) {
        isThumbnailUpdate_ = true;
    }
    InitDownloadTaskInfo();
    // prepare for data consumption time, don't move this line
    CloudMediaAssetDownloadOperation::DownloadFileData data = ReadyDataForBatchDownload();
    if  (IsDataEmpty(data)) {
        CancelDownloadTask();
        return;
    }
    readyForDownload_ = data;
    // prepare the data and then set task status
    SetTaskStatus(Status::PAUSE_FOR_CLOUD_ERROR);
}

void CloudMediaAssetDownloadOperation::StartBatchDownload(const int64_t batchNum, const int64_t batchSize)
{
    std::thread([this, batchNum, batchSize]() {
        this_thread::sleep_for(chrono::milliseconds(SLEEP_FOR_LOCK));
        int32_t ret = cloudSyncManager_.get().StartFileCache(dataForDownload_.pathVec, downloadId_);
        if (ret == E_RDB || ret == E_PATH_NOT_FOUND) {
            MEDIA_INFO_LOG("failed to StartFileCache, ret: %{public}d, downloadId_: %{public}s.",
                ret, to_string(downloadId_).c_str());
            StartFileCacheFailed(batchNum, batchSize);
            return;
        }
        if (ret != E_OK || downloadId_ == DOWNLOAD_ID_DEFAULT) {
            MEDIA_ERR_LOG("failed to StartFileCache, ret: %{public}d, downloadId_: %{public}s.",
                ret, to_string(downloadId_).c_str());
            downloadId_ = DOWNLOAD_ID_DEFAULT;
            downloadNum_ = 0;
            cacheForDownload_ = dataForDownload_;
            SetTaskStatus(Status::PAUSE_FOR_CLOUD_ERROR);
            return;
        }
        MEDIA_INFO_LOG("Success, downloadId: %{public}d, downloadNum: %{public}d, isCache: %{public}d.",
            static_cast<int32_t>(downloadId_), static_cast<int32_t>(downloadNum_), static_cast<int32_t>(isCache_));
        if (isCache_) {
            ClearData(cacheForDownload_);
            return;
        }
        CloudMediaAssetDownloadOperation::DownloadFileData data = ReadyDataForBatchDownload();
        if (taskStatus_ == CloudMediaAssetTaskStatus::IDLE) {
            MEDIA_INFO_LOG("taskStatus_ is IDLE.");
            return;
        }
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
    isCache_ = isCache;
    if (IsDataEmpty(data)) {
        MEDIA_INFO_LOG("No data need to submit.");
        if (!isCache_) {
            CancelDownloadTask();
            return EXIT_TASK;
        }
        return E_OK;
    }

    downloadNum_ = static_cast<int64_t>(data.pathVec.size());
    dataForDownload_ = data;
    if (!isCache_) {
        batchDownloadTotalNum_ += data.batchCountNeedDownload;
        batchDownloadTotalSize_ += data.batchSizeNeedDownload;
    }

    StartBatchDownload(data.batchCountNeedDownload, data.batchSizeNeedDownload);
    return E_OK;
}

void CloudMediaAssetDownloadOperation::InitStartDownloadTaskStatus(const bool &isForeground)
{
    if (isForeground && !IsProperFgTemperature()) {
        SetTaskStatus(Status::PAUSE_FOR_TEMPERATURE_LIMIT);
        MEDIA_ERR_LOG("Temperature is not suitable for foreground downloads.");
        return;
    }
    if (!CommonEventUtils::IsWifiConnected() && !CloudSyncUtils::IsUnlimitedTrafficStatusOn()) {
        Status status = MedialibrarySubscriber::GetIsCellularNetConnected() ?
            Status::PAUSE_FOR_WIFI_UNAVAILABLE : Status::PAUSE_FOR_NETWORK_FLOW_LIMIT;
        SetTaskStatus(status);
        MEDIA_ERR_LOG("No wifi and no cellular data.");
        return;
    }
}

int32_t CloudMediaAssetDownloadOperation::SetDeathRecipient()
{
    auto saMgr = OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saMgr == nullptr) {
        MEDIA_ERR_LOG("Failed to get SystemAbilityManagerClient");
        return E_ERR;
    }
    cloudRemoteObject_ = saMgr->CheckSystemAbility(CLOUD_MANAGER_MANAGER_ID);
    if (cloudRemoteObject_ == nullptr) {
        MEDIA_ERR_LOG("Token is null.");
        return E_ERR;
    }
    if (!cloudRemoteObject_->AddDeathRecipient(sptr(new CloudDeathRecipient(instance_)))) {
        MEDIA_ERR_LOG("Failed to add death recipient.");
        return E_ERR;
    }
    return E_OK;
}

int32_t CloudMediaAssetDownloadOperation::DoRelativedRegister()
{
    // register unlimit traffic status
    auto saMgr = OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saMgr == nullptr) {
        MEDIA_ERR_LOG("Failed to get SystemAbilityManagerClient");
        return E_ERR;
    }
    OHOS::sptr<OHOS::IRemoteObject> remoteObject = saMgr->CheckSystemAbility(STORAGE_MANAGER_MANAGER_ID);
    if (remoteObject == nullptr) {
        MEDIA_ERR_LOG("Token is null.");
        return E_ERR;
    }
    cloudHelper_ = DataShare::DataShareHelper::Creator(remoteObject, CLOUD_DATASHARE_URI);
    cloudMediaAssetObserver_ = std::make_shared<CloudMediaAssetObserver>(instance_);
    // observer more than 50, failed to register
    cloudHelper_->RegisterObserverExt(Uri(CLOUD_URI), cloudMediaAssetObserver_, true);

    // observer download callback
    downloadCallback_ = std::make_shared<MediaCloudDownloadCallback>(instance_);
    int32_t ret = cloudSyncManager_.get().RegisterDownloadFileCallback(downloadCallback_);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("failed to register downloadCallback, ret: %{public}d.", ret);
        return ret;
    }
    ret = SetDeathRecipient();
    if (ret != E_OK) {
        MEDIA_ERR_LOG("failed to register death recipient, ret: %{public}d.", ret);
        return ret;
    }
    MEDIA_INFO_LOG("success to register");
    return ret;
}

int32_t CloudMediaAssetDownloadOperation::DoForceTaskExecute()
{
    if (taskStatus_ == CloudMediaAssetTaskStatus::IDLE) {
        MEDIA_ERR_LOG("DoForceTaskExecute permission denied");
        return E_ERR;
    }
    if (taskStatus_ == CloudMediaAssetTaskStatus::PAUSED) {
        MEDIA_INFO_LOG("pause cause is %{public}d", static_cast<int32_t>(pauseCause_));
        readyForDownload_ = ReadyDataForBatchDownload();
        if (IsDataEmpty(readyForDownload_)) {
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
    if (taskStatus_ != CloudMediaAssetTaskStatus::IDLE) {
        MEDIA_ERR_LOG("permission denied");
        return E_ERR;
    }
    MEDIA_INFO_LOG("enter, download type: %{public}d", cloudMediaDownloadType);
    int32_t ret = DoRelativedRegister();
    if (ret != E_OK) {
        return ret;
    }
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
        CancelDownloadTask();
    }
    return E_OK;
}

int32_t CloudMediaAssetDownloadOperation::DoRecoverExecute()
{
    if (fileNumCache_ > 0) {
        MEDIA_ERR_LOG("callback is still alive, fileNumCache: %{public}d.", fileNumCache_);
        return E_ERR;
    }
    if (IsDataEmpty(cacheForDownload_)) {
        return SubmitBatchDownload(readyForDownload_, false);
    }
    return SubmitBatchDownload(cacheForDownload_, true);
}

int32_t CloudMediaAssetDownloadOperation::ManualActiveRecoverTask(int32_t cloudMediaDownloadType)
{
    MEDIA_INFO_LOG("enter ManualActiveRecoverTask.");
    if (taskStatus_ != CloudMediaAssetTaskStatus::PAUSED) {
        MEDIA_ERR_LOG("ManualActiveRecoverTask permission denied");
        return E_ERR;
    }

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
    if (taskStatus_ != CloudMediaAssetTaskStatus::PAUSED || pauseCause_ == CloudMediaTaskPauseCause::USER_PAUSED) {
        MEDIA_ERR_LOG("PassiveStatusRecoverTask permission denied, taskStatus: %{public}d, pauseCause: %{public}d,",
            static_cast<int32_t>(taskStatus_), static_cast<int32_t>(pauseCause_));
        return E_ERR;
    }

    if (recoverCause == CloudMediaTaskRecoverCause::NETWORK_NORMAL &&
        (pauseCause_ == CloudMediaTaskPauseCause::WIFI_UNAVAILABLE ||
        pauseCause_ == CloudMediaTaskPauseCause::NETWORK_FLOW_LIMIT)) {
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

int32_t CloudMediaAssetDownloadOperation::PauseDownloadTask(const CloudMediaTaskPauseCause &pauseCause)
{
    MediaLibraryTracer tracer;
    tracer.Start("PauseDownloadTask");
    if (taskStatus_ == CloudMediaAssetTaskStatus::IDLE || pauseCause_ == CloudMediaTaskPauseCause::USER_PAUSED) {
        MEDIA_ERR_LOG("PauseDownloadTask permission denied");
        return E_ERR;
    }
    MEDIA_INFO_LOG("enter PauseDownloadTask, taskStatus_: %{public}d, pauseCause_: %{public}d, pauseCause: %{public}d",
        static_cast<int32_t>(taskStatus_), static_cast<int32_t>(pauseCause_), static_cast<int32_t>(pauseCause));

    pauseCause_ = pauseCause;
    if (taskStatus_ == CloudMediaAssetTaskStatus::DOWNLOADING) {
        taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
        if (downloadId_ != DOWNLOAD_ID_DEFAULT) {
            downloadIdCache_ = downloadId_;
            fileNumCache_ = static_cast<int32_t>(dataForDownload_.fileDownloadMap.size());
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
    downloadIdCache_ = DOWNLOAD_ID_DEFAULT;
    fileNumCache_ = 0;
    ClearData(cacheForDownload_);
    downloadId_ = DOWNLOAD_ID_DEFAULT;
    ClearData(dataForDownload_);

    isThumbnailUpdate_ = true;
    isBgDownloadPermission_ = false;

    batchDownloadTotalNum_ = 0;
    batchDownloadTotalSize_ = 0;
    hasDownloadNum_ = 0;
    hasDownloadSize_ = 0;
}

int32_t CloudMediaAssetDownloadOperation::CancelDownloadTask()
{
    if (taskStatus_ == CloudMediaAssetTaskStatus::IDLE) {
        MEDIA_ERR_LOG("CancelDownloadTask permission denied");
        return E_ERR;
    }
    int32_t ret = cloudSyncManager_.get().UnregisterDownloadFileCallback();
    SetTaskStatus(Status::IDLE);
    if (downloadId_ != DOWNLOAD_ID_DEFAULT) {
        cloudSyncManager_.get().StopFileCache(downloadId_, NEED_CLEAN);
    }
    ResetParameter();
    downloadCallback_ = nullptr;
    cloudRemoteObject_ = nullptr;
    if (cloudHelper_ == nullptr) {
        return ret;
    }
    cloudHelper_->UnregisterObserverExt(Uri(CLOUD_URI), cloudMediaAssetObserver_);
    cloudHelper_ = nullptr;
    cloudMediaAssetObserver_ = nullptr;
    return ret;
}

void CloudMediaAssetDownloadOperation::HandleSuccessCallback(const DownloadProgressObj& progress)
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    MediaLibraryTracer tracer;
    tracer.Start("HandleSuccessCallback");
    if (progress.downloadId != downloadId_ ||
        dataForDownload_.fileDownloadMap.find(progress.path) == dataForDownload_.fileDownloadMap.end()) {
        MEDIA_WARN_LOG("this path is unknown, path: %{public}s, downloadId: %{public}s, downloadId_: %{public}s.",
            MediaFileUtils::DesensitizePath(progress.path).c_str(), to_string(progress.downloadId).c_str(),
            to_string(downloadId_).c_str());
        return;
    }

    int64_t size = dataForDownload_.fileDownloadMap[progress.path];
    if (size != 0) {
        remainCount_--;
        remainSize_ -= size;
        hasDownloadNum_++;
        hasDownloadSize_ += size;
    }
    dataForDownload_.fileDownloadMap.erase(progress.path);
    downloadNum_--;

    MEDIA_INFO_LOG("success, path: %{public}s, size: %{public}s, batchSuccNum: %{public}s.",
        MediaFileUtils::DesensitizePath(progress.path).c_str(), to_string(size).c_str(),
        to_string(progress.batchSuccNum).c_str());

    if (taskStatus_ == CloudMediaAssetTaskStatus::PAUSED && progress.downloadId == downloadId_ &&
        cacheForDownload_.fileDownloadMap.find(progress.path) == cacheForDownload_.fileDownloadMap.end()) {
        fileNumCache_--;
        MEDIA_INFO_LOG("wait for callback, fileNumCache: %{public}d.", fileNumCache_);
        if (fileNumCache_ == 0) {
            downloadId_ = DOWNLOAD_ID_DEFAULT;
            SubmitBatchDownload(cacheForDownload_, true);
            return;
        }
    }

    if (downloadNum_ == 0) {
        MEDIA_INFO_LOG("success download %{public}s files.", to_string(progress.batchSuccNum).c_str());
        downloadId_ = DOWNLOAD_ID_DEFAULT;
        SubmitBatchDownload(readyForDownload_, false);
    }
}

void CloudMediaAssetDownloadOperation::MoveDownloadFileToCache(const DownloadProgressObj& progress)
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    if (progress.downloadId != downloadId_ ||
        dataForDownload_.fileDownloadMap.find(progress.path) == dataForDownload_.fileDownloadMap.end()) {
        MEDIA_WARN_LOG("This file is unknown, path: %{public}s, downloadId: %{public}s, downloadId_: %{public}s.",
            MediaFileUtils::DesensitizePath(progress.path).c_str(), to_string(progress.downloadId).c_str(),
            to_string(downloadId_).c_str());
        return;
    }
    if (cacheForDownload_.fileDownloadMap.find(progress.path) != cacheForDownload_.fileDownloadMap.end()) {
        MEDIA_INFO_LOG("file is in fileDownloadCacheMap_, path: %{public}s.",
            MediaFileUtils::DesensitizePath(progress.path).c_str());
        return;
    }
    cacheForDownload_.pathVec.push_back(progress.path);
    cacheForDownload_.fileDownloadMap[progress.path] = dataForDownload_.fileDownloadMap.at(progress.path);
    dataForDownload_.fileDownloadMap.erase(progress.path);
    fileNumCache_--;
    if (fileNumCache_ == 0) {
        downloadId_ = DOWNLOAD_ID_DEFAULT;
        SubmitBatchDownload(cacheForDownload_, true);
    }
    MEDIA_INFO_LOG("success, path: %{public}s.", MediaFileUtils::DesensitizePath(progress.path).c_str());
}

void CloudMediaAssetDownloadOperation::MoveDownloadFileToNotFound(const DownloadProgressObj& progress)
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    if (progress.downloadId != downloadId_ ||
        dataForDownload_.fileDownloadMap.find(progress.path) != dataForDownload_.fileDownloadMap.end()) {
        MEDIA_ERR_LOG("This file is known, path: %{public}s, downloadId: %{public}s, downloadId_: %{public}s.",
            MediaFileUtils::DesensitizePath(progress.path).c_str(), to_string(progress.downloadId).c_str(),
            to_string(downloadId_).c_str());
        return;
    }
    if (notFoundForDownload_.fileDownloadMap.find(progress.path) != notFoundForDownload_.fileDownloadMap.end()) {
        MEDIA_INFO_LOG("file is in notFoundForDownload_, path: %{public}s.",
            MediaFileUtils::DesensitizePath(progress.path).c_str());
        return;
    }
    notFoundForDownload_.fileDownloadMap[progress.path] = dataForDownload_.fileDownloadMap.at(progress.path);
    int64_t size = dataForDownload_.fileDownloadMap.at(progress.path);
    if (size != 0) {
        totalCount_--;
        batchDownloadTotalNum_--;
        remainCount_--;
        totalSize_ -= size;
        batchDownloadTotalSize_ -= size;
        remainSize_ -= size;
    }
    dataForDownload_.fileDownloadMap.erase(progress.path);
    downloadNum_--;
    MEDIA_INFO_LOG("success, path: %{public}s.", MediaFileUtils::DesensitizePath(progress.path).c_str());
}

void CloudMediaAssetDownloadOperation::HandleFailedCallback(const DownloadProgressObj& progress)
{
    MediaLibraryTracer tracer;
    tracer.Start("HandleFailedCallback");
    if (taskStatus_ == CloudMediaAssetTaskStatus::PAUSED && pauseCause_ == CloudMediaTaskPauseCause::USER_PAUSED) {
        MEDIA_INFO_LOG("pauseCause_ is USER_PAUSED");
        return;
    }
    MEDIA_INFO_LOG("Download error type: %{public}d, path: %{public}s.", progress.downloadErrorType,
        MediaFileUtils::DesensitizePath(progress.path).c_str());
    switch (progress.downloadErrorType) {
        case static_cast<int32_t>(DownloadProgressObj::DownloadErrorType::UNKNOWN_ERROR): {
            PauseDownloadTask(CloudMediaTaskPauseCause::CLOUD_ERROR);
            MoveDownloadFileToCache(progress);
            break;
        }
        case static_cast<int32_t>(DownloadProgressObj::DownloadErrorType::NETWORK_UNAVAILABLE): {
            PauseDownloadTask(CloudMediaTaskPauseCause::NETWORK_FLOW_LIMIT);
            MoveDownloadFileToCache(progress);
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
    MEDIA_INFO_LOG("enter DownloadStopped, path: %{public}s.", MediaFileUtils::DesensitizePath(progress.path).c_str());
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
} // namespace Media
} // namespace OHOS