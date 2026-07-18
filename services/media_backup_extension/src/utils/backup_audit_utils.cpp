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

#define MLOG_TAG "BackupAuditUtils"

#include "backup_audit_utils.h"

#include "media_log.h"
#include "backup_const.h"
#include "upgrade_restore_task_report.h"
#include "media_file_utils.h"

namespace OHOS::Media {

void BackupAuditUtils::RecordCloudIdEmptyAudit(const FileInfo &fileInfo, const std::string &scenario,
    int32_t sceneCode, const std::string &taskId)
{
    CHECK_AND_RETURN(fileInfo.cloudUniqueId.empty());

    MEDIA_ERR_LOG("cloud_id is Empty, fileId: %{public}s, displayName: %{public}s",
        std::to_string(fileInfo.fileIdOld).c_str(),
        MediaFileUtils::DesensitizeName(fileInfo.displayName).c_str());
    
    std::string extendInfo = scenario + ", fileId: " + std::to_string(fileInfo.fileIdOld) + 
        ", displayName: " + MediaFileUtils::DesensitizeName(fileInfo.displayName) + 
        ", filePath: " + MediaFileUtils::DesensitizePath(fileInfo.filePath);
    ErrorInfo errorInfo(RestoreError::PHOTOS_CLOUD_ID_EMPTY, 1, "", extendInfo);
    UpgradeRestoreTaskReport(sceneCode, taskId).ReportErrorInAudit(errorInfo);
}

} // namespace OHOS::Media