/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#define MLOG_TAG "Media_Client"

#include "cloud_media_photo_handler_processor.h"

#include <string>
#include <unordered_map>

#include "media_log.h"

namespace OHOS::Media::CloudSync {
int32_t CloudMediaPhotoHandlerProcessor::ConvertFromRetryRecordsRespBodyToCloudMetaData(
    const GetRetryRecordsRespBody &respBody, std::unordered_map<std::string, CloudMetaData> &retryRecords)
{
    for (const auto &[cloudId, info] : respBody.retryDataList) {
        CloudMetaData cloudMetaData;
        cloudMetaData.cloudId = info.cloudId;
        cloudMetaData.shareAlbumOwner = info.shareAlbumOwner;
        retryRecords[cloudId] = cloudMetaData;
        MEDIA_DEBUG_LOG("GetRetryRecords CloudMetaData: %{public}s", cloudMetaData.ToString().c_str());
    }
    return E_OK;
}
}  // namespace OHOS::Media::CloudSync
