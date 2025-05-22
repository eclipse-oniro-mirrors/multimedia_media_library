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

#include "media_itypes_utils.h"

namespace OHOS::Media::CloudSync {
bool CloudFileDataVo::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadString(this->fileName);
    parcel.ReadString(this->filePath);
    parcel.ReadInt64(this->size);
    return true;
}
bool CloudFileDataVo::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteString(this->fileName);
    parcel.WriteString(this->filePath);
    parcel.WriteInt64(this->size);
    return true;
}

bool CloudFileDataVo::Marshalling(const std::map<std::string, CloudFileDataVo> &result, MessageParcel &parcel)
{
    if (!parcel.WriteInt32(static_cast<int32_t>(result.size()))) {
        return false;
    }
    for (const auto &entry : result) {
        if (!parcel.WriteString(entry.first) || !entry.second.Marshalling(parcel)) {
            return false;
        }
    }
    return true;
}
bool CloudFileDataVo::Unmarshalling(std::map<std::string, CloudFileDataVo> &val, MessageParcel &parcel)
{
    int32_t size = 0;
    if (!parcel.ReadInt32(size)) {
        return false;
    }
    if (size < 0) {
        return false;
    }
    size_t readAbleSize = parcel.GetReadableBytes();
    if ((static_cast<size_t>(size) > readAbleSize) || static_cast<size_t>(size) > val.max_size()) {
        return false;
    }
    bool isValid;
    for (int32_t i = 0; i < size; i++) {
        std::string key;
        if ((!parcel.ReadString(key))) {
            return false;
        }
        CloudFileDataVo nodeObj;
        isValid = nodeObj.Unmarshalling(parcel);
        if (!isValid) {
            return false;
        }
        val.emplace(key, nodeObj);
    }
    return true;
}
std::string CloudFileDataVo::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"fileName\": \"" << this->fileName << "\", "
       << "\"filePath\": \"" << this->filePath << "\", "
       << "\"size\": " << this->size << "}";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync