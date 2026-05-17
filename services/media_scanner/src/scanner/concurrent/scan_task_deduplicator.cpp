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

#define MLOG_TAG "ScanTaskDeduplicator"

#include "scan_task_deduplicator.h"

#include "media_log.h"

namespace OHOS {
namespace Media {

ScanTaskDeduplicator::ScanTaskDeduplicator()
{
    handler_ = std::make_shared<DeduplicationHandler>();
    MEDIA_INFO_LOG("ScanTaskDeduplicator created");
}

ScanTaskDeduplicator::~ScanTaskDeduplicator()
{
    ClearAllWaitingQueues();
    MEDIA_INFO_LOG("ScanTaskDeduplicator destroyed");
}

ScanSubmitResult ScanTaskDeduplicator::SubmitTask(const std::shared_ptr<ScanTaskContext>& task)
{
    if (task == nullptr) {
        MEDIA_ERR_LOG("task is nullptr");
        return ScanSubmitResult::REJECTED;
    }

    if (task->config.GetFileId() < 0) {
        MEDIA_ERR_LOG("invalid fileId %{public}d", task->config.GetFileId());
        return ScanSubmitResult::REJECTED;
    }

    ScanSubmitResult result = handler_->Handle(task);
    MEDIA_INFO_LOG("SubmitTask result %{public}d, executing %{public}zu, waiting %{public}zu, syncPending %{public}zu",
        static_cast<int32_t>(result), handler_->GetExecutingCount(), handler_->GetWaitingCount(),
        handler_->GetSyncPendingCount());
    return result;
}

void ScanTaskDeduplicator::UnmarkAsScanning(const std::shared_ptr<ScanTaskContext>& task)
{
    if (handler_ != nullptr) {
        handler_->UnmarkAsScanning(task);
    }
}

std::shared_ptr<ScanTaskContext> ScanTaskDeduplicator::GetNextWaitingTask(
    const std::shared_ptr<ScanTaskContext>& task)
{
    if (handler_ != nullptr) {
        return handler_->GetNextWaitingTask(task);
    }
    return nullptr;
}

void ScanTaskDeduplicator::ClearAllWaitingQueues()
{
    if (handler_ != nullptr) {
        handler_->ClearWaitingTasks(nullptr);
        MEDIA_INFO_LOG("cleared all waiting tasks");
    }
}

void ScanTaskDeduplicator::WaitForSyncScanCompletion(int32_t fileId)
{
    if (handler_ != nullptr) {
        handler_->WaitForSyncScanCompletion(fileId);
    }
}

void ScanTaskDeduplicator::NotifySyncTaskCompleted(int32_t fileId)
{
    if (handler_ != nullptr) {
        handler_->NotifySyncTaskCompleted(fileId);
    }
}

std::shared_ptr<ScanTaskContext> ScanTaskDeduplicator::GetSyncPendingTask(int32_t fileId)
{
    if (handler_ != nullptr) {
        return handler_->GetSyncPendingTask(fileId);
    }
    return nullptr;
}

} // namespace Media
} // namespace OHOS