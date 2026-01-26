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

#include "get_aging_file_vo.h"

#include <sstream>

#include "media_itypes_utils.h"

namespace OHOS::Media::CloudSync {
bool GetAgingFileReqBody::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadInt64(this->time);
    parcel.ReadInt32(this->mediaType);
    parcel.ReadInt32(this->sizeLimit);
    parcel.ReadInt32(this->offset);
    return true;
}

bool GetAgingFileReqBody::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteInt64(this->time);
    parcel.WriteInt32(this->mediaType);
    parcel.WriteInt32(this->sizeLimit);
    parcel.WriteInt32(this->offset);
    return true;
}

std::string GetAgingFileReqBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"time\": \"" << this->time << "\", "
       << "\"mediaType\": " << this->mediaType << ", "
       << "\"sizeLimit\": " << this->sizeLimit << ""
       << "}";
    return ss.str();
}

bool GetAgingFileRespBody::Unmarshalling(MessageParcel &parcel)
{
    return PhotosVo::Unmarshalling(this->photos, parcel);
}

bool GetAgingFileRespBody::Marshalling(MessageParcel &parcel) const
{
    return PhotosVo::Marshalling(this->photos, parcel);
}

std::string GetAgingFileRespBody::ToString() const
{
    std::stringstream ss;
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync