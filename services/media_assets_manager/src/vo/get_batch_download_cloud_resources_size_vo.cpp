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

#include "get_batch_download_cloud_resources_size_vo.h"

#include <sstream>
#include "media_log.h"
#include "itypes_util.h"

namespace OHOS::Media {
using namespace std;
using namespace OHOS::DataShare;
// LCOV_EXCL_START
bool GetBatchDownloadCloudResourcesSizeReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = DataSharePredicates::Unmarshal(this->predicates, parcel);
    return status;
}

bool GetBatchDownloadCloudResourcesSizeReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = DataSharePredicates::Marshal(this->predicates, parcel);
    return status;
}

std::string GetBatchDownloadCloudResourcesSizeReqBody::ToString() const
{
    std::stringstream ss;
    return ss.str();
}

bool GetBatchDownloadCloudResourcesSizeRespBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadInt64(this->size);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadInt64(this->count);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool GetBatchDownloadCloudResourcesSizeRespBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteInt64(this->size);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteInt64(this->count);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

std::string GetBatchDownloadCloudResourcesSizeRespBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"size\": \"" << size  << ","
       << "\"count\": \"" << count
       << "}";
    return ss.str();
}
// LCOV_EXCL_STOP
};