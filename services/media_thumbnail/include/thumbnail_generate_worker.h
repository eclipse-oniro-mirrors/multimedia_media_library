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

#ifndef FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_GENERATE_WORKER_H
#define FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_GENERATE_WORKER_H

#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <safe_queue.h>
#include <thread>
#include <timer.h>

#include "cpu_utils.h"
#include "thumbnail_utils.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

enum class ThumbnailTaskType {
    FOREGROUND,
    BACKGROUND,
    ASYNC_UPDATE_RDB,
};

enum class ThumbnailTaskPriority {
    HIGH,
    MID,
    LOW,
};

class ThumbnailTaskData {
public:
    ThumbnailTaskData() = default;

    ThumbnailTaskData(int32_t requestId, pid_t pid = 0) : requestId_(requestId), pid_(pid) {}

    ThumbnailTaskData(ThumbRdbOpt &opts, ThumbnailData &data) : opts_(opts), thumbnailData_(data) {}

    ThumbnailTaskData(ThumbRdbOpt &opts, ThumbnailData &data,
        int32_t requestId, pid_t pid = 0) : opts_(opts), thumbnailData_(data), requestId_(requestId), pid_(pid) {}

    ~ThumbnailTaskData() = default;

    ThumbRdbOpt opts_;
    ThumbnailData thumbnailData_;
    int32_t requestId_ = 0;
    pid_t pid_ = 0;
};

using ThumbnailGenerateExecute = std::function<void(std::shared_ptr<ThumbnailTaskData> &)>;

class ExecuteParamBuilder {
public:
    ExecuteParamBuilder() = default;
    ~ExecuteParamBuilder() = default;
    int32_t tempLimit_ { -1 };
    int32_t batteryLimit_ { -1 };
    CpuAffinityType affinity_ { CpuAffinityType::CPU_IDX_DEFAULT };
};

class ThumbnailGeneratorWrapper {
public:
    ThumbnailGeneratorWrapper() = default;
    ~ThumbnailGeneratorWrapper() = default;
    ThumbnailGeneratorWrapper(ThumbnailGenerateExecute execute,
        std::shared_ptr<ExecuteParamBuilder> param) : executor_(execute), executeParam_(param) {};
    void operator()(std::shared_ptr<ThumbnailTaskData> &data);
private:
    bool IsPreconditionFullfilled();
    void BeforeExecute();
    ThumbnailGenerateExecute executor_ { nullptr };
    std::shared_ptr<ExecuteParamBuilder> executeParam_ { nullptr };
};

class ThumbnailGenerateTask {
public:
    ThumbnailGenerateTask(ThumbnailGenerateExecute executor,
        std::shared_ptr<ThumbnailTaskData> &data,
        std::shared_ptr<ExecuteParamBuilder> param = nullptr) : executor_(executor, param), data_(data) {}
    ~ThumbnailGenerateTask() = default;
    ThumbnailGeneratorWrapper executor_;
    std::shared_ptr<ThumbnailTaskData> data_;
};

class ThumbnailGenerateThreadStatus {
public:
    ThumbnailGenerateThreadStatus(int threadId) : threadId_(threadId) {}
    ~ThumbnailGenerateThreadStatus() = default;
    int threadId_;
    bool isThreadWaiting_ = false;
    int32_t taskNum_ = 0;
    CpuAffinityType cpuAffinityType = CpuAffinityType::CPU_IDX_DEFAULT;
    CpuAffinityType cpuAffinityTypeLowPriority = CpuAffinityType::CPU_IDX_DEFAULT;
};

class ThumbnailGenerateWorker {
public:
    EXPORT ThumbnailGenerateWorker() = default;
    EXPORT ~ThumbnailGenerateWorker();
    EXPORT int32_t Init(const ThumbnailTaskType &taskType);
    EXPORT int32_t ReleaseTaskQueue(const ThumbnailTaskPriority &taskPriority);
    EXPORT int32_t AddTask(
        const std::shared_ptr<ThumbnailGenerateTask> &task, const ThumbnailTaskPriority &taskPriority);
    EXPORT void TryCloseTimer();
    EXPORT bool IsLowerQueueEmpty();

private:
    void StartWorker(std::shared_ptr<ThumbnailGenerateThreadStatus> threadStatus);
    bool WaitForTask(std::shared_ptr<ThumbnailGenerateThreadStatus> threadStatus);
    void ClearWorkerThreads();
    void TryClearWorkerThreads();
    void RegisterWorkerTimer();
    bool IsAllThreadWaiting();

    ThumbnailTaskType taskType_;

    std::atomic<bool> isThreadRunning_ = false;
    std::list<std::thread> threads_;
    std::list<std::shared_ptr<ThumbnailGenerateThreadStatus>> threadsStatus_;

    std::mutex workerLock_;
    std::condition_variable workerCv_;

    SafeQueue<std::shared_ptr<ThumbnailGenerateTask>> highPriorityTaskQueue_;
    SafeQueue<std::shared_ptr<ThumbnailGenerateTask>> midPriorityTaskQueue_;
    SafeQueue<std::shared_ptr<ThumbnailGenerateTask>> lowPriorityTaskQueue_;
    Utils::Timer timer_{"closeThumbnailWorker"};
    uint32_t timerId_ = 0;
    std::mutex timerMutex_;
    std::mutex taskMutex_;
};
} // namespace Media
} // namespace OHOS

#endif //FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_GENERATE_WORKER_H
