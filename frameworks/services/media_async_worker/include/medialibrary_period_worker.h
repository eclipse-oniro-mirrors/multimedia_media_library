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

#ifndef FRAMEWORKS_SERVICE_MEDIA_PERIOD_WORKER_INCLUDE_MEDIALIBRARY_ASYNC_WORKER_H_
#define FRAMEWORKS_SERVICE_MEDIA_PERIOD_WORKER_INCLUDE_MEDIALIBRARY_ASYNC_WORKER_H_

#include <atomic>
#include <condition_variable>
#include <vector>
#include <map>
#include <mutex>
#include <thread>

namespace OHOS {
namespace Media {

#define EXPORT __attribute__ ((visibility ("default")))
#define COMPILE_HIDDEN __attribute__ ((visibility ("hidden")))

class PeriodTaskData {
public:
    PeriodTaskData() {};
    virtual ~PeriodTaskData() {};
};

enum PeriodTaskType {
    COMMON_NOTIFY,
    CLOUD_ANALYSIS_ALBUM
};

using PeriodExecute = void (*)(PeriodTaskData *data);
class MedialibraryPeriodTask {
public:
    MedialibraryPeriodTask(PeriodTaskType periodTaskType, int32_t period, std::string threadName)
        : periodTaskType_(periodTaskType), period_(period), threadName_(threadName) {}
    virtual ~MedialibraryPeriodTask() {}
    PeriodExecute executor_{nullptr};
    PeriodTaskData *data_{nullptr};
    PeriodTaskType periodTaskType_;
    std::atomic<bool> isThreadRunning_{false};
    std::atomic<bool> isStop_{true};
    int32_t period_{0};
    std::string threadName_;
};


class MediaLibraryPeriodWorker {
public:
    virtual ~MediaLibraryPeriodWorker();
    EXPORT static std::shared_ptr<MediaLibraryPeriodWorker> GetInstance();
    EXPORT void StopThread(PeriodTaskType periodTaskType);
    EXPORT bool IsThreadRunning(PeriodTaskType periodTaskType);
    EXPORT bool StartTask(PeriodTaskType periodTaskType, PeriodExecute executor, PeriodTaskData *data);

private:
    COMPILE_HIDDEN MediaLibraryPeriodWorker();
    COMPILE_HIDDEN void Init();
    COMPILE_HIDDEN static void HandleTask(PeriodTaskType periodTaskType);

    COMPILE_HIDDEN static std::atomic<bool> stop_;
    COMPILE_HIDDEN static std::shared_ptr<MediaLibraryPeriodWorker> periodWorkerInstance_;
    COMPILE_HIDDEN static std::mutex instanceMtx_;
    COMPILE_HIDDEN static std::map<int32_t, std::shared_ptr<MedialibraryPeriodTask>> tasks_;
    COMPILE_HIDDEN static std::mutex mtx_;
};

} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_NOTIFY_H