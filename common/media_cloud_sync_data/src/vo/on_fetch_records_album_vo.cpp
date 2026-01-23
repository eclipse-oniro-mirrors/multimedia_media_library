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

#include "on_fetch_records_album_vo.h"

#include <sstream>

#include "media_itypes_utils.h"
#include "media_log.h"

namespace OHOS::Media::CloudSync {
bool OnFetchRecordsAlbumReqBody::AlbumReqData::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadString(this->cloudId);
    parcel.ReadString(this->localPath);
    parcel.ReadString(this->albumName);
    parcel.ReadString(this->albumBundleName);
    parcel.ReadString(this->localLanguage);
    parcel.ReadInt32(this->albumId);
    parcel.ReadInt32(this->priority);
    parcel.ReadInt32(this->albumType);
    parcel.ReadInt32(this->albumSubType);
    parcel.ReadInt64(this->albumDateCreated);
    parcel.ReadInt64(this->albumDateAdded);
    parcel.ReadInt64(this->albumDateModified);
    parcel.ReadBool(this->isDelete);
    parcel.ReadInt32(this->coverUriSource);
    parcel.ReadString(this->coverCloudId);
    return true;
}
bool OnFetchRecordsAlbumReqBody::AlbumReqData::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteString(this->cloudId);
    parcel.WriteString(this->localPath);
    parcel.WriteString(this->albumName);
    parcel.WriteString(this->albumBundleName);
    parcel.WriteString(this->localLanguage);
    parcel.WriteInt32(this->albumId);
    parcel.WriteInt32(this->priority);
    parcel.WriteInt32(this->albumType);
    parcel.WriteInt32(this->albumSubType);
    parcel.WriteInt64(this->albumDateCreated);
    parcel.WriteInt64(this->albumDateAdded);
    parcel.WriteInt64(this->albumDateModified);
    parcel.WriteBool(this->isDelete);
    parcel.WriteInt32(this->coverUriSource);
    parcel.WriteString(this->coverCloudId);
    return true;
}

std::string OnFetchRecordsAlbumReqBody::AlbumReqData::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"cloudId\": \"" << cloudId << "\","
       << "\"albumBundleName\": \"" << albumBundleName << "\","
       << "\"localLanguage\": \"" << localLanguage << "\","
       << "\"albumId\": \"" << albumId << "\","
       << "\"priority\": \"" << priority << "\","
       << "\"albumType\": \"" << albumType << "\","
       << "\"albumSubType\": \"" << albumSubType << "\","
       << "\"albumDateCreated\": \"" << albumDateCreated << "\","
       << "\"albumDateAdded\": \"" << albumDateAdded << "\","
       << "\"albumDateModified\": \"" << albumDateModified << "\","
       << "\"isDelete\": \"" << isDelete << "\","
       << "\"coverUriSource\": \"" << coverUriSource << "\","
       << "\"coverCloudId\": \"" << coverCloudId << "\","
       << "}";
    return ss.str();
}

bool OnFetchRecordsAlbumReqBody::Unmarshalling(MessageParcel &parcel)
{
    return IPC::ITypeMediaUtil::UnmarshallingParcelable<AlbumReqData>(this->albums, parcel);
}

bool OnFetchRecordsAlbumReqBody::Marshalling(MessageParcel &parcel) const
{
    return IPC::ITypeMediaUtil::MarshallingParcelable<AlbumReqData>(this->albums, parcel);
}

std::string OnFetchRecordsAlbumReqBody::ToString() const
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

bool OnFetchRecordsAlbumRespBody::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET(IPC::ITypeMediaUtil::Unmarshalling<std::string>(this->failedRecords, parcel), false);
    return IPC::ITypeMediaUtil::Unmarshalling<int32_t>(this->stats, parcel);
}

bool OnFetchRecordsAlbumRespBody::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET(IPC::ITypeMediaUtil::Marshalling<std::string>(this->failedRecords, parcel), false);
    return IPC::ITypeMediaUtil::Marshalling<int32_t>(this->stats, parcel);
}

std::string OnFetchRecordsAlbumRespBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"failedRecords\": [";
    for (size_t i = 0; i < failedRecords.size(); i++) {
        ss << failedRecords[i];
        if (i != failedRecords.size() - 1) {
            ss << ", ";
        }
    }
    ss << "], \"stats\": [";
    for (size_t i = 0; i < stats.size(); i++) {
        ss << stats[i];
        if (i != stats.size() - 1) {
            ss << ", ";
        }
    }
    ss << "]"
       << "}";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync