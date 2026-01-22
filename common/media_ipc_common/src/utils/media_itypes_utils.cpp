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

#include "media_itypes_utils.h"

#include <sstream>

#include "media_log.h"

namespace OHOS::Media::IPC::ITypeMediaUtil {
// Maximum value of IPC shared memory
static const size_t MAX_IPC_SIZE = 128 * 1024 * 1024;

template <typename T>
bool WriteBasicTypeToStream(std::ostringstream &oss, const T &value)
{
    oss.write(reinterpret_cast<const char *>(&value), sizeof(value));
    return oss.good();
}

bool WriteStringToStream(std::ostringstream &oss, const std::string &str)
{
    // write string length
    size_t len = str.length();
    if (!WriteBasicTypeToStream(oss, len)) {
        return false;
    }
    // write string data
    if (len > 0) {
        oss.write(str.data(), len);
    }
    return oss.good();
}

bool WriteStrVecToStream(std::ostringstream &oss, const std::vector<std::string> &strVec)
{
    // write vector size
    size_t len = strVec.size();
    if (!WriteBasicTypeToStream(oss, len)) {
        return false;
    }
    for (const auto &str : strVec) {
        if (!WriteStringToStream(oss, str)) {
            return false;
        }
    }
    return oss.good();
}

bool WriteMapVecToStream(std::ostringstream &oss,
                         const std::vector<std::unordered_map<std::string, std::string>> &mapVec)
{
    size_t len = mapVec.size();
    if (!WriteBasicTypeToStream(oss, len)) {
        return false;
    }
    for (const auto &map : mapVec) {
        len = map.size();
        if (!WriteBasicTypeToStream(oss, len)) {
            return false;
        }
        for (const auto &entry : map) {
            if ((!WriteStringToStream(oss, entry.first)) || (!WriteStringToStream(oss, entry.second))) {
                return false;
            }
        }
    }
    return oss.good();
}

template <typename T>
bool ReadStreamToBasicType(std::istringstream &iss, T &value)
{
    iss.read(reinterpret_cast<char *>(&value), sizeof(value));
    return iss.good();
}

bool ReadStreamToString(std::istringstream &iss, std::string &str)
{
    // Get string length
    size_t len;
    if (!ReadStreamToBasicType(iss, len)) {
        return false;
    }
    // Get string content
    if (len > 0) {
        str.resize(len, '\0');
        iss.read(str.data(), len);
    }
    return iss.good();
}

bool ReadStreamToStrVec(std::istringstream &iss, std::vector<std::string> &strVec)
{
    // Get vec length
    size_t len;
    if (!ReadStreamToBasicType(iss, len)) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        std::string str;
        if (!ReadStreamToString(iss, str)) {
            return false;
        }
        strVec.push_back(str);
    }
    return iss.good();
}

bool ReadStreamToMapVec(std::istringstream &iss,
                        std::vector<std::unordered_map<std::string, std::string>> &mapVec)
{
    size_t len;
    if (!ReadStreamToBasicType(iss, len)) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        std::unordered_map<std::string, std::string> map;
        size_t mapLen;
        if (!ReadStreamToBasicType(iss, mapLen)) {
            return false;
        }
        for (size_t j = 0; j < mapLen; j++) {
            std::string key;
            std::string value;
            if (!ReadStreamToString(iss, key) || !ReadStreamToString(iss, value)) {
                return false;
            }
            map[key] = value;
        }
        mapVec.push_back(map);
    }
    return iss.good();
}

bool MarshalStrVec(const std::vector<std::string> &strVec, MessageParcel &parcel)
{
    std::ostringstream oss;
    if (!WriteStrVecToStream(oss, strVec)) {
        MEDIA_ERR_LOG("WriteStrVecToStream failed.");
        return false;
    }
    std::string str = oss.str();
    size_t size = str.length();
    if (size < 1 || size > MAX_IPC_SIZE) {
        MEDIA_ERR_LOG("The length of strVec converted to string is invalid.");
        return false;
    }
    if (!parcel.WriteUint32(static_cast<uint32_t>(size))) {
        MEDIA_ERR_LOG("Write size failed.");
        return false;
    }
    return parcel.WriteRawData(reinterpret_cast<const void *>(str.data()), size);
}

bool UnmarshalStrVec(std::vector<std::string> &strVec, MessageParcel &parcel)
{
    size_t size = static_cast<size_t>(parcel.ReadUint32());
    if (size < 1 || size > MAX_IPC_SIZE) {
        MEDIA_ERR_LOG("The length of strVec converted to string is invalid.");
        return false;
    }
    const char *buffer = reinterpret_cast<const char *>(parcel.ReadRawData(size));
    if (buffer == nullptr) {
        MEDIA_ERR_LOG("ReadRawData failed.");
        return false;
    }
    std::istringstream iss(std::string(buffer, size));
    return ReadStreamToStrVec(iss, strVec);
}

bool MarshalMapVec(const std::vector<std::unordered_map<std::string, std::string>> &val, MessageParcel &parcel)
{
    std::ostringstream oss;
    if (!WriteMapVecToStream(oss, val)) {
        MEDIA_ERR_LOG("WriteMapVecToStream failed.");
        return false;
    }
    std::string str = oss.str();
    size_t size = str.length();
    if (size < 1 || size > MAX_IPC_SIZE) {
        MEDIA_ERR_LOG("The length of MapVec converted to string is invalid.");
        return false;
    }
    if (!parcel.WriteUint32(static_cast<uint32_t>(size))) {
        MEDIA_ERR_LOG("Write size failed.");
        return false;
    }
    return parcel.WriteRawData(reinterpret_cast<const void *>(str.data()), size);
}

bool UnmarshalMapVec(std::vector<std::unordered_map<std::string, std::string>> &val, MessageParcel &parcel)
{
    size_t size = static_cast<size_t>(parcel.ReadUint32());
    if (size < 1 || size > MAX_IPC_SIZE) {
        MEDIA_ERR_LOG("The length of mapVec converted to string is invalid.");
        return false;
    }
    const char *buffer = reinterpret_cast<const char *>(parcel.ReadRawData(size));
    if (buffer == nullptr) {
        MEDIA_ERR_LOG("ReadRawData failed.");
        return false;
    }
    std::istringstream iss(std::string(buffer, size));
    return ReadStreamToMapVec(iss, val);
}
}  // namespace OHOS::Media::IPC::ITypeMediaUtil
