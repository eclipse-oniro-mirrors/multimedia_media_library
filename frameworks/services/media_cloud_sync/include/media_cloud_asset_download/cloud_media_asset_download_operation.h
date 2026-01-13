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

#ifndef OHOS_CLOUD_MEDIA_ASSET_DOWNLOAD_OPERATION_H
#define OHOS_CLOUD_MEDIA_ASSET_DOWNLOAD_OPERATION_H

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <memory>
#include <chrono>

#include "cloud_media_asset_callback.h"
#include "cloud_media_asset_types.h"
#include "cloud_media_asset_observer.h"
#include "cloud_sync_common.h"
#include "cloud_sync_manager.h"
#include "datashare_helper.h"
#include "medialibrary_command.h"
#include "medialibrary_rdbstore.h"
#include "net_connect_observer.h"
#include "safe_map.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
using namespace FileManagement::CloudSync;

class CloudDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    CloudDeathRecipient(std::shared_ptr<CloudMediaAssetDownloadOperation> operation) : operation_(operation) {}
    ~CloudDeathRecipient() {}
    void OnRemoteDied(const wptr<IRemoteObject> &object);

private:
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation_ = nullptr;
};

class CloudMediaAssetDownloadOperation {
public:
    struct DownloadFileData {
        std::vector<std::string> pathVec;
        SafeMap<std::string, int64_t> fileDownloadMap;
        std::vector<std::string> batchFileIdNeedDownload;
        int64_t batchSizeNeedDownload = 0;
        int64_t batchCountNeedDownload = 0;
    };

    enum class Status : int32_t {
        FORCE_DOWNLOADING,
        GENTLE_DOWNLOADING,
        PAUSE_FOR_TEMPERATURE_LIMIT,
        PAUSE_FOR_ROM_LIMIT,
        PAUSE_FOR_NETWORK_FLOW_LIMIT,
        PAUSE_FOR_WIFI_UNAVAILABLE,
        PAUSE_FOR_POWER_LIMIT,
        PAUSE_FOR_BACKGROUND_TASK_UNAVAILABLE,
        PAUSE_FOR_FREQUENT_USER_REQUESTS,
        PAUSE_FOR_CLOUD_ERROR,
        PAUSE_FOR_USER_PAUSE,
        RECOVER_FOR_MANAUL_ACTIVE,
        RECOVER_FOR_PASSIVE_STATUS,
        IDLE,
    };

    CloudMediaAssetDownloadOperation() {}
    ~CloudMediaAssetDownloadOperation() {}
    CloudMediaAssetDownloadOperation(const CloudMediaAssetDownloadOperation &) = delete;
    CloudMediaAssetDownloadOperation& operator=(const CloudMediaAssetDownloadOperation &) = delete;

    EXPORT static std::shared_ptr<CloudMediaAssetDownloadOperation> GetInstance();
    EXPORT int32_t StartDownloadTask(int32_t cloudMediaDownloadType);
    EXPORT int32_t PauseDownloadTask(const CloudMediaTaskPauseCause &pauseCause);
    EXPORT int32_t CancelDownloadTask();
    EXPORT int32_t ManualActiveRecoverTask(int32_t cloudMediaDownloadType);
    EXPORT int32_t PassiveStatusRecoverTask(const CloudMediaTaskRecoverCause &recoverCause);
    EXPORT void CheckStorageAndRecoverDownloadTask();

    EXPORT void HandleSuccessCallback(const DownloadProgressObj &progress);
    EXPORT void HandleFailedCallback(const DownloadProgressObj &progress);
    EXPORT void HandleStoppedCallback(const DownloadProgressObj &progress);

    EXPORT CloudMediaDownloadType GetDownloadType();
    EXPORT CloudMediaAssetTaskStatus GetTaskStatus();
    EXPORT CloudMediaTaskPauseCause GetTaskPauseCause();
    EXPORT std::string GetTaskInfo();
    EXPORT int32_t InitDownloadTaskInfo();
    void ResetDownloadTryTime();
    void HandleOnRemoteDied();

private:
    void ClearData(DownloadFileData &data);
    bool IsDataEmpty(DownloadFileData &data);
    EXPORT int32_t DoRelativedRegister();
    int32_t SetDeathRecipient();
    bool IsProperFgTemperature();
    EXPORT void InitStartDownloadTaskStatus(const bool &isForeground);
    void ResetParameter();

    EXPORT void SetTaskStatus(Status status);
    EXPORT std::shared_ptr<NativeRdb::ResultSet> QueryDownloadFilesNeeded(const bool &isQueryInfo);
    EXPORT DownloadFileData ReadyDataForBatchDownload();
    EXPORT int32_t DoForceTaskExecute();
    EXPORT int32_t SubmitBatchDownload(DownloadFileData &data, const bool &isCache);
    EXPORT void StartFileCacheFailed();
    EXPORT void StartBatchDownload();
    EXPORT int32_t DoRecoverExecute();
    EXPORT int32_t PassiveStatusRecover();
    EXPORT int32_t SubmitBatchDownloadAgain();
    void MoveDownloadFileToCache(const DownloadProgressObj &progress, const bool tryDownload = false);
    void MoveDownloadFileToNotFound(const DownloadProgressObj &progress);
    int32_t RegisterNetObserver();

public:
    static std::shared_ptr<CloudMediaAssetDownloadOperation> instance_;

    // Confirmation of the notification
    bool isThumbnailUpdate_ = true;
    bool isBgDownloadPermission_ = false;

private:
    std::reference_wrapper<CloudSyncManager> cloudSyncManager_ = CloudSyncManager::GetInstance();
    CloudMediaAssetTaskStatus taskStatus_ = CloudMediaAssetTaskStatus::IDLE;
    CloudMediaDownloadType downloadType_ = CloudMediaDownloadType::DOWNLOAD_GENTLE;
    CloudMediaTaskPauseCause pauseCause_ = CloudMediaTaskPauseCause::NO_PAUSE;
    std::shared_ptr<DataShare::DataShareHelper> cloudHelper_;
    std::shared_ptr<CloudMediaAssetObserver> cloudMediaAssetObserver_;
    std::shared_ptr<MediaCloudDownloadCallback> downloadCallback_;
    OHOS::sptr<OHOS::IRemoteObject> cloudRemoteObject_;
    OHOS::sptr<NetConnectObserver> netObserver_;
    static std::mutex mutex_;
    static std::mutex callbackMutex_;

    DownloadFileData readyForDownload_;
    DownloadFileData notFoundForDownload_;

    // data cache
    DownloadFileData cacheForDownload_;

    // data downloading
    bool isCache_ = false;
    int64_t downloadId_ = -1;
    DownloadFileData dataForDownload_;

    // common info
    int64_t totalCount_ = 0;
    int64_t totalSize_ = 0;
    int64_t remainCount_ = 0;
    int64_t remainSize_ = 0;

    uint32_t downloadTryTime_ = 0;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_CLOUD_MEDIA_ASSET_DOWNLOAD_OPERATION_H