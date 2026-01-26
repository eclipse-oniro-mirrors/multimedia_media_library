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

#include "get_download_thm_vo.h"

#include <sstream>

#include "media_itypes_utils.h"

namespace OHOS::Media::CloudSync {
bool GetDownloadThmReqBody::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadInt32(this->size);
    parcel.ReadInt32(this->type);
    parcel.ReadInt32(this->offset);
    parcel.ReadBool(this->isDownloadDisplayFirst);
    return true;
}

bool GetDownloadThmReqBody::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteInt32(this->size);
    parcel.WriteInt32(this->type);
    parcel.WriteInt32(this->offset);
    parcel.WriteBool(this->isDownloadDisplayFirst);
    return true;
}

std::string GetDownloadThmReqBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"size\": " << this->size << ","
       << "\"type\": " << this->type << ","
       << "\"offset\": " << this->offset << ","
       << "\"isDownloadDisplayFirst\": " << this->isDownloadDisplayFirst << ""
       << "}";
    return ss.str();
}

bool GetDownloadThmRespBody::Unmarshalling(MessageParcel &parcel)
{
    return PhotosVo::Unmarshalling(this->photos, parcel);
}

bool GetDownloadThmRespBody::Marshalling(MessageParcel &parcel) const
{
    return PhotosVo::Marshalling(this->photos, parcel);
}

std::string GetDownloadThmRespBody::ToString() const
{
    std::stringstream ss;
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync