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

#define MLOG_TAG "EnhancedScanExecutor"

#include "enhanced_scan_executor.h"

#include "media_log.h"
#include "medialibrary_errno.h"
#include "scan_strategy_manager.h"

namespace OHOS {
namespace Media {

EnhancedScanExecutor::EnhancedScanExecutor()
{
    deduplicator_ = std::make_shared<ScanTaskDeduplicator>();
}

EnhancedScanExecutor::~EnhancedScanExecutor()
{
    Stop();
    MEDIA_INFO_LOG("EnhancedScanExecutor destroyed");
}

ScanSubmitResult EnhancedScanExecutor::Submit(const std::shared_ptr<ScanTaskContext>& task)
{
    if (task == nullptr || deduplicator_ == nullptr) {
        MEDIA_ERR_LOG("Submit: invalid state");
        return ScanSubmitResult::REJECTED;
    }

    if (!running_) {
        running_ = true;
        stopFlag_ = false;
    }

    ScanSubmitResult result = deduplicator_->SubmitTask(task);
    if (result == ScanSubmitResult::EXECUTING && task->config.GetExecutionMode() == ScanExecutionMode::ASYNC) {
        std::lock_guard<std::mutex> lock(queueMutex_);
        globalTaskQueue_.push(task);
    }

    MEDIA_INFO_LOG("result %{public}d, globalTask count: %{public}zu, config %{public}s",
        static_cast<int32_t>(result), globalTaskQueue_.size(), task->config.ToString().c_str());

    return result;
}

void EnhancedScanExecutor::StartSync(const std::shared_ptr<ScanTaskContext>& task)
{
    if (task == nullptr) {
        MEDIA_ERR_LOG("StartSync: task is nullptr");
        return;
    }

    ExecuteTask(task);
    HandleTaskCompletion(task);

    MEDIA_INFO_LOG("completed (fileId %{public}d)", task->config.GetFileId());
}

void EnhancedScanExecutor::StartAsync()
{
    if (!running_) {
        running_ = true;
        stopFlag_ = false;
    }

    std::lock_guard<std::mutex> lock(queueMutex_);

    if (activeThreadCount_.load() < MAX_THREAD_COUNT && !globalTaskQueue_.empty()) {
        auto self = shared_from_this();
        std::thread([self] {
            self->WorkerThread();
        }).detach();
        activeThreadCount_++;
    }

    MEDIA_INFO_LOG("EnhancedScanExecutor started (active %{public}zu)", activeThreadCount_.load());
}

void EnhancedScanExecutor::Stop()
{
    if (!running_) {
        MEDIA_WARN_LOG("Stop: executor not running");
        return;
    }

    stopFlag_ = true;
    running_ = false;
    ClearAllTasks();

    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        threadCv_.wait(lock, [this] { return activeThreadCount_.load() == 0; });
    }

