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

#include "cloud_file_data_vo.h"

#include <sstream>

#include "media_log.h"
#include "media_file_utils.h"

namespace OHOS::Media::CloudSync {
static const int32_t MAX_DATA_SIZE = 10000;
bool CloudFileDataVo::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->fileName), false, "fileName");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->filePath), false, "filePath");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(this->size), false, "size");
    return true;
}
bool CloudFileDataVo::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->fileName), false, "fileName");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->filePath), false, "filePath");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(this->size), false, "size");
    return true;
}

bool CloudFileDataVo::Marshalling(const std::map<std::string, CloudFileDataVo> &result, MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(static_cast<int32_t>(result.size())), false, "result.size()");
    for (const auto &entry : result) {
        CHECK_AND_RETURN_RET_LOG(parcel.WriteString(entry.first), false, "entry.first");
        CHECK_AND_RETURN_RET_LOG(entry.second.Marshalling(parcel), false, "entry.second");
    }
    return true;
}
bool CloudFileDataVo::Unmarshalling(std::map<std::string, CloudFileDataVo> &val, MessageParcel &parcel)
{
    int32_t size = 0;
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(size), false, "size");
    CHECK_AND_RETURN_RET_LOG(size >= 0, false, "size >= 0");
    size_t readAbleSize = parcel.GetReadableBytes();
    bool isInValid = (static_cast<size_t>(size) > readAbleSize) || static_cast<size_t>(size) > val.max_size();
    CHECK_AND_RETURN_RET_LOG(!isInValid, false, "size invalid");
    size = size < MAX_DATA_SIZE ? size : MAX_DATA_SIZE;
    for (int32_t i = 0; i < size; i++) {
        std::string key;
        CHECK_AND_RETURN_RET_LOG(parcel.ReadString(key), false, "CloudFileDataVo key");
        CloudFileDataVo nodeObj;
        CHECK_AND_RETURN_RET_LOG(nodeObj.Unmarshalling(parcel), false, "CloudFileDataVo value");
        val.emplace(key, nodeObj);
    }
    return true;
}
std::string CloudFileDataVo::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"fileName\": \"" << MediaFileUtils::DesensitizeName(this->fileName) << "\","
       << "\"filePath\": \"" << MediaFileUtils::DesensitizePath(this->filePath) << "\","
       << "\"size\": " << this->size << "}";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync