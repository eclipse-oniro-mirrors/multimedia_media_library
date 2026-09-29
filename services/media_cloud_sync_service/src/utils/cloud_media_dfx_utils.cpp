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

#define MLOG_TAG "Media_Cloud_Dfx"

#include "cloud_media_dfx_utils.h"

#include "media_log.h"
#include "hi_audit.h"
#include "dfx_const.h"
#include "photos_dto.h"
#include "cloud_media_pull_data_dto.h"
#include "media_file_utils.h"

namespace OHOS::Media::CloudSync {
// Audit the cloudId is empty, unexpected, and should not happen. Audit log for investigation.
void CloudMediaDfxUtils::RecordCloudIdEmptyAudit(const PhotosDto &record, const std::string &scenario)
{
    CHECK_AND_RETURN(record.cloudId.empty());

    MEDIA_ERR_LOG("cloud_id is Empty, fileId: %{public}s, displayName: %{public}s",
        std::to_string(record.fileId).c_str(),
        MediaFileUtils::DesensitizeName(record.displayName).c_str());
    AuditLog auditLog;
    auditLog.isUserBehavior = false;
    auditLog.cause = "DFX";
    auditLog.operationType = "PHOTOS_CLOUD_ID_EMPTY";
    auditLog.operationScenario = scenario;
    auditLog.operationCount = 1;
    auditLog.operationStatus = "empty";
    auditLog.extend = record.ToString();
    auditLog.id = std::to_string(record.fileId);
    auditLog.displayName = MediaFileUtils::DesensitizeName(record.displayName);
    auditLog.type = static_cast<int32_t>(DfxType::CLOUD_SYNC_PHOTOS_CLOUD_ID_EMPTY);
    auditLog.path = record.path;
    HiAudit::GetInstance().Write(auditLog);
}

void CloudMediaDfxUtils::RecordCloudIdEmptyAudit(const CloudMediaPullDataDto &pullData, const std::string &scenario)
{
    CHECK_AND_RETURN(pullData.cloudId.empty());

    MEDIA_ERR_LOG("cloud_id is Empty, fileId: %{public}s, displayName: %{public}s",
        std::to_string(pullData.localFileId).c_str(),
        MediaFileUtils::DesensitizeName(pullData.basicDisplayName).c_str());
    AuditLog auditLog;
    auditLog.isUserBehavior = false;
    auditLog.cause = "DFX";
    auditLog.operationType = "PHOTOS_CLOUD_ID_EMPTY";
    auditLog.operationScenario = scenario;
    auditLog.operationCount = 1;
    auditLog.operationStatus = "empty";
    auditLog.extend = pullData.ToString();
    auditLog.id = std::to_string(pullData.localFileId);
    auditLog.displayName = MediaFileUtils::DesensitizeName(pullData.basicDisplayName);
    auditLog.type = static_cast<int32_t>(DfxType::CLOUD_SYNC_PHOTOS_CLOUD_ID_EMPTY);
    auditLog.path = pullData.localPath;
    HiAudit::GetInstance().Write(auditLog);
}

}  // namespace OHOS::Media::CloudSync