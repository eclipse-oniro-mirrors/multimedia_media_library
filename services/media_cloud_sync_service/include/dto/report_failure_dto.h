/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_CLOUD_SYNC_REPORT_FAILURE_DTO_H
#define OHOS_MEDIA_CLOUD_SYNC_REPORT_FAILURE_DTO_H

#include <string>
#include <vector>
#include <sstream>
#include "cloud_media_define.h"

namespace OHOS::Media::CloudSync {
class EXPORT ReportFailureDto {
public:
    int32_t apiCode;
    int32_t errorCode;
    int32_t fileId;
    std::string cloudId;

public:  // basic functions
    std::string ToString() const;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_REPORT_FAILURE_DTO_H