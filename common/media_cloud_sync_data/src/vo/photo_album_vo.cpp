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

#include "photo_album_vo.h"

#include <sstream>

#include "media_itypes_utils.h"

namespace OHOS::Media::CloudSync {
bool PhotoAlbumVo::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadInt32(this->albumId);
    parcel.ReadInt32(this->albumType);
    parcel.ReadInt32(this->albumSubType);
    parcel.ReadString(this->albumName);
    parcel.ReadString(this->lPath);
    parcel.ReadString(this->bundleName);
    parcel.ReadInt32(this->priority);
    return true;
}

bool PhotoAlbumVo::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteInt32(this->albumId);
    parcel.WriteInt32(this->albumType);
    parcel.WriteInt32(this->albumSubType);
    parcel.WriteString(this->albumName);
    parcel.WriteString(this->lPath);
    parcel.WriteString(this->bundleName);
    parcel.WriteInt32(this->priority);
    return true;
}

std::string PhotoAlbumVo::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"albumId\": " << this->albumId << ", "
       << "\"albumType\": " << this->albumType << ", "
       << "\"albumSubType\": " << this->albumSubType << ", "
       << "\"lPath\": \"" << this->lPath << "\", "
       << "\"bundleName\": \"" << this->bundleName << "\", "
       << "\"priority\": " << this->priority << ""
       << "}";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync