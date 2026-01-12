/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_READY_MANAGER_H_
#define FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_READY_MANAGER_H_

#include <memory>
#include <vector>
#include <string>

#include "safe_map.h"

#include "cloud_media_sync_const.h"
#include "cloud_sync_manager.h"
#include "media_file_utils.h"
#include "thumbnail_data.h"
#include "thumbnail_generate_worker.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

using namespace OHOS::FileManagement::CloudSync;
class ThumbnailCloudDownloadCallback : public CloudDownloadCallback {
public:
    ThumbnailCloudDownloadCallback(int32_t requestId, pid_t pid) : requestId_(requestId), pid_(pid) {};
    virtual ~ThumbnailCloudDownloadCallback() override = default;
    void OnDownloadProcess(const DownloadProgressObj &progress) override;
private:
    int32_t requestId_;
    pid_t pid_;
};

class ThumbnailReadyManager {
public:
struct AstcBatchTaskInfo {
    int32_t requestId;
    pid_t pid;
    int64_t downloadId{-1};
    std::unordered_map<std::string, ThumbnailData> downloadThumbMap;
    std::vector<ThumbnailData> localInfos;
    std::vector<std::string> cloudPaths;
    bool isCloudTaskFinish{true};
    bool isLocalTaskFinish{true};
    ThumbRdbOpt opts;
    bool isTemperatureHighForReady{false};
    std::shared_ptr<NativeRdb::RdbPredicates> rdbPredicatePtr;
    std::atomic<bool> isCancel{false};
    bool isDownloadEnd{false};
    std::atomic<int> pendingTasks{0};
    std::condition_variable cv;
    std::mutex cvMutex;
};

public:
    EXPORT static ThumbnailReadyManager& GetInstance();
    EXPORT int32_t CreateAstcBatchOnDemand(NativeRdb::RdbPredicates &rdbPredicate, int32_t requestId, pid_t pid);
    EXPORT int32_t CreateAstcBatchOnDemand(ThumbRdbOpt &opts, NativeRdb::RdbPredicates predicate, int32_t requestId,
        pid_t pid);
    EXPORT void CancelAstcBatchTask(int32_t requestId, pid_t pid);
    EXPORT int32_t GetCurrentTemperatureLevel();
    EXPORT void NotifyTempStatusForReady(const int32_t &currentTemperatureLevel);
    EXPORT std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo> GetAstcBatchTaskInfo(const pid_t pid);
    EXPORT void CreateAstcAfterDownloadThumbOnDemand(const std::string &path,
        std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo> taskInfo);
    EXPORT void RecordNotFoundThumbnail(const std::string &path,
        std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo> thumbReadyTaskData);
    EXPORT void SetDownloadEnd(pid_t pid);
    EXPORT void ExecuteCreateThumbnailTask(std::shared_ptr<ThumbnailTaskData> &data, int32_t requestId, pid_t pid);
    EXPORT void CreateAstcBatchOnDemandTaskFinish(std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo>&
        thumbReadyTaskData);
    EXPORT bool IsNeedExecuteTask(int32_t requestId, pid_t pid);
    EXPORT bool IsNeedExecuteTask(int32_t requestId, pid_t pid,
        std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo>& taskInfo);
    EXPORT void ThumbGenBatchTaskFinishNotify(int32_t requestId, pid_t pid);

private:
    ThumbnailReadyManager() = default;
    ~ThumbnailReadyManager();

    void AddQueryNoAstcRulesOnlyLocal(NativeRdb::RdbPredicates &rdbPredicate);
    bool QueryNoAstcInfosOnDemand(ThumbRdbOpt &opts, std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo> taskInfo,
        NativeRdb::RdbPredicates rdbPredicate, int &err);
    void HandleDownloadBatch(int32_t requestId, pid_t pid);
    void ProcessAstcBatchTask(ThumbRdbOpt opts, NativeRdb::RdbPredicates predicate,
        const int32_t requestId, const pid_t pid);
    void DownloadTimeOut(int32_t requestId, pid_t pid);
    void RegisterDownloadTimer(int32_t requestId, pid_t pid);
    void UnRegisterDownloadTimer();
    void InsertHighTemperatureTask(NativeRdb::RdbPredicates &rdbPredicate,
    int32_t requestId, pid_t pid);

    SafeMap<pid_t, std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo>> temperatureStatusMap_;
    int32_t currentTemperatureLevel_ = 0;
    std::mutex processMutex_;
    SafeMap<pid_t, std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo>> processRequestMap_;
    std::atomic<int32_t> timeoutCount_{0};
    std::mutex timerMutex_;
    Utils::Timer timer_{"closeDownload"};
    uint32_t timerId = 0;
    int64_t lastTimeoutTime_ = MediaFileUtils::UTCTimeMilliSeconds();
    std::mutex downloadIdMutex_;
};
} // namespace Media
} // namespace OHOS

#endif // FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_READY_MANAGER_H_