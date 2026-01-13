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

#define MLOG_TAG "BackupReport"

#include "upgrade_restore_task_report.h"

#include <string>

#include "backup_hi_audit_helper.h"
#include "backup_log_const.h"
#include "backup_log_utils.h"
#include "hisysevent.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "upgrade_restore_gallery_media_task.h"

namespace OHOS::Media {
static constexpr char MEDIA_LIBRARY[] = "MEDIALIBRARY";
UpgradeRestoreTaskReport &UpgradeRestoreTaskReport::ReportTask(const std::string &taskInfo)
{
    std::vector<MediaRestoreResultInfo> resultInfos =
        UpgradeRestoreGalleryMediaTask().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).LoadTask(taskInfo);
    for (const auto &info : resultInfos) {
        MEDIA_INFO_LOG("[STAT] GET restoreExInfo: %{public}s", info.ToString().c_str());
        PostInfoDfx(info);
        PostInfoAuditLog(info);
    }
    return *this;
}

UpgradeRestoreTaskReport &UpgradeRestoreTaskReport::Report(const std::string &type, const std::string &errorCode,
    const std::string &errorInfo)
{
    MediaRestoreResultInfo resultInfo = UpgradeRestoreGalleryMediaTask()
                                            .SetSceneCode(this->sceneCode_)
                                            .SetTaskId(this->taskId_)
                                            .Load(type, errorCode, errorInfo);
    MEDIA_INFO_LOG("[%{public}s] %{public}s: %{public}s", type.c_str(), errorCode.c_str(), errorInfo.c_str());
    PostInfoDfx(resultInfo);
    PostInfoAuditLog(resultInfo);
    return *this;
}

UpgradeRestoreTaskReport &UpgradeRestoreTaskReport::Report(const RestoreTaskInfo &taskInfo)
{
    MediaRestoreResultInfo resultInfo = UpgradeRestoreGalleryMediaTask()
                                            .SetSceneCode(this->sceneCode_)
                                            .SetTaskId(this->taskId_)
                                            .Load(taskInfo);
    MEDIA_INFO_LOG("[%{public}s] %{public}s", taskInfo.type.c_str(), resultInfo.ToString().c_str());
    PostInfoDfx(resultInfo);
    PostInfoAuditLog(resultInfo);
    return *this;
}

UpgradeRestoreTaskReport &UpgradeRestoreTaskReport::ReportInAudit(const std::string &type, const std::string &errorCode,
    const std::string &errorInfo)
{
    MediaRestoreResultInfo resultInfo = UpgradeRestoreGalleryMediaTask()
                                            .SetSceneCode(this->sceneCode_)
                                            .SetTaskId(this->taskId_)
                                            .Load(type, errorCode, errorInfo);
    MEDIA_INFO_LOG("[%{public}s] %{public}s: %{public}s", type.c_str(), errorCode.c_str(), errorInfo.c_str());
    PostInfoAuditLog(resultInfo);
    return *this;
}

UpgradeRestoreTaskReport &UpgradeRestoreTaskReport::ReportError(const ErrorInfo &info)
{
    std::string errorCode = std::to_string(info.error);
    std::string errorInfo = BackupLogUtils::ErrorInfoToString(info);
    MediaRestoreResultInfo resultInfo = UpgradeRestoreGalleryMediaTask()
                                            .SetSceneCode(this->sceneCode_)
                                            .SetTaskId(this->taskId_)
                                            .Load("ErrorInfo", errorCode, errorInfo);
    MEDIA_ERR_LOG("[Error] %{public}s: %{public}s", errorCode.c_str(), errorInfo.c_str());
    PostInfoDfx(resultInfo);
    PostErrorInfoAuditLog(info);
    return *this;
}

UpgradeRestoreTaskReport &UpgradeRestoreTaskReport::ReportProgress(const std::string &status,
    const std::string &progressInfo)
{
    MediaRestoreResultInfo resultInfo = UpgradeRestoreGalleryMediaTask()
                                            .SetSceneCode(this->sceneCode_)
                                            .SetTaskId(this->taskId_)
                                            .Load("ProgressInfo", status, progressInfo);
    MEDIA_INFO_LOG("[Progress] %{public}s: %{public}s", status.c_str(), progressInfo.c_str());
    PostInfoDfx(resultInfo);
    PostProgressInfoAuditLog(status, progressInfo);
    return *this;
}

UpgradeRestoreTaskReport &UpgradeRestoreTaskReport::ReportProgress(const std::string &status,
    const std::string &progressInfo, uint64_t ongoingTotalNumber)
{
    if (ongoingTotalNumber * ON_PROCESS_INTV % LOG_PROGRESS_INTV != 0) {
        return *this;
    }
    return ReportProgress(status, progressInfo);
}

