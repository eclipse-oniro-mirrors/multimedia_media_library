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

#define MLOG_TAG "Media_Cloud_Vo"

#include "get_retey_records_vo.h"

#include <sstream>

#include "media_itypes_utils.h"
#include "media_log.h"
#include "media_file_utils.h"

namespace OHOS::Media::CloudSync {
bool GetRetryRecordsDataVo::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->cloudId), false, "cloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->shareAlbumOwner), false, "shareAlbumOwner");
    return true;
}

bool GetRetryRecordsDataVo::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->cloudId), false, "cloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->shareAlbumOwner), false, "shareAlbumOwner");
    return true;
}

std::string GetRetryRecordsDataVo::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"cloudId\": \"" << this->cloudId << "\","
       << "\"shareAlbumOwner\": \"" << this->shareAlbumOwner << "\""
       << "}";
    return ss.str();
}

bool GetRetryRecordsRespBody::Unmarshalling(MessageParcel &parcel)
{
    bool ret = IPC::ITypeMediaUtil::Unmarshalling<std::string, GetRetryRecordsDataVo>(this->retryDataList, parcel);
    CHECK_AND_RETURN_RET_LOG(ret, false, "retryDataList");
    return ret;
}

bool GetRetryRecordsRespBody::Marshalling(MessageParcel &parcel) const
{
    bool ret = IPC::ITypeMediaUtil::Marshalling<std::string, GetRetryRecordsDataVo>(this->retryDataList, parcel);
    CHECK_AND_RETURN_RET_LOG(ret, false, "retryDataList");
    return ret;
}

std::string GetRetryRecordsRespBody::ToString() const
{
    std::stringstream ss;
    ss << "[";
    for (const auto &entry : this->retryDataList) {
        ss << "{\"" << entry.first << "\":" << entry.second.ToString() << "}, ";
    }
    ss << "]";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync