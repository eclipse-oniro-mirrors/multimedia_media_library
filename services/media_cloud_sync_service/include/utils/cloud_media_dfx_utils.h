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

#ifndef OHOS_CLOUD_MEDIA_DFX_UTILS_H
#define OHOS_CLOUD_MEDIA_DFX_UTILS_H

#include <string>

#include "photos_dto.h"
#include "cloud_media_pull_data_dto.h"

namespace OHOS::Media::CloudSync {
class EXPORT CloudMediaDfxUtils {
public:
    static void RecordCloudIdEmptyAudit(const PhotosDto &record, const std::string &scenario);
    static void RecordCloudIdEmptyAudit(const CloudMediaPullDataDto &pullData, const std::string &scenario);
};
}  // namespace OHOS::Media::CloudSync

#endif  // OHOS_CLOUD_MEDIA_DFX_UTILS_H