UpgradeRestoreTaskReport &UpgradeRestoreTaskReport::ReportTimeout(uint64_t ongoingTotalNumber)
{
    if (ongoingTotalNumber * ON_PROCESS_INTV % LOG_TIMEOUT_INTV != 0) {
        return *this;
    }
    std::string status = "timeout";
    std::string progressInfo = std::to_string(ongoingTotalNumber * ON_PROCESS_INTV);
    return ReportProgress(status, progressInfo);
}

UpgradeRestoreTaskReport &UpgradeRestoreTaskReport::ReportTotal(const std::string &errorCode,
    const std::string &totalInfo)
{
    return Report("TotalInfo", errorCode, totalInfo);
}

UpgradeRestoreTaskReport &UpgradeRestoreTaskReport::ReportUpgradeEnh(const std::string &errorCode,
    const std::string &info)
{
    return Report("UpgradeEnh", errorCode, info);
}

UpgradeRestoreTaskReport &UpgradeRestoreTaskReport::ReportTimeCost(const uint64_t successCount,
    const uint64_t duplicateCount, const size_t failCount)
{
    int64_t startTime = std::atoll(this->taskId_.c_str());
    int64_t endTime = MediaFileUtils::UTCTimeSeconds();
    int64_t timeCost = endTime - startTime;
    if (timeCost < 0) {
        MEDIA_ERR_LOG("Get timeCost < 0, startTime: %{public}lld, %{public}lld", (long long)startTime,
            (long long)endTime);
        return *this;
    }
    std::string type = "TimeCost";
    std::string errorCode = std::to_string(timeCost);
    std::string errorInfo = "";
    MediaRestoreResultInfo resultInfo = UpgradeRestoreGalleryMediaTask()
                                            .SetSceneCode(this->sceneCode_)
                                            .SetTaskId(this->taskId_)
                                            .Load(type, errorCode, errorInfo);
    resultInfo.duplicateCount = static_cast<int>(duplicateCount);
    resultInfo.failedCount = static_cast<int>(failCount);
    resultInfo.successCount = static_cast<int>(successCount);
    MEDIA_INFO_LOG("[%{public}s]: %{public}s, successCount: %{public}d, duplicateCount: %{public}d, "
        "failCount: %{public}d", type.c_str(), errorCode.c_str(), resultInfo.successCount, resultInfo.duplicateCount,
        resultInfo.failedCount);
    PostInfoDfx(resultInfo);
    PostInfoAuditLog(resultInfo);
    return *this;
}

UpgradeRestoreTaskReport &UpgradeRestoreTaskReport::ReportRestoreMode(int32_t restoreMode, uint64_t notFoundFileNum)
{
    return Report("RestoreMode:NotFoundFileNum", std::to_string(restoreMode), std::to_string(notFoundFileNum));
}

int32_t UpgradeRestoreTaskReport::PostInfoDfx(const MediaRestoreResultInfo &info)
{
    int32_t ret = HiSysEventWrite(MEDIA_LIBRARY,
        "MEDIALIB_BACKUP_RESTORE_RESULT",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "SCENE_CODE",
        info.sceneCode,
        "TASK_ID",
        info.taskId,
        "ERROR_CODE",
        info.errorCode,
        "ERROR_INFO",
        info.errorInfo,
        "TYPE",
        info.type,
        "BACKUP_INFO",
        info.backupInfo,
        "DUPLICATE_COUNT",
        info.duplicateCount,
        "FAILED_COUNT",
        info.failedCount,
        "SUCCESS_COUNT",
        info.successCount);
    if (ret != 0) {
        MEDIA_ERR_LOG("PostInfoDfx error:%{public}d", ret);
    }
    return ret;
}

int32_t UpgradeRestoreTaskReport::PostInfoAuditLog(const MediaRestoreResultInfo &info)
{
    BackupHiAuditHelper().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).WriteReportAuditLog(info.ToString());
    return 0;
}

int32_t UpgradeRestoreTaskReport::PostErrorInfoAuditLog(const ErrorInfo &info)
{
    BackupHiAuditHelper().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).WriteErrorAuditLog(info);
    return 0;
}

int32_t UpgradeRestoreTaskReport::PostProgressInfoAuditLog(const std::string &status, const std::string &progressInfo)
{
    BackupHiAuditHelper()
        .SetSceneCode(this->sceneCode_)
        .SetTaskId(this->taskId_)
        .WriteProgressAuditLog(status, progressInfo);
    return 0;
}
}  // namespace OHOS::Media