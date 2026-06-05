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
#include "media_log.h"
#include "media_file_utils.h"

namespace OHOS::Media::CloudSync {
bool GetCheckRecordsReqBody::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(IPC::ITypeMediaUtil::Unmarshalling(this->cloudIds, parcel), false, "cloudIds");
    return true;
}

bool GetCheckRecordsReqBody::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(IPC::ITypeMediaUtil::Marshalling(this->cloudIds, parcel), false, "cloudIds");
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
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->cloudId), false, "cloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(this->size), false, "size");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->data), false, "data");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->displayName), false, "displayName");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->fileName), false, "fileName");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->mediaType), false, "mediaType");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->cloudVersion), false, "cloudVersion");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->position), false, "position");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(this->dateModified), false, "dateModified");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->dirty), false, "dirty");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->thmStatus), false, "thmStatus");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->syncStatus), false, "syncStatus");
    int32_t attachmentSize;
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(attachmentSize), false, "attachmentSize");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->fileSourceType), false, "fileSourceType");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->storagePath), false, "storagePath");
    for (int32_t i = 0; i < attachmentSize; ++i) {
        CloudFileDataVo vo;
        std::string key;
        CHECK_AND_RETURN_RET_LOG(parcel.ReadString(key), false, "CloudFileDataVo key");
        CHECK_AND_RETURN_RET_LOG(vo.Unmarshalling(parcel), false, "CloudFileDataVo value");
        this->attachment[key] = vo;
    }
    return true;
}

bool GetCheckRecordsRespBodyCheckData::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->cloudId), false, "cloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(this->size), false, "size");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->data), false, "data");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->displayName), false, "displayName");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->fileName), false, "fileName");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->mediaType), false, "mediaType");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->cloudVersion), false, "cloudVersion");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->position), false, "position");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(this->dateModified), false, "dateModified");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->dirty), false, "dirty");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->thmStatus), false, "thmStatus");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->syncStatus), false, "syncStatus");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->attachment.size()), false, "attachmentSize");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->fileSourceType), false, "fileSourceType");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->storagePath), false, "storagePath");
    for (auto &[key, value] : attachment) {
        CHECK_AND_RETURN_RET_LOG(parcel.WriteString(key), false, "CloudFileDataVo key");
        CHECK_AND_RETURN_RET_LOG(value.Marshalling(parcel), false, "CloudFileDataVo value");
    }
    return true;
}

std::string GetCheckRecordsRespBodyCheckData::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"cloudId\": \"" << this->cloudId << "\","
       << "\"size\": " << this->size << ","
       << "\"data\": \"" << this->data << "\","
       << "\"mediaType\": " << this->mediaType << ","
       << "\"cloudVersion\": " << this->cloudVersion << ","
       << "\"dateModified\": " << this->dateModified << ","
       << "\"dirty\": " << this->dirty << ","
       << "\"thmStatus\": " << this->thmStatus << ","
       << "\"syncStatus\": " << this->syncStatus << ","
       << "\"fileSourceType\": " << this->fileSourceType << ","
       << "\"storagePath\": " << MediaFileUtils::DesensitizePath(this->storagePath) << "\"";
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
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(size), false, "size");
    for (int32_t i = 0; i < size; ++i) {
        GetCheckRecordsRespBodyCheckData data;
        std::string key;
        CHECK_AND_RETURN_RET_LOG(parcel.ReadString(key), false, "key");
        CHECK_AND_RETURN_RET_LOG(data.Unmarshalling(parcel), false, "data");
        this->checkDataList[key] = data;
    }
    return true;
}

bool GetCheckRecordsRespBody::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->checkDataList.size()), false, "checkDataList.size()");
    for (auto [key, value] : this->checkDataList) {
        CHECK_AND_RETURN_RET_LOG(parcel.WriteString(key), false, "GetCheckRecordsRespBodyCheckData key");
        CHECK_AND_RETURN_RET_LOG(value.Marshalling(parcel), false, "GetCheckRecordsRespBodyCheckData value");
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