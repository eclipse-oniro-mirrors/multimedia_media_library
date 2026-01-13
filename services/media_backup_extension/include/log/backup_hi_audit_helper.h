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

#ifndef BACKUP_HI_AUDIT_HELPER_H
#define BACKUP_HI_AUDIT_HELPER_H

#include <string>

#include "backup_hi_audit.h"
#include "media_backup_report_data_type.h"

namespace OHOS::Media {
class BackupHiAuditHelper {
public:
    BackupHiAuditHelper &SetSceneCode(int32_t sceneCode)
    {
        sceneCode_ = sceneCode;
        return *this;
    }
    BackupHiAuditHelper &SetTaskId(const std::string &taskId)
    {
        taskId_ = taskId;
        return *this;
    }
    void WriteErrorAuditLog(const ErrorInfo &info);
    void WriteProgressAuditLog(const std::string &status, const std::string &extend = "");
    void WriteReportAuditLog(const std::string &extend = "");

private:
    void SetBasicAuditLog(BackupAuditLog &auditLog, const std::string &extend);
    void SetErrorAuditLog(BackupAuditLog &auditLog, const ErrorInfo &info);
    void SetProgressAuditLog(BackupAuditLog &auditLog, const std::string &status, const std::string &extend);
    void SetReportAuditLog(BackupAuditLog &auditLog, const std::string &extend);

private:
    int32_t sceneCode_{-1};
    std::string taskId_;
};
} // namespace OHOS::Media

#endif // BACKUP_HI_AUDIT_HELPER_H