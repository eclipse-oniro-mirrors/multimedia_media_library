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

#include "get_download_thm_num_vo.h"

#include <sstream>

#include "media_itypes_utils.h"

namespace OHOS::Media::CloudSync {

bool GetDownloadThmNumReqBody::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadInt32(this->type);
    return true;
}
bool GetDownloadThmNumReqBody::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteInt32(this->type);
    return true;
}

std::string GetDownloadThmNumReqBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"type\": " << this->type << "}";
    return ss.str();
}

bool GetDownloadThmNumRespBody::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadInt32(this->totalNum);
    parcel.ReadInt32(this->type);
    return true;
}

bool GetDownloadThmNumRespBody::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteInt32(this->totalNum);
    parcel.WriteInt32(this->type);
    return true;
}

std::string GetDownloadThmNumRespBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"type\": " << this->type << "\"num\": " << this->totalNum << "}";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync