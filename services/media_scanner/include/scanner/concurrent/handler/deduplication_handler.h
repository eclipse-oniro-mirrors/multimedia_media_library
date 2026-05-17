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

#ifndef DEDUPLICATION_HANDLER_H
#define DEDUPLICATION_HANDLER_H

#include <condition_variable>
#include <mutex>
#include <unordered_map>

#include "i_conflict_resolver.h"
#include "resolver_registry.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class EXPORT DeduplicationHandler {
public:
    DeduplicationHandler();
    ~DeduplicationHandler();

    ScanSubmitResult Handle(const std::shared_ptr<ScanTaskContext>& context);
    void UnmarkAsScanning(const std::shared_ptr<ScanTaskContext>& context);
    void WaitForSyncScanCompletion(int32_t fileId);
    void NotifySyncTaskCompleted(int32_t fileId);
    std::shared_ptr<ScanTaskContext> GetSyncPendingTask(int32_t fileId);
    std::shared_ptr<ScanTaskContext> GetNextWaitingTask(const std::shared_ptr<ScanTaskContext>& context);
    void ClearWaitingTasks(const std::shared_ptr<ScanTaskContext>& context);

    bool IsFileIdExecuting(int32_t fileId) const;
    bool HasWaitingTask(int32_t fileId) const;
    bool HasSyncPending(int32_t fileId) const;
    bool HasSyncWaiting(int32_t fileId) const;
    size_t GetExecutingCount() const;
    size_t GetWaitingCount() const;
    size_t GetSyncPendingCount() const;

private:
    bool IsValidFileId(int32_t fileId) const;

    ScanSubmitResult HandleConflict(const std::shared_ptr<ScanTaskContext>& newTask, int32_t fileId);

    void HandleWaitingTask(const std::shared_ptr<ScanTaskContext>& task, int32_t fileId);
    void HandleSyncWaitingTask(const std::shared_ptr<ScanTaskContext>& task, int32_t fileId);
    void HandleAsyncWaitingTask(const std::shared_ptr<ScanTaskContext>& task, int32_t fileId);
    void MergeWaitingTaskConfig(
        const std::shared_ptr<ScanTaskContext>& task, int32_t fileId, ScanExecutionMode executionMode);

    std::shared_ptr<ResolverRegistry> resolverRegistry_;

    std::unordered_map<int32_t, std::shared_ptr<ScanTaskContext>> executingTasks_;
    std::unordered_map<int32_t, std::shared_ptr<ScanTaskContext>> waitingTask_;
    std::unordered_map<int32_t, std::shared_ptr<std::condition_variable>> syncWaitingCv_;
    std::unordered_map<int32_t, std::shared_ptr<ScanTaskContext>> syncPendingTask_;
    mutable std::mutex mutex_;
    bool destroying_ = false;
};

} // namespace Media
} // namespace OHOS

#endif // DEDUPLICATION_HANDLER_H