    MEDIA_INFO_LOG("EnhancedScanExecutor stopped");
}

void EnhancedScanExecutor::ClearAllTasks()
{
    {
        std::lock_guard<std::mutex> lock(queueMutex_);

        while (!globalTaskQueue_.empty()) {
            auto task = globalTaskQueue_.front();
            globalTaskQueue_.pop();

            if (task != nullptr && task->config.GetCallback() != nullptr) {
                task->config.GetCallback()->OnScanFinished(
                    E_STOP, "", task->config.GetFilePath()
                );
            }
        }
    }

    if (deduplicator_) {
        deduplicator_->ClearAllWaitingQueues();
    }

    MEDIA_INFO_LOG("cleared all tasks");
}

void EnhancedScanExecutor::WaitForSyncScanCompletion(int32_t fileId)
{
    if (deduplicator_ != nullptr) {
        deduplicator_->WaitForSyncScanCompletion(fileId);
    }
}

void EnhancedScanExecutor::ExecuteTask(const std::shared_ptr<ScanTaskContext>& task)
{
    if (task == nullptr) {
        MEDIA_ERR_LOG("ExecuteTask: task is nullptr");
        return;
    }

    MEDIA_INFO_LOG("start (fileId %{public}d, path %{private}s)",
        task->config.GetFileId(), task->config.GetFilePath().c_str());

    auto strategy = ScanStrategyManager::GetInstance().SelectStrategy(task->config.GetStrategyType());
    if (strategy == nullptr) {
        MEDIA_ERR_LOG("ExecuteTask: strategy not found: %{public}d",
            static_cast<int>(task->config.GetStrategyType()));

        if (task->config.GetCallback()) {
            task->config.GetCallback()->OnScanFinished(E_ERR, "", task->config.GetFilePath());
        }
        return;
    }

    strategy->Scan(task);

    MEDIA_INFO_LOG("completed (fileId %{public}d)", task->config.GetFileId());
}

void EnhancedScanExecutor::HandleTaskCompletion(const std::shared_ptr<ScanTaskContext>& task)
{
    if (task == nullptr || deduplicator_ == nullptr) {
        MEDIA_ERR_LOG("HandleTaskCompletion: invalid state");
        return;
    }

    int32_t fileId = task->config.GetFileId();

    deduplicator_->UnmarkAsScanning(task);

    if (ExecuteSyncPendingTask(fileId)) {
        return;
    }

    ScheduleNextAsyncTask(task);
    MEDIA_INFO_LOG("globalTask count: %{public}zu, activeThreadCount: %{public}zu.",
        globalTaskQueue_.size(), activeThreadCount_.load());
}

bool EnhancedScanExecutor::ExecuteSyncPendingTask(int32_t fileId)
{
    auto syncPendingTask = deduplicator_->GetSyncPendingTask(fileId);
    if (syncPendingTask == nullptr) {
        return false;
    }

    auto self = shared_from_this();
    std::thread([self, syncPendingTask, fileId] {
        self->ExecuteTask(syncPendingTask);
        self->deduplicator_->NotifySyncTaskCompleted(fileId);
    }).detach();

    MEDIA_INFO_LOG("sync task started (fileId %{public}d)", fileId);
    return true;
}

void EnhancedScanExecutor::ScheduleNextAsyncTask(const std::shared_ptr<ScanTaskContext>& task)
{
    auto nextTask = deduplicator_->GetNextWaitingTask(task);
    if (nextTask == nullptr) {
        MEDIA_INFO_LOG("no waiting task for fileId %{public}d", task->config.GetFileId());
        return;
    }

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        globalTaskQueue_.push(nextTask);
    }

    if (task->config.GetExecutionMode() == ScanExecutionMode::SYNC) {
        StartAsync();
    }
}

void EnhancedScanExecutor::NotifyThreadExited()
{
    std::lock_guard<std::mutex> lock(queueMutex_);
    threadCv_.notify_all();
}

void EnhancedScanExecutor::WorkerThread()
{
    MEDIA_DEBUG_LOG("WorkerThread: started");

    while (!stopFlag_) {
        std::shared_ptr<ScanTaskContext> task;
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            if (globalTaskQueue_.empty() || stopFlag_) {
                break;
            }
            task = GetNextTask();
        }

        ExecuteTask(task);
        HandleTaskCompletion(task);
    }

    activeThreadCount_--;
    NotifyThreadExited();
    MEDIA_INFO_LOG("WorkerThread exiting, activeThread: %{public}zu, globalTask: %{public}zu.",
        activeThreadCount_.load(), globalTaskQueue_.size());
}

std::shared_ptr<ScanTaskContext> EnhancedScanExecutor::GetNextTask()
{
    if (globalTaskQueue_.empty()) {
        return nullptr;
    }

    auto task = globalTaskQueue_.front();
    globalTaskQueue_.pop();

    return task;
}

} // namespace Media
} // namespace OHOS