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

#ifndef OHOS_MEDIA_CLOUD_MEDIA_PHOTO_HANDLER_PROCESSOR_H
#define OHOS_MEDIA_CLOUD_MEDIA_PHOTO_HANDLER_PROCESSOR_H

#include <string>
#include <unordered_map>

#include "cloud_meta_data.h"
#include "get_retey_records_vo.h"
#include "medialibrary_errno.h"

namespace OHOS::Media::CloudSync {
class CloudMediaPhotoHandlerProcessor {
public:
    int32_t ConvertFromRetryRecordsRespBodyToCloudMetaData(const GetRetryRecordsRespBody &respBody,
        std::unordered_map<std::string, CloudMetaData> &retryRecords);
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_MEDIA_PHOTO_HANDLER_PROCESSOR_H
