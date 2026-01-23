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

#include "cloud_error_detail_vo.h"

#include <sstream>

#include "media_itypes_utils.h"

namespace OHOS::Media::CloudSync {
bool CloudErrorDetail::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadString(domain);
    parcel.ReadString(reason);
    parcel.ReadString(errorCode);
    parcel.ReadString(description);
    parcel.ReadString(errorPos);
    parcel.ReadString(errorParam);
    parcel.ReadInt32(detailCode);
    return true;
}

bool CloudErrorDetail::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteString(domain);
    parcel.WriteString(reason);
    parcel.WriteString(errorCode);
    parcel.WriteString(description);
    parcel.WriteString(errorPos);
    parcel.WriteString(errorParam);
    parcel.WriteInt32(detailCode);
    return true;
}

std::string CloudErrorDetail::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"reason\": \"" << reason << "\","
       << "\"description\": \"" << description << "\","
       << "\"domain\": \"" << domain << "\","
       << "\"errorCode\": \"" << errorCode << "\","
       << "\"errorPos\": \"" << errorPos << "\","
       << "\"errorParam\": \"" << errorParam << "\","
       << "\"detailCode\": \"" << detailCode << "\""
       << "}";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync