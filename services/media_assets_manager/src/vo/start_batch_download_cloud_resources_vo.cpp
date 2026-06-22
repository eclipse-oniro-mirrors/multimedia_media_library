 /*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "start_batch_download_cloud_resources_vo.h"

#include <sstream>
#include "media_log.h"
#include "itypes_util.h"

namespace OHOS::Media {
// LCOV_EXCL_START
bool StartBatchDownloadCloudResourcesReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = ITypesUtil::Unmarshalling(this->uris, parcel);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool StartBatchDownloadCloudResourcesReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = ITypesUtil::Marshalling(this->uris, parcel);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

std::string StartBatchDownloadCloudResourcesReqBody::ToString() const
{
    std::stringstream ss;
    ss << "{\"uris\": [";
    for (uint32_t i = 0; i < uris.size(); ++i) {
        ss << "\"" << i << "\": \"" << uris[i] << "\"";
        if (i != uris.size() - 1) {
            ss << ",";
        }
    }
    ss << "]}";
    return ss.str();
}

bool StartBatchDownloadCloudResourcesRespBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = ITypesUtil::Unmarshalling(this->uriStatusMap, parcel);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool StartBatchDownloadCloudResourcesRespBody::Marshalling(MessageParcel &parcel) const
{
    bool status = ITypesUtil::Marshalling(this->uriStatusMap, parcel);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

std::string StartBatchDownloadCloudResourcesRespBody::ToString() const
{
    std::stringstream ss;
    ss << "{";
    for (const auto &entry : uriStatusMap) {
        ss << entry.first << " : " << std::to_string(entry.second) << ",";
    }
    ss << "}";
    return ss.str();
}
// LCOV_EXCL_STOP
};