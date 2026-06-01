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

#define MLOG_TAG "DeduplicationHandler"

#include "deduplication_handler.h"

#include "media_log.h"
#include "quality_conflict_resolver.h"

namespace OHOS {
namespace Media {

constexpr int32_t SYNC_WAIT_TIMEOUT_SECONDS = 5;

DeduplicationHandler::DeduplicationHandler()
{
    resolverRegistry_ = std::make_shared<ResolverRegistry>();
    
    auto qualityResolver = std::make_shared<QualityConflictResolver>();
    resolverRegistry_->RegisterResolver(ConflictPolicy::QUALITY_PRIORITY, qualityResolver);
    
    MEDIA_INFO_LOG("DeduplicationHandler created");
}

DeduplicationHandler::~DeduplicationHandler()
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    destroying_ = true;
    
    waitingTask_.clear();
    executingTasks_.clear();
    syncPendingTask_.clear();
    
    for (auto& pair : syncWaitingCv_) {
        pair.second->notify_all();
    }
    
    MEDIA_INFO_LOG("DeduplicationHandler destroyed");
}

ScanSubmitResult DeduplicationHandler::Handle(const std::shared_ptr<ScanTaskContext>& context)
{
    if (context == nullptr) {
        MEDIA_ERR_LOG("context is nullptr");
        return ScanSubmitResult::REJECTED;
    }

    int32_t fileId = context->config.GetFileId();
    if (!IsValidFileId(fileId)) {
        MEDIA_DEBUG_LOG("fileId %{public}d invalid, skip", fileId);
        return ScanSubmitResult::EXECUTING;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (executingTasks_.find(fileId) == executingTasks_.end()) {
        executingTasks_[fileId] = context;
        MEDIA_DEBUG_LOG("fileId %{public}d not executing, marked", fileId);
        return ScanSubmitResult::EXECUTING;
    }

    return HandleConflict(context, fileId);
}

ScanSubmitResult DeduplicationHandler::HandleConflict(const std::shared_ptr<ScanTaskContext>& newTask, int32_t fileId)
{
    ConflictPolicy policy = newTask->config.GetConflictPolicy();

    auto resolver = resolverRegistry_->GetResolver(policy);
    if (resolver == nullptr || !resolver->IsStrategyEnabled(newTask)) {
        HandleWaitingTask(newTask, fileId);
        MEDIA_WARN_LOG("strategy not enabled, waiting (fileId %{public}d)", fileId);
        return ScanSubmitResult::WAITING;
    }

    auto executingTask = executingTasks_.at(fileId);
    ScanSubmitResult result = resolver->Resolve(newTask, executingTask);
    if (result == ScanSubmitResult::WAITING) {
        HandleWaitingTask(newTask, fileId);
    }

    return result;
}

void DeduplicationHandler::HandleWaitingTask(const std::shared_ptr<ScanTaskContext>& task, int32_t fileId)
{
    if (task->config.GetExecutionMode() == ScanExecutionMode::SYNC) {
        HandleSyncWaitingTask(task, fileId);
    } else {
        HandleAsyncWaitingTask(task, fileId);
    }
}

void DeduplicationHandler::HandleSyncWaitingTask(const std::shared_ptr<ScanTaskContext>& task, int32_t fileId)
{
    if (syncWaitingCv_.find(fileId) == syncWaitingCv_.end()) {
        syncWaitingCv_[fileId] = std::make_shared<std::condition_variable>();
    }

    if (waitingTask_.find(fileId) != waitingTask_.end()) {
        MergeWaitingTaskConfig(task, fileId, ScanExecutionMode::SYNC);
        MEDIA_INFO_LOG("sync merged with waiting (fileId %{public}d)", fileId);
    }

    waitingTask_[fileId] = task;
    MEDIA_INFO_LOG("sync task added (fileId %{public}d)", fileId);
}

void DeduplicationHandler::HandleAsyncWaitingTask(const std::shared_ptr<ScanTaskContext>& task, int32_t fileId)
{
    if (syncWaitingCv_.find(fileId) != syncWaitingCv_.end()) {
        MEDIA_INFO_LOG("async task merged to sync (fileId %{public}d)", fileId);
        return;
    }

    if (waitingTask_.find(fileId) != waitingTask_.end()) {
        MergeWaitingTaskConfig(task, fileId, ScanExecutionMode::ASYNC);
        MEDIA_INFO_LOG("async merged (fileId %{public}d)", fileId);
    }

    waitingTask_[fileId] = task;
    MEDIA_INFO_LOG("async task added (fileId %{public}d)", fileId);
}

void DeduplicationHandler::MergeWaitingTaskConfig(const std::shared_ptr<ScanTaskContext>& task, int32_t fileId,
    ScanExecutionMode executionMode)
{
    auto mergedConfig = waitingTask_[fileId]->config.Merge(task->config, executionMode);
    task->config = mergedConfig;
}

void DeduplicationHandler::UnmarkAsScanning(const std::shared_ptr<ScanTaskContext>& context)
{
    if (context == nullptr || !IsValidFileId(context->config.GetFileId())) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    int32_t fileId = context->config.GetFileId();
    if (executingTasks_.find(fileId) == executingTasks_.end()) {
        return;
    }

    executingTasks_.erase(fileId);

    if (waitingTask_.find(fileId) != waitingTask_.end() && syncWaitingCv_.find(fileId) != syncWaitingCv_.end()) {
        syncPendingTask_[fileId] = waitingTask_[fileId];
        executingTasks_[fileId] = waitingTask_[fileId];
        waitingTask_.erase(fileId);
    }

    MEDIA_INFO_LOG("fileId %{public}d unmarked (executing %{public}zu)", fileId, executingTasks_.size());
}

void DeduplicationHandler::NotifySyncTaskCompleted(int32_t fileId)
{
    std::lock_guard<std::mutex> lock(mutex_);

    syncPendingTask_.erase(fileId);
    waitingTask_.erase(fileId);
    executingTasks_.erase(fileId);

    if (syncWaitingCv_.find(fileId) != syncWaitingCv_.end()) {
        syncWaitingCv_[fileId]->notify_all();
        syncWaitingCv_.erase(fileId);
    }

    MEDIA_INFO_LOG("sync task completed (fileId %{public}d)", fileId);
}

void DeduplicationHandler::WaitForSyncScanCompletion(int32_t fileId)
{
    if (!IsValidFileId(fileId)) {
        return;
    }

    std::unique_lock<std::mutex> lock(mutex_);

    if (executingTasks_.find(fileId) == executingTasks_.end() &&
        syncPendingTask_.find(fileId) == syncPendingTask_.end()) {
        MEDIA_INFO_LOG("fileId %{public}d not executing", fileId);
        return;
    }

    if (syncWaitingCv_.find(fileId) == syncWaitingCv_.end()) {
        syncWaitingCv_[fileId] = std::make_shared<std::condition_variable>();
    }

    bool completed = syncWaitingCv_[fileId]->wait_for(lock, std::chrono::seconds(SYNC_WAIT_TIMEOUT_SECONDS),
        [this, fileId] {
        return destroying_ ||
               (executingTasks_.find(fileId) == executingTasks_.end() &&
                syncPendingTask_.find(fileId) == syncPendingTask_.end());
    });

    if (destroying_) {
        return;
    }

    if (!completed) {
        MEDIA_WARN_LOG("fileId %{public}d wait timeout (2s)", fileId);
    }

    syncWaitingCv_.erase(fileId);

    MEDIA_INFO_LOG("fileId %{public}d completed (timeout %{public}d)", fileId, !completed);
}

std::shared_ptr<ScanTaskContext> DeduplicationHandler::GetNextWaitingTask(
    const std::shared_ptr<ScanTaskContext>& context)
{
    if (context == nullptr || !IsValidFileId(context->config.GetFileId())) {
        return nullptr;
    }

    int32_t fileId = context->config.GetFileId();

    std::lock_guard<std::mutex> lock(mutex_);

    if (waitingTask_.find(fileId) == waitingTask_.end()) {
        return nullptr;
    }

    auto nextTask = waitingTask_[fileId];
    waitingTask_.erase(fileId);
    executingTasks_[fileId] = nextTask;

    MEDIA_INFO_LOG("popped and marked, config %{public}s", nextTask->config.ToString().c_str());

    return nextTask;
}

void DeduplicationHandler::ClearWaitingTasks(const std::shared_ptr<ScanTaskContext>& context)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (context == nullptr) {
        waitingTask_.clear();
        syncPendingTask_.clear();
        executingTasks_.clear();
        for (auto& pair : syncWaitingCv_) {
            pair.second->notify_all();
        }
        syncWaitingCv_.clear();
        MEDIA_INFO_LOG("cleared all waiting tasks and sync state");
        return;
    }

    int32_t fileId = context->config.GetFileId();
    if (!IsValidFileId(fileId)) {
        return;
    }

    if (waitingTask_.find(fileId) != waitingTask_.end()) {
        waitingTask_.erase(fileId);
        MEDIA_INFO_LOG("cleared waiting task (fileId %{public}d)", fileId);
    }
}

bool DeduplicationHandler::IsValidFileId(int32_t fileId) const
{
    return fileId > 0;
}

std::shared_ptr<ScanTaskContext> DeduplicationHandler::GetSyncPendingTask(int32_t fileId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (syncPendingTask_.find(fileId) != syncPendingTask_.end()) {
        return syncPendingTask_[fileId];
    }
    
    return nullptr;
}

bool DeduplicationHandler::IsFileIdExecuting(int32_t fileId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return executingTasks_.find(fileId) != executingTasks_.end();
}

bool DeduplicationHandler::HasWaitingTask(int32_t fileId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return waitingTask_.find(fileId) != waitingTask_.end();
}

bool DeduplicationHandler::HasSyncPending(int32_t fileId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return syncPendingTask_.find(fileId) != syncPendingTask_.end();
}

bool DeduplicationHandler::HasSyncWaiting(int32_t fileId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return syncWaitingCv_.find(fileId) != syncWaitingCv_.end();
}

size_t DeduplicationHandler::GetExecutingCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return executingTasks_.size();
}

size_t DeduplicationHandler::GetWaitingCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return waitingTask_.size();
}

size_t DeduplicationHandler::GetSyncPendingCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return syncPendingTask_.size();
}

} // namespace Media
} // namespace OHOS