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

#ifndef ENHANCED_SCAN_EXECUTOR_H
#define ENHANCED_SCAN_EXECUTOR_H

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "scan_task_deduplicator.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class EXPORT EnhancedScanExecutor : public std::enable_shared_from_this<EnhancedScanExecutor> {
public:
    EnhancedScanExecutor();
    ~EnhancedScanExecutor();

    ScanSubmitResult Submit(const std::shared_ptr<ScanTaskContext>& task);
    void StartSync(const std::shared_ptr<ScanTaskContext>& task);
    void StartAsync();
    void Stop();
    void ClearAllTasks();
    void WaitForSyncScanCompletion(int32_t fileId);

private:
    void ExecuteTask(const std::shared_ptr<ScanTaskContext>& task);
    void HandleTaskCompletion(const std::shared_ptr<ScanTaskContext>& task);
    void WorkerThread();
    std::shared_ptr<ScanTaskContext> GetNextTask();
    bool ExecuteSyncPendingTask(int32_t fileId);
    void ScheduleNextAsyncTask(const std::shared_ptr<ScanTaskContext>& task);
    void NotifyThreadExited();

    static const size_t MAX_THREAD_COUNT = 4;

    std::atomic<size_t> activeThreadCount_{0};
    std::queue<std::shared_ptr<ScanTaskContext>> globalTaskQueue_;
    std::shared_ptr<ScanTaskDeduplicator> deduplicator_;

    std::mutex queueMutex_;
    std::condition_variable threadCv_;

    std::atomic<bool> running_{false};
    std::atomic<bool> stopFlag_{false};
};

} // namespace Media
} // namespace OHOS
#endif // ENHANCED_SCAN_EXECUTOR_H