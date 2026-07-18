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
#define MLOG_TAG "Media_Reverse_Restore"

#include "photo_count_context.h"
#include "photo_count_strategy.h"
#include "reverse_clone_restore.h"
#include "media_log.h"
#include "media_file_utils.h"
#include "upgrade_restore_task_report.h"
#include "backup_const.h"

namespace OHOS {
namespace Media {

static constexpr int32_t OLD_DATABASE_PHOTOS_THRESHOLD = 1000;
static constexpr int32_t NEW_DATABASE_PHOTOS_THRESHOLD = 10000;
static constexpr int32_t DATABASE_PHOTOS_DIFFERENCE = 1000;

PhotoCountContext::PhotoCountContext(std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    bool isCloudRestoreSatisfied, bool shouldAbsorbCloudFromSourceRdb,
    int32_t sceneCode, const std::string& taskId)
    : mediaRdb_(mediaRdb), mediaLibraryRdb_(mediaLibraryRdb),
      isCloudRestoreSatisfied_(isCloudRestoreSatisfied),
      shouldAbsorbCloudFromSourceRdb_(shouldAbsorbCloudFromSourceRdb),
      sceneCode_(sceneCode), taskId_(taskId)
{
    if (shouldAbsorbCloudFromSourceRdb_) {
        countStrategy_ = std::make_unique<CloudAbsorbCountStrategy>();
    } else {
        countStrategy_ = std::make_unique<StandardCountStrategy>();
    }
}

bool PhotoCountContext::NeedReverseRestore(ReverseRestoreReportInfo& info)
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    if (mediaRdb_ == nullptr || mediaLibraryRdb_ == nullptr) {
        MEDIA_ERR_LOG("PhotoCountContext: rdb null, old is null: %{public}d, new is null: %{public}d",
            mediaRdb_ == nullptr, mediaLibraryRdb_ == nullptr);
        return false;
    }

    int32_t oldCount = countStrategy_->GetOldCount(mediaRdb_, isCloudRestoreSatisfied_);
    int32_t newCount = countStrategy_->GetNewCount(mediaLibraryRdb_, isCloudRestoreSatisfied_);
    MEDIA_INFO_LOG("PhotoCountContext: strategy=%{public}s, oldCount=%{public}d, newCount=%{public}d",
        countStrategy_->GetStrategyName().c_str(), oldCount, newCount);
    info.restoreCountInfo = std::to_string(oldCount) + " | " + std::to_string(newCount);
    info.failedCount = oldCount;

    if (oldCount == -NativeRdb::E_SQLITE_CORRUPT) {
        ErrorInfo errorInfo(RestoreError::CLONE_RESTORE_DATABASE_CORRUPTION, 1, "ERR_SQLITE_CORRUPT",
            "ERR_SQLITE_CORRUPT");
        UpgradeRestoreTaskReport(sceneCode_, taskId_).ReportErrorInAudit(errorInfo);
        MEDIA_ERR_LOG("Database corruption detected in old database");
        return false;
    }

    if (oldCount <= OLD_DATABASE_PHOTOS_THRESHOLD || newCount > NEW_DATABASE_PHOTOS_THRESHOLD) {
        MEDIA_INFO_LOG("threshold not met, old count: %{public}d, new count: %{public}d", oldCount, newCount);
        return false;
    }

    if (oldCount > OLD_DATABASE_PHOTOS_THRESHOLD && oldCount - newCount >= DATABASE_PHOTOS_DIFFERENCE) {
        MEDIA_INFO_LOG("threshold and difference are met, old count: %{public}d, new count: %{public}d",
            oldCount, newCount);
        return true;
    }
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    info.beforeTransformTimeCost.append(" Determine Direction: ")
        .append(std::to_string(endTime - startTime) + ";");
    MEDIA_INFO_LOG("difference not met, old count: %{public}d, new count: %{public}d", oldCount, newCount);
    return false;
}

} // namespace Media
} // namespace OHOS