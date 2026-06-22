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

#include "media_log.h"
#include "media_file_utils.h"

namespace OHOS::Media::CloudSync {
bool PhotosVo::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->fileId), false, "fileId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->cloudId), false, "cloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(this->size), false, "size");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(this->modifiedTime), false, "modifiedTime");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->path), false, "path");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->fileName), false, "fileName");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->localPath), false, "localPath");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->originalCloudId), false, "originalCloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->type), false, "type");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->orientation), false, "orientation");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->fileSourceType), false, "fileSourceType");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->storagePath), false, "storagePath");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->uniqueId), false, "uniqueId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->packageName), false, "packageName");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->photoRiskStatus), false, "photoRiskStatus");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->hidden), false, "hidden");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(this->dateTrashed), false, "dateTrashed");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->attributesMediaType), false, "attributesMediaType");
    CHECK_AND_RETURN_RET_LOG(CloudFileDataVo::Unmarshalling(this->attachment, parcel), false, "attachment");
    return true;
}

bool PhotosVo::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->fileId), false, "fileId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->cloudId), false, "cloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(this->size), false, "size");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(this->modifiedTime), false, "modifiedTime");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->path), false, "path");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->fileName), false, "fileName");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->localPath), false, "localPath");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->originalCloudId), false, "originalCloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->type), false, "type");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->orientation), false, "orientation");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->fileSourceType), false, "fileSourceType");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->storagePath), false, "storagePath");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->uniqueId), false, "uniqueId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->packageName), false, "packageName");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->photoRiskStatus), false, "photoRiskStatus");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->hidden), false, "hidden");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(this->dateTrashed), false, "dateTrashed");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->attributesMediaType), false, "attributesMediaType");
    CHECK_AND_RETURN_RET_LOG(CloudFileDataVo::Marshalling(this->attachment, parcel), false, "attachment");
    return true;
}

bool PhotosVo::Marshalling(const std::vector<PhotosVo> &result, MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(static_cast<int32_t>(result.size())), false, "result.size()");
    for (const auto &entry : result) {
        CHECK_AND_RETURN_RET_LOG(entry.Marshalling(parcel), false, "PhotosVo");
    }
    return true;
}

bool PhotosVo::Unmarshalling(std::vector<PhotosVo> &val, MessageParcel &parcel)
{
    int32_t len = parcel.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(len >= 0, false, "len >= 0");

    size_t readAbleSize = parcel.GetReadableBytes();
    size_t size = static_cast<size_t>(len);
    bool isInValid = (size > readAbleSize) || (size > val.max_size());
    CHECK_AND_RETURN_RET_LOG(!isInValid, false, "size invalid");
    val.clear();
    for (size_t i = 0; i < size; i++) {
        PhotosVo nodeObj;
        CHECK_AND_RETURN_RET_LOG(nodeObj.Unmarshalling(parcel), false, "PhotosVo");
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
       << "\"storagePath\": \"" << MediaFileUtils::DesensitizePath(this->storagePath) << "\", "
       << "\"uniqueId\": " << this->uniqueId << ", "
       << "\"packageName\": " << this->packageName << ", "
       << "\"photoRiskStatus\": " << this->photoRiskStatus << ", "
       << "\"hidden\": " << this->hidden << ", "
       << "\"dateTrashed\": " << this->dateTrashed << ", "
       << "\"attachment\": {" << this->ToString(this->attachment) << "}"
       << "}";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync