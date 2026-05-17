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

#define MLOG_TAG "QualityConflictResolver"

#include "quality_conflict_resolver.h"

#include "media_log.h"
#include "scan_task_deduplicator.h"

namespace OHOS {
namespace Media {
constexpr int32_t QUALITY_COUNT = 2;
constexpr ScanSubmitResult DECISION_TABLE[QUALITY_COUNT][QUALITY_COUNT] = {
    // [newQuality-1][executingQuality-1]: LOW, FULL
    {ScanSubmitResult::WAITING, ScanSubmitResult::REJECTED},  // new=LOW: vs LOW=等待, vs FULL=拒绝
    {ScanSubmitResult::WAITING, ScanSubmitResult::WAITING}    // new=FULL: vs LOW=优先, vs FULL=等待
};

QualityConflictResolver::QualityConflictResolver()
{
    MEDIA_INFO_LOG("QualityConflictResolver created for Camera Segmented Shot");
}

QualityConflictResolver::~QualityConflictResolver()
{
    MEDIA_INFO_LOG("QualityConflictResolver destroyed");
}

bool QualityConflictResolver::IsStrategyEnabled(const std::shared_ptr<ScanTaskContext>& newTask) const
{
    if (newTask == nullptr) {
        return false;
    }

    return newTask->config.GetQuality() != ScanQuality::DEFAULT;
}

bool QualityConflictResolver::IsValidQualityIndex(const std::shared_ptr<ScanTaskContext>& task, int32_t& rowIndex) const
{
    if (task == nullptr) {
        return false;
    }

    rowIndex = static_cast<int32_t>(task->config.GetQuality());
    return rowIndex >= 0 && rowIndex < QUALITY_COUNT;
}

ScanSubmitResult QualityConflictResolver::Resolve(const std::shared_ptr<ScanTaskContext>& newTask,
    const std::shared_ptr<ScanTaskContext>& executingTask)
{
    int32_t newRow = -1;
    if (!IsValidQualityIndex(newTask, newRow)) {
        MEDIA_ERR_LOG("QualityResolver: invalid task (new %{public}d)", newRow);
        return ScanSubmitResult::WAITING;
    }

    int32_t execRow = 1;
    if (!IsValidQualityIndex(executingTask, execRow)) {
        MEDIA_ERR_LOG("QualityResolver: invalid task (executing %{public}d)", execRow);
        return ScanSubmitResult::WAITING;
    }

    ScanSubmitResult result = DECISION_TABLE[newRow][execRow];

    MEDIA_INFO_LOG("result %{public}d (new %{public}d vs executing %{public}d, fileId %{public}d)",
        static_cast<int32_t>(result), newRow + 1, execRow + 1, newTask->config.GetFileId());

    return result;
}

} // namespace Media
} // namespace OHOS