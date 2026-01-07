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
#include "upgrade_restore_gallery_media_task.h"

#include <sstream>
#include <nlohmann/json.hpp>

#include "media_log.h"
#include "hisysevent.h"
#include "media_backup_report_data_type.h"
#include "json_utils.h"

namespace OHOS::Media {
std::vector<MediaRestoreResultInfo> UpgradeRestoreGalleryMediaTask::LoadTask(const std::string &taskInfo)
{
    CallbackResultData resultData = this->ParseFromJsonStr(taskInfo);
    MEDIA_INFO_LOG("GET restoreExInfo: %{public}s", resultData.ToString().c_str());
    return this->Parse(resultData);
}

MediaRestoreResultInfo UpgradeRestoreGalleryMediaTask::Load(const std::string &type, const std::string &errorCode,
    const std::string &errorInfo)
{
    MediaRestoreResultInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    info.errorCode = errorCode;
    info.errorInfo = errorInfo;
    info.type = type;
    return info;
}

MediaRestoreResultInfo UpgradeRestoreGalleryMediaTask::Load(const RestoreTaskInfo &taskInfo)
{
    MediaRestoreResultInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    info.errorCode = taskInfo.errorCode;
    info.errorInfo = taskInfo.errorInfo;
    info.successCount = taskInfo.successCount;
    info.failedCount = taskInfo.failedCount;
    info.duplicateCount = taskInfo.duplicateCount;
    info.type = taskInfo.type;
    return info;
}

CallbackResultData UpgradeRestoreGalleryMediaTask::ParseFromJsonStr(const std::string &jsonStr)
{
    JsonUtils jsonUtils;
    nlohmann::json jsonObj = jsonUtils.Parse(jsonStr);
    CallbackResultData resultData;
    for (const auto &info : jsonUtils.GetArray(jsonObj, "resultInfo")) {
        // Parse the resultInfo
        bool isResultInfo = jsonUtils.IsExists(info, "errorCode");
        isResultInfo = isResultInfo && jsonUtils.IsExists(info, "errorInfo");
        isResultInfo = isResultInfo && jsonUtils.IsExists(info, "type");
        if (isResultInfo) {
            CallbackResultInfo resultInfo;
            resultInfo.errorCode = jsonUtils.GetString(info, "errorCode");
            resultInfo.errorInfo = jsonUtils.GetString(info, "errorInfo");
            resultInfo.type = jsonUtils.GetString(info, "type");
            resultData.resultInfo = resultInfo;
            continue;
        }
        // Parse the backupInfo
        bool isBackupInfo = jsonUtils.IsExists(info, "infos");
        CHECK_AND_CONTINUE(isBackupInfo);
        for (const auto &backup : jsonUtils.GetArray(info, "infos")) {
            CallbackBackupInfo backupInfo;
            backupInfo.backupInfo = jsonUtils.GetString(backup, "backupInfo");
            backupInfo.duplicateCount = jsonUtils.GetInt(backup, "duplicateCount");
            backupInfo.failedCount = jsonUtils.GetInt(backup, "failedCount");
            backupInfo.successCount = jsonUtils.GetInt(backup, "successCount");
            resultData.infos.emplace_back(backupInfo);
        }
    }
    return resultData;
}

std::vector<MediaRestoreResultInfo> UpgradeRestoreGalleryMediaTask::Parse(const CallbackResultData &resultData)
{
    std::vector<MediaRestoreResultInfo> result;
    const CallbackResultInfo &resultInfo = resultData.resultInfo;
    for (const auto &backupInfo : resultData.infos) {
        MediaRestoreResultInfo eventInfo;
        eventInfo.sceneCode = this->sceneCode_;
        eventInfo.taskId = this->taskId_;
        eventInfo.errorCode = resultInfo.errorCode;
        eventInfo.errorInfo = resultInfo.errorInfo;
        eventInfo.type = resultInfo.type;
        eventInfo.backupInfo = backupInfo.backupInfo;
        eventInfo.duplicateCount = backupInfo.duplicateCount;
        eventInfo.failedCount = backupInfo.failedCount;
        eventInfo.successCount = backupInfo.successCount;
        result.emplace_back(eventInfo);
    }
    return result;
}
}  // namespace OHOS::Media