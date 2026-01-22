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

#include "photos_vo.h"

#include <sstream>

#include "media_itypes_utils.h"
#include "media_log.h"

namespace OHOS::Media::CloudSync {
bool PhotosVo::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadInt32(this->fileId);
    parcel.ReadString(this->cloudId);
    parcel.ReadInt64(this->size);
    parcel.ReadInt64(this->modifiedTime);
    parcel.ReadString(this->path);
    parcel.ReadString(this->fileName);
    parcel.ReadString(this->localPath);
    parcel.ReadString(this->originalCloudId);
    parcel.ReadInt32(this->type);
    parcel.ReadInt32(this->orientation);
    parcel.ReadInt32(this->fileSourceType);
    parcel.ReadString(this->storagePath);
    parcel.ReadInt32(this->hidden);
    parcel.ReadInt64(this->dateTrashed);
    parcel.ReadInt32(this->attributesMediaType);
    CloudFileDataVo::Unmarshalling(this->attachment, parcel);
    return true;
}

bool PhotosVo::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteInt32(this->fileId);
    parcel.WriteString(this->cloudId);
    parcel.WriteInt64(this->size);
    parcel.WriteInt64(this->modifiedTime);
    parcel.WriteString(this->path);
    parcel.WriteString(this->fileName);
    parcel.WriteString(this->localPath);
    parcel.WriteString(this->originalCloudId);
    parcel.WriteInt32(this->type);
    parcel.WriteInt32(this->orientation);
    parcel.WriteInt32(this->fileSourceType);
    parcel.WriteString(this->storagePath);
    parcel.WriteInt32(this->hidden);
    parcel.WriteInt64(this->dateTrashed);
    parcel.WriteInt32(this->attributesMediaType);
    CloudFileDataVo::Marshalling(this->attachment, parcel);
    return true;
}

bool PhotosVo::Marshalling(const std::vector<PhotosVo> &result, MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET(parcel.WriteInt32(static_cast<int32_t>(result.size())), false);
    for (const auto &entry : result) {
        if (!entry.Marshalling(parcel)) {
            return false;
        }
    }
    return true;
}

bool PhotosVo::Unmarshalling(std::vector<PhotosVo> &val, MessageParcel &parcel)
{
    int32_t len = parcel.ReadInt32();
    CHECK_AND_RETURN_RET(len >= 0, false);

    size_t readAbleSize = parcel.GetReadableBytes();
    size_t size = static_cast<size_t>(len);
    if ((size > readAbleSize) || (size > val.max_size())) {
        return false;
    }

    val.clear();
    bool isValid;
    for (size_t i = 0; i < size; i++) {
        PhotosVo nodeObj;
        isValid = nodeObj.Unmarshalling(parcel);
        CHECK_AND_RETURN_RET(isValid, false);
        val.emplace_back(nodeObj);
    }
    return true;
}

std::string PhotosVo::ToString(std::map<std::string, CloudFileDataVo> dataMap) const
{
    std::stringstream ss;
    bool first = true;
    for (auto &node : dataMap) {
        if (!first) {
            ss << ", ";
        }
        ss << "\"" << node.first << "\": " << node.second.ToString();
        first = false;
    }
    return ss.str();
}

std::string PhotosVo::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"fileId\": " << this->fileId << ", "
       << "\"cloudId\": \"" << this->cloudId << "\", "
       << "\"originalCloudId\": \"" << this->originalCloudId << "\", "
       << "\"size\": " << this->size << ", "
       << "\"modifiedTime\": " << this->modifiedTime << ", "
       << "\"type\": " << this->type << ", "
       << "\"fileSourceType\": " << this->fileSourceType << ", "
       << "\"storagePath\": " << this->storagePath << ", "
       << "\"hidden\": " << this->hidden << ", "
       << "\"dateTrashed\": " << this->dateTrashed << ", "
       << "\"attachment\": {" << this->ToString(this->attachment) << "}"
       << "}";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync