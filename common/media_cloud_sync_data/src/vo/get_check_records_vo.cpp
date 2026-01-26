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

#include "get_check_records_vo.h"

#include <sstream>

#include "media_itypes_utils.h"

namespace OHOS::Media::CloudSync {
bool GetCheckRecordsReqBody::Unmarshalling(MessageParcel &parcel)
{
    return IPC::ITypeMediaUtil::Unmarshalling(this->cloudIds, parcel);
}

bool GetCheckRecordsReqBody::Marshalling(MessageParcel &parcel) const
{
    IPC::ITypeMediaUtil::Marshalling(this->cloudIds, parcel);
    return true;
}

std::string GetCheckRecordsReqBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"cloudIds\": [";
    for (size_t i = 0; i < this->cloudIds.size(); i++) {
        ss << this->cloudIds[i];
        if (i != this->cloudIds.size() - 1) {
            ss << ", ";
        }
    }
    ss << "]"
       << "}";
    return ss.str();
}

bool GetCheckRecordsRespBodyCheckData::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadString(this->cloudId);
    parcel.ReadInt64(this->size);
    parcel.ReadString(this->data);
    parcel.ReadString(this->displayName);
    parcel.ReadString(this->fileName);
    parcel.ReadInt32(this->mediaType);
    parcel.ReadInt32(this->cloudVersion);
    parcel.ReadInt32(this->position);
    parcel.ReadInt64(this->dateModified);
    parcel.ReadInt32(this->dirty);
    parcel.ReadInt32(this->thmStatus);
    parcel.ReadInt32(this->syncStatus);
    int32_t attachmentSize;
    parcel.ReadInt32(attachmentSize);
    parcel.ReadInt32(this->fileSourceType);
    parcel.ReadString(this->storagePath);
    for (int32_t i = 0; i < attachmentSize; ++i) {
        CloudFileDataVo vo;
        std::string key;
        parcel.ReadString(key);
        vo.Unmarshalling(parcel);
        this->attachment[key] = vo;
    }
    return true;
}

bool GetCheckRecordsRespBodyCheckData::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteString(this->cloudId);
    parcel.WriteInt64(this->size);
    parcel.WriteString(this->data);
    parcel.WriteString(this->displayName);
    parcel.WriteString(this->fileName);
    parcel.WriteInt32(this->mediaType);
    parcel.WriteInt32(this->cloudVersion);
    parcel.WriteInt32(this->position);
    parcel.WriteInt64(this->dateModified);
    parcel.WriteInt32(this->dirty);
    parcel.WriteInt32(this->thmStatus);
    parcel.WriteInt32(this->syncStatus);
    parcel.WriteInt32(this->attachment.size());
    parcel.WriteInt32(this->fileSourceType);
    parcel.WriteString(this->storagePath);
    for (auto &[key, value] : attachment) {
        parcel.WriteString(key);
        value.Marshalling(parcel);
    }
    return true;
}

std::string GetCheckRecordsRespBodyCheckData::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"cloudId\": \"" << this->cloudId << "\", "
       << "\"size\": " << this->size << ", "
       << "\"data\": \"" << this->data << "\", "
       << "\"mediaType\": " << this->mediaType << ", "
       << "\"cloudVersion\": " << this->cloudVersion << ", "
       << "\"dateModified\": " << this->dateModified << ", "
       << "\"dirty\": " << this->dirty << "\""
       << "\"thmStatus\": " << this->thmStatus << "\""
       << "\"syncStatus\": " << this->syncStatus << "\""
       << "\"fileSourceType\": " << this->fileSourceType << "\""
       << "\"storagePath\": " << this->storagePath << "\"";
    ss << ",[";
    uint32_t index = 0;
    for (const auto &[key, value] : attachment) {
        ss << "{\"" << key << "\": " << value.ToString() << "}";
        if (index != attachment.size() - 1) {
            ss << ",";
        }
        index++;
    }
    ss << "]}";
    return ss.str();
}

bool GetCheckRecordsRespBody::Unmarshalling(MessageParcel &parcel)
{
    int32_t size = 0;
    parcel.ReadInt32(size);
    if (size <= 0) {
        return true;
    }
    for (int32_t i = 0; i < size; ++i) {
        GetCheckRecordsRespBodyCheckData data;
        std::string key;
        parcel.ReadString(key);
        data.Unmarshalling(parcel);
        this->checkDataList[key] = data;
    }
    return true;
}

bool GetCheckRecordsRespBody::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteInt32(this->checkDataList.size());
    for (auto [key, value] : this->checkDataList) {
        parcel.WriteString(key);
        if (!value.Marshalling(parcel)) {
            return false;
        }
    }
    return true;
}

std::string GetCheckRecordsRespBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"checkDataList\": [";
    auto it = checkDataList.begin();
    auto endIt = checkDataList.end();
    while (it != endIt) {
        if (std::next(it) == endIt) {
            ss << "{\"" << it->first << "\":" << it->second.ToString() << "}";
        } else {
            ss << "{\"" << it->first << "\":" << it->second.ToString() << "},";
        }
        ++it;
    }
    ss << "]"
       << "}";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync