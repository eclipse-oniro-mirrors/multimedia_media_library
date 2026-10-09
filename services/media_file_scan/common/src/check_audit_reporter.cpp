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

#include "check_audit_reporter.h"

#include "hi_audit.h"
#include "file_scan_utils.h"
#include "media_log.h"

namespace OHOS::Media::CheckAuditReporter {
void ReportPhotos(const std::vector<ConsistencyCheck::PhotoRecord> &records, const std::string &type,
    const std::string &scenario)
{
    CHECK_AND_RETURN(records.size() > 0);
    uint32_t count = static_cast<uint32_t>(records.size());
    for (const auto &record : records) {
        ReportPhoto(record, type, scenario, count);
    }
}

void ReportPhoto(const ConsistencyCheck::PhotoRecord &record, const std::string &type, const std::string &scenario,
    uint32_t count)
{
    AuditLog auditLog = {
        .isUserBehavior = false,
        .cause = "CHECK_PHOTO",
        .operationType = type,
        .operationScenario = scenario,
        .operationCount = count,
        .id = std::to_string(record.fileId),
        .type = record.fileSourceType,
        .path = FileScanUtils::GarbleFilePath(record.storagePath)
    };
    HiAudit::GetInstance().Write(auditLog);
}

void ReportAlbums(const std::vector<ConsistencyCheck::AlbumRecord> &records, const std::string &type,
    const std::string &scenario)
{
    CHECK_AND_RETURN(records.size() > 0);
    uint32_t count = static_cast<uint32_t>(records.size());
    for (const auto &record : records) {
        ReportAlbum(record, type, scenario, count);
    }
}

void ReportAlbum(const ConsistencyCheck::AlbumRecord &record, const std::string &type, const std::string &scenario,
    uint32_t count)
{
    AuditLog auditLog = {
        .isUserBehavior = false,
        .cause = "CHECK_ALBUM",
        .operationType = type,
        .operationScenario = scenario,
        .operationCount = count,
        .id = std::to_string(record.albumId),
        .type = record.albumSubtype,
        .path = FileScanUtils::GarbleFilePath(record.lpath)
    };
    HiAudit::GetInstance().Write(auditLog);
}
} // namespace OHOS::Media::CheckAuditReporter