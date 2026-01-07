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
#ifndef OHOS_MEDIA_BACKUP_UPGRADE_RESTORE_TASK_REPORT_H
#define OHOS_MEDIA_BACKUP_UPGRADE_RESTORE_TASK_REPORT_H

#include <string>
#include <vector>

#include "media_backup_report_data_type.h"

namespace OHOS::Media {
class UpgradeRestoreTaskReport {
public:
    UpgradeRestoreTaskReport() {};
    UpgradeRestoreTaskReport(int32_t sceneCode, const std::string &taskId)
        : sceneCode_(sceneCode), taskId_(taskId)
    {
    }
    UpgradeRestoreTaskReport &SetSceneCode(int32_t sceneCode)
    {
        this->sceneCode_ = sceneCode;
        return *this;
    }
    UpgradeRestoreTaskReport &SetTaskId(const std::string &taskId)
    {
        this->taskId_ = taskId;
        return *this;
    }
    UpgradeRestoreTaskReport &ReportTask(const std::string &taskInfo);
    UpgradeRestoreTaskReport &Report(const std::string &type, const std::string &errorCode,
        const std::string &errorInfo);
    UpgradeRestoreTaskReport &Report(const RestoreTaskInfo &taskInfo);
    UpgradeRestoreTaskReport &ReportInAudit(const std::string &type, const std::string &errorCode,
        const std::string &errorInfo);
    UpgradeRestoreTaskReport &ReportError(const ErrorInfo &info);
    UpgradeRestoreTaskReport &ReportProgress(const std::string &status, const std::string &progressInfo);
    UpgradeRestoreTaskReport &ReportProgress(const std::string &status, const std::string &progressInfo,
        uint64_t ongoingTotalNumber);
    UpgradeRestoreTaskReport &ReportTimeout(uint64_t ongoingTotalNumber);
    UpgradeRestoreTaskReport &ReportTotal(const std::string &errorCode, const std::string &totalInfo);
    UpgradeRestoreTaskReport &ReportTimeCost(const uint64_t successCount, const uint64_t duplicateCount,
        const size_t failCount);
    UpgradeRestoreTaskReport &ReportUpgradeEnh(const std::string &errorCode, const std::string &info);
    UpgradeRestoreTaskReport &ReportRestoreMode(int32_t restoreMode, uint64_t notFoundFileNum);

private:
    int32_t PostInfoDfx(const MediaRestoreResultInfo &info);
    int32_t PostInfoAuditLog(const MediaRestoreResultInfo &info);
    int32_t PostErrorInfoAuditLog(const ErrorInfo &info);
    int32_t PostProgressInfoAuditLog(const std::string &status, const std::string &progressInfo);

private:
    int32_t sceneCode_{-1};
    std::string taskId_{};
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_BACKUP_UPGRADE_RESTORE_TASK_REPORT_H