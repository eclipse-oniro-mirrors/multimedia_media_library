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

#include "backup_hi_audit_helper.h"

#include "backup_hi_audit.h"
#include "backup_log_utils.h"

namespace OHOS::Media {
void BackupHiAuditHelper::WriteErrorAuditLog(const ErrorInfo &info)
{
    BackupAuditLog auditLog;
    SetErrorAuditLog(auditLog, info);
    BackupHiAudit::GetInstance().Write(auditLog);
}

void BackupHiAuditHelper::WriteProgressAuditLog(const std::string &status, const std::string &extend)
{
    BackupAuditLog auditLog;
    SetProgressAuditLog(auditLog, status, extend);
    BackupHiAudit::GetInstance().Write(auditLog);
}

void BackupHiAuditHelper::WriteReportAuditLog(const std::string &extend)
{
    BackupAuditLog auditLog;
    SetReportAuditLog(auditLog, extend);
    BackupHiAudit::GetInstance().Write(auditLog);
}

void BackupHiAuditHelper::SetBasicAuditLog(BackupAuditLog &auditLog, const std::string &extend)
{
    auditLog.operationScenario = std::to_string(sceneCode_);
    auditLog.taskId = taskId_;
    auditLog.extend = BackupLogUtils::Format(extend);
}

void BackupHiAuditHelper::SetErrorAuditLog(BackupAuditLog &auditLog, const ErrorInfo &info)
{
    auditLog.operationType = "RESTORE";
    auditLog.cause = BackupLogUtils::RestoreErrorToString(info.error);
    auditLog.operationCount = static_cast<uint32_t>(info.count);
    auditLog.operationStatus = info.status.empty() ? "failed" : info.status;
    SetBasicAuditLog(auditLog, info.extend);
}

void BackupHiAuditHelper::SetProgressAuditLog(BackupAuditLog &auditLog, const std::string &status,
    const std::string &extend)
{
    auditLog.operationType = "STAT";
    auditLog.cause = "PROGRESS";
    auditLog.operationStatus = status.empty() ? "success" : status;
    SetBasicAuditLog(auditLog, extend);
}

void BackupHiAuditHelper::SetReportAuditLog(BackupAuditLog &auditLog, const std::string &extend)
{
    auditLog.operationType = "STAT";
    auditLog.cause = "DFX";
    auditLog.operationStatus = "success";
    SetBasicAuditLog(auditLog, extend);
}
} // namespace OHOS::Media