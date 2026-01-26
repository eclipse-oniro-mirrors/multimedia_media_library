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

#include "on_delete_records_album_vo.h"

#include <sstream>

#include "media_itypes_utils.h"

namespace OHOS::Media::CloudSync {
bool OnDeleteAlbumData::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadString(this->cloudId);
    parcel.ReadBool(this->isSuccess);
    return true;
}
bool OnDeleteAlbumData::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteString(this->cloudId);
    parcel.WriteBool(this->isSuccess);
    return true;
}

std::string OnDeleteAlbumData::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"cloudId\": \"" << cloudId << "\", "
       << "\"isSuccess\": \"" << isSuccess << "}";
    return ss.str();
}

bool OnDeleteRecordsAlbumReqBody::AddSuccessResult(OnDeleteAlbumData &albumData)
{
    this->albums.emplace_back(albumData);
    return true;
}

bool OnDeleteRecordsAlbumReqBody::Unmarshalling(MessageParcel &parcel)
{
    return IPC::ITypeMediaUtil::UnmarshallingParcelable<OnDeleteAlbumData>(this->albums, parcel);
}

bool OnDeleteRecordsAlbumReqBody::Marshalling(MessageParcel &parcel) const
{
    return IPC::ITypeMediaUtil::MarshallingParcelable<OnDeleteAlbumData>(this->albums, parcel);
}

std::string OnDeleteRecordsAlbumReqBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"albums\": [";
    for (size_t i = 0; i < albums.size(); i++) {
        ss << albums[i].ToString();
        if (i != albums.size() - 1) {
            ss << ", ";
        }
    }
    ss << "]"
       << "}";
    return ss.str();
}

bool OnDeleteRecordsAlbumRespBody::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadInt32(this->failSize);
    return true;
}

bool OnDeleteRecordsAlbumRespBody::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteInt32(this->failSize);
    return true;
}

std::string OnDeleteRecordsAlbumRespBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"failSize\": " << failSize << "}";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync