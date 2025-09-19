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

#include "get_batch_download_cloud_resources_count_vo.h"

#include <sstream>
#include "media_log.h"
#include "itypes_util.h"

namespace OHOS::Media {
using namespace std;
using namespace OHOS::DataShare;
// LCOV_EXCL_START
bool GetBatchDownloadCloudResourcesCountReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = DataSharePredicates::Unmarshal(this->predicates, parcel);
    return status;
}

bool GetBatchDownloadCloudResourcesCountReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = DataSharePredicates::Marshal(this->predicates, parcel);
    return status;
}

std::string GetBatchDownloadCloudResourcesCountReqBody::ToString() const
{
    std::stringstream ss;
    return ss.str();
}

bool GetBatchDownloadCloudResourcesCountRespBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadInt32(this->count);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool GetBatchDownloadCloudResourcesCountRespBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteInt32(this->count);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

std::string GetBatchDownloadCloudResourcesCountRespBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"count\": \"" << count
       << "}";
    return ss.str();
}
// LCOV_EXCL_STOP
};