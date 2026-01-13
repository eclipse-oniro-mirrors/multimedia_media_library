/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_MEDIALIBRARY_BACKGROUND_CLOUD_BATCH_SELECTED_FILE_PROCESSOR_H
#define OHOS_MEDIALIBRARY_BACKGROUND_CLOUD_BATCH_SELECTED_FILE_PROCESSOR_H

#include "background_cloud_file_processor_common.h"
#include "background_cloud_batch_selected_file_download_callback.h"
#include "medialibrary_related_system_state_manager.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

constexpr int32_t DOWNLOAD_SELECTED_INTERVAL = 2 * 1000;  // 2 seconds

class BackgroundCloudBatchSelectedFileProcessor {
public:
    // 回调
    EXPORT static void HandleBatchSelectedSuccessCallback(const DownloadProgressObj &progress);
    EXPORT static void HandleBatchSelectedFailedCallback(const DownloadProgressObj &progress);
    EXPORT static void HandleBatchSelectedStoppedCallback(const DownloadProgressObj &progress);
    EXPORT static void HandleBatchSelectedRunningCallback(const DownloadProgressObj &progress);
    // process
    EXPORT static bool IsBatchDownloadProcessRunningStatus();
    EXPORT static void SetBatchDownloadProcessRunningStatus(bool running);
    EXPORT static void SetBatchDownloadAddedFlag(bool status);
    EXPORT static bool GetBatchDownloadAddedFlag();

    EXPORT static void TriggerStopBatchDownloadProcessor(bool needClean = false);
    EXPORT static void TriggerPauseBatchDownloadProcessor(std::vector<std::string> &fileIdsDownloading);
    EXPORT static void TriggerCancelBatchDownloadProcessor(std::vector<std::string> &fileIds, bool sendNotify = false);
    EXPORT static void LaunchBatchDownloadProcessor();
    EXPORT static void LaunchAutoResumeBatchDownloadProcessor();

    EXPORT static bool HaveBatchDownloadResourcesTask();
    EXPORT static bool HaveBatchDownloadForAutoResumeTask();
    EXPORT static void StartBatchDownloadResourcesTimer();
    EXPORT static void StopBatchDownloadResourcesTimer(bool needClean = false);
    EXPORT static bool IsStartTimerRunning();

    EXPORT static bool CanAutoStopCondition(BatchDownloadAutoPauseReasonType &autoPauseReason);
    EXPORT static bool CanAutoRestoreCondition();
    EXPORT static bool StopProcessConditionCheck();

    EXPORT static void AutoStopAction(BatchDownloadAutoPauseReasonType &autoPauseReason);
    EXPORT static void AutoResumeAction();
    EXPORT static void NotifyRefreshProgressInfo();
    EXPORT static void TriggerAutoResumeBatchDownloadResourceCheck();
    EXPORT static void TriggerAutoStopBatchDownloadResourceCheck();

    EXPORT static void UpdateDBStatusInfoForSingleDownloadCompletely(int32_t fileId);

    EXPORT static bool IsNetValidated();

    enum BatchDownloadStatus : int32_t {
        INIT = 0,
        SUCCESS,
        FAILED,
        SKIP_UPDATE_DB,
        NETWORK_UNAVAILABLE,
        STORAGE_FULL,
        STOPPED,
        UNKNOWN,
    };

    typedef struct {
        std::string fileId;
        int32_t percent;
        BatchDownloadStatus status;
    } InDownloadingFileInfo;

private:
    typedef struct {
        std::string uri;
        MediaType mediaType;
    } SingleDownloadFiles;

    class BatchDownloadCloudFilesData : public AsyncTaskData {
    public:
        BatchDownloadCloudFilesData(SingleDownloadFiles downloadFile) : downloadFile_(downloadFile){};
        ~BatchDownloadCloudFilesData() override = default;

        SingleDownloadFiles downloadFile_;
    };

    EXPORT static bool GetCurrentRoundInDownloadingFileIdList(std::string &fileIdsStr);
    EXPORT static bool GetCurrentRoundExcludeList(std::string &fileIdsStr);
    EXPORT static bool IsFileIdInCurrentRoundWithoutLock(const std::string &fileId);
    EXPORT static void ClassifyCurrentRoundFileIdInList(std::vector<std::string> &fileIdList,
        std::vector<int64_t> &needStopDownloadIds);
    EXPORT static int64_t GetDownloadIdByFileIdInCurrentRound(const std::string &fileId);
    EXPORT static void ClearRoundMapInfos();
    EXPORT static int32_t GetDownloadQueueSizeWithLock();

    EXPORT static void ExitDownloadSelectedBatchResources();
    EXPORT static void DownloadSelectedBatchResources();
    EXPORT static void ParseBatchSelectedToDoFiles(std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        vector<std::string> &pendingURIs, vector<int32_t> &localFileIds, vector<int32_t> &exceptionFileIds);
    EXPORT static void DownloadLatestBatchSelectedFinished();
    EXPORT static bool GetStorageFreeRatio(double &freeRatio);
    EXPORT static int32_t GetDownloadFileIdCnt(std::string fileId);
    EXPORT static void CheckAndUpdateDownloadFileIdCnt(std::string fileId, int32_t cnt);
    EXPORT static void RemoveFinishedResult();
    EXPORT static int32_t AddSelectedBatchDownloadTask(std::string &downloadFilesUri);
    EXPORT static void DownloadSelectedBatchFilesExecutor(AsyncTaskData *data);
    EXPORT static void StopDownloadFiles(int64_t downloadId, bool needClean = false);
    EXPORT static int32_t AddTasksAndStarted(vector<std::string> &pendingURIs);
    EXPORT static void StopAllDownloadingTask(bool needClean = false);
    EXPORT static void RefreshNotRestoreReason(vector<int32_t> &currentNotRestoreReasons);
    // DB
    EXPORT static std::shared_ptr<NativeRdb::ResultSet> QueryBatchSelectedResourceFiles();
    EXPORT static void UpdateDBProgressStatusInfoForBatch(vector<int32_t> fileIds, int32_t status);
    EXPORT static int32_t UpdateDBProgressInfoForFileId(std::string &fileIdStr, int32_t percent,
        int64_t finishTime, int32_t status);
    EXPORT static int32_t QueryBatchSelectedResourceFilesNum();
    EXPORT static int32_t QueryBatchSelectedFilesNumForAutoResume();
    EXPORT static int32_t QueryBatchDownloadFinishStatusCountFromDB(int32_t &totalValue,
        int32_t &completedValue, int32_t &failedValue);
    EXPORT static int32_t ClassifyFileIdsInDownloadResourcesTable(const std::vector<std::string> &fileIds,
        std::vector<std::string> &existedIds);
    EXPORT static int32_t DeleteCancelStateDownloadResources(const std::vector<std::string> &fileIds);
    EXPORT static int32_t QueryPercentOnTaskStart(std::string &fileId, int32_t &percent);
    EXPORT static int32_t QueryAutoPauseReason(int32_t &autoStopReason);
    EXPORT static int32_t UpdateAllAutoPauseReason(int32_t reason);

    // Auto pause resume
    EXPORT static int32_t UpdateAllAutoPauseDownloadResourcesInfo(BatchDownloadAutoPauseReasonType &autoPauseReason);
    EXPORT static int32_t UpdateAllAutoResumeDownloadResourcesInfo();
    EXPORT static int32_t UpdateAllStatusAutoPauseToDownloading();
    EXPORT static int32_t UpdateAllStatusAutoPauseToWaiting();

    EXPORT static bool IsCellularNetConnected();
    EXPORT static bool IsWifiConnected();
    EXPORT static int32_t GetDeviceTemperature();
    EXPORT static void ControlDownloadLimit();

    static int32_t downloadInterval_;
    static int32_t downloadDuration_;
    static std::recursive_mutex mutex_;
    static std::mutex downloadResultMutex_;
    static std::mutex mutexRunningStatus_;
    static std::mutex autoActionMutex_;
    static int32_t downloadSelectedInterval_;
    static Utils::Timer batchDownloadResourceTimer_;
    static uint32_t batchDownloadResourcesStartTimerId_;
    static const uint32_t batchQueryLimitNum = 10;
    static int32_t batchDownloadQueueLimitNum_;
    static const int32_t batchDownloadQueueLimitNumHigh = 5;
    static const int32_t batchDownloadQueueLimitNumLow = 1;
    // fileId-StatusMap 记录本轮 一轮清理一次 失败的可以跨一轮开始
    EXPORT static std::unordered_map<std::string, BatchDownloadStatus> downloadResult_;
    // fileId-Count 记录失败+本轮 即时清理
    EXPORT static std::unordered_map<std::string, int32_t> downloadFileIdAndCount_;
    // DownloadId-Info 记录在运行任务 即时清理
    EXPORT static std::unordered_map<int64_t, InDownloadingFileInfo> currentDownloadIdFileInfoMap_;
    
    static std::atomic<bool> batchDownloadTaskAdded_;
    static std::atomic<bool> downloadLatestFinished_;
    static std::atomic<bool> batchDownloadProcessRunningStatus_;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_BACKGROUND_CLOUD_BATCH_SELECTED_FILE_PROCESSOR_H