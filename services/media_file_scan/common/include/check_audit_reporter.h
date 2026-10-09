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
#ifndef OHOS_MEDIA_CHECK_AUDIT_REPORTER_H
#define OHOS_MEDIA_CHECK_AUDIT_REPORTER_H

#include <string>

#include "consistency_check_data_types.h"

namespace OHOS::Media::CheckAuditReporter {
void ReportPhotos(const std::vector<ConsistencyCheck::PhotoRecord> &records, const std::string &type,
    const std::string &scenario);
void ReportPhoto(const ConsistencyCheck::PhotoRecord &record, const std::string &type, const std::string &scenario,
    uint32_t count = 1);
void ReportAlbums(const std::vector<ConsistencyCheck::AlbumRecord> &records, const std::string &type,
    const std::string &scenario);
void ReportAlbum(const ConsistencyCheck::AlbumRecord &record, const std::string &type, const std::string &scenario,
    uint32_t count = 1);
} // namespace OHOS::Media::CheckAuditReporter

#endif // OHOS_MEDIA_CHECK_AUDIT_REPORTER_H