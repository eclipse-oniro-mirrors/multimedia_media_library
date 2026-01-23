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

#ifndef OHOS_MEDIA_IPC_ITYPES_MEDIA_UTIL_H
#define OHOS_MEDIA_IPC_ITYPES_MEDIA_UTIL_H

#include <string>
#include <vector>
#include <map>

#include "message_parcel.h"

namespace OHOS::Media::IPC::ITypeMediaUtil {
template <class T>
struct is_container : std::false_type {};
template <class T>
struct is_container<std::vector<T>> : std::true_type {};

static inline bool Marshalling(int16_t input, MessageParcel &data)
{
    return data.WriteInt16(input);
}

static inline bool Unmarshalling(int16_t &output, MessageParcel &data)
{
    return data.ReadInt16(output);
}

static inline bool Marshalling(uint16_t input, MessageParcel &data)
{
    return data.WriteUint16(input);
}

static inline bool Unmarshalling(uint16_t &output, MessageParcel &data)
{
    return data.ReadUint16(output);
}

static inline bool Marshalling(uint32_t input, MessageParcel &data)
{
    return data.WriteUint32(input);
}

static inline bool Unmarshalling(uint32_t &output, MessageParcel &data)
{
    return data.ReadUint32(output);
}

static inline bool Marshalling(int32_t input, MessageParcel &data)
{
    return data.WriteInt32(input);
}

static inline bool Unmarshalling(int32_t &output, MessageParcel &data)
{
    return data.ReadInt32(output);
}

static inline bool Marshalling(uint64_t input, MessageParcel &data)
{
    return data.WriteUint64(input);
}

static inline bool Unmarshalling(uint64_t &output, MessageParcel &data)
{
    return data.ReadUint64(output);
}

static inline bool Marshalling(int64_t input, MessageParcel &data)
{
    return data.WriteInt64(input);
}

static inline bool Unmarshalling(int64_t &output, MessageParcel &data)
{
    return data.ReadInt64(output);
}

static inline bool Marshalling(double input, MessageParcel &data)
{
    return data.WriteDouble(input);
}

static inline bool Unmarshalling(double &output, MessageParcel &data)
{
    return data.ReadDouble(output);
}

static inline bool Marshalling(const std::string &input, MessageParcel &data)
{
    return data.WriteString(input);
}

static inline bool Unmarshalling(std::string &output, MessageParcel &data)
{
    return data.ReadString(output);
}

template <class T>
bool MarshalToContainer(const T &val, MessageParcel &parcel)
{
    if (val.size() > INT_MAX) {
        return false;
    }

    if (!parcel.WriteInt32(static_cast<int32_t>(val.size()))) {
        return false;
    }

    for (auto &v : val) {
        if (!Marshalling(v, parcel)) {
            return false;
        }
    }
    return true;
}

// template <class T, typename std::enable_if<is_container<T>{}, int>::type>
template <class T>
bool UnmarshalFromContainer(T &val, MessageParcel &parcel)
{
    int32_t len = parcel.ReadInt32();
    if (len < 0) {
        return false;
    }

    size_t readAbleSize = parcel.GetReadableBytes();
    size_t size = static_cast<size_t>(len);
    if ((size > readAbleSize) || (size > val.max_size())) {
        return false;
    }

    val.clear();
    for (size_t i = 0; i < size; i++) {
        typename T::value_type value;
        if (!Unmarshalling(value, parcel)) {
            return false;
        }
        val.emplace_back(std::move(value));
    }
    return true;
}

template <class T>
bool MarshalToContainerParcelable(const std::vector<T> &val, MessageParcel &parcel)
{
    if (val.size() > INT_MAX) {
        return false;
    }

    if (!parcel.WriteInt32(static_cast<int32_t>(val.size()))) {
        return false;
    }

    for (auto &v : val) {
        if (!v.Marshalling(parcel)) {
            return false;
        }
    }
    return true;
}

template <class T>
bool UnmarshalFromContainerParcelable(std::vector<T> &val, MessageParcel &parcel)
{
    int32_t len = parcel.ReadInt32();
    if (len < 0) {
        return false;
    }

    size_t readAbleSize = parcel.GetReadableBytes();
    size_t size = static_cast<size_t>(len);
    if ((size > readAbleSize) || (size > val.max_size())) {
        return false;
    }

    val.clear();
    bool isValid;
    for (size_t i = 0; i < size; i++) {
        T nodeObj;
        isValid = nodeObj.Unmarshalling(parcel);
        if (!isValid) {
            return false;
        }
        val.emplace_back(std::move(nodeObj));
    }
    return true;
}
template <class T>
bool Marshalling(const std::vector<T> &val, MessageParcel &parcel)
{
    return MarshalToContainer(val, parcel);
}

template <class T>
bool Unmarshalling(std::vector<T> &val, MessageParcel &parcel)
{
    return UnmarshalFromContainer(val, parcel);
}
template <class T>
bool MarshallingParcelable(const std::vector<T> &val, MessageParcel &parcel)
{
    return MarshalToContainerParcelable(val, parcel);
}

template <class T>
bool UnmarshallingParcelable(std::vector<T> &val, MessageParcel &parcel)
{
    return UnmarshalFromContainerParcelable(val, parcel);
}

template <class K, class V>
bool Marshalling(const std::unordered_map<K, V> &result, MessageParcel &parcel)
{
    if (!parcel.WriteInt32(static_cast<int32_t>(result.size()))) {
        return false;
    }
    for (const auto &entry : result) {
        if ((!Marshalling(entry.first, parcel)) || (!Marshalling(entry.second, parcel))) {
            return false;
        }
    }
    return true;
}

template <class K, class V>
bool Unmarshalling(std::unordered_map<K, V> &val, MessageParcel &parcel)
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
    for (int32_t i = 0; i < size; i++) {
        K key;
        if ((!Unmarshalling(key, parcel)) || (!Unmarshalling(val[key], parcel))) {
            return false;
        }
    }
    return true;
}

/**
 * @brief The following two functions are used in scenarios where IPC communication parameters are extremely large,
 * serializing objects to shared memory and deserializing objects from shared memory. The upper limit of shared memory
 * is 128M. Note: For large amounts of data, this function can only be called once per IPC.
 */
bool MarshalStrVec(const std::vector<std::string> &strVec, MessageParcel &parcel);

bool UnmarshalStrVec(std::vector<std::string> &strVec, MessageParcel &parcel);

bool MarshalMapVec(const std::vector<std::unordered_map<std::string, std::string>> &val, MessageParcel &parcel);

bool UnmarshalMapVec(std::vector<std::unordered_map<std::string, std::string>> &val, MessageParcel &parcel);
}  // namespace OHOS::Media::IPC::ITypeMediaUtil
#endif  // OHOS_MEDIA_IPC_ITYPES_MEDIA_UTIL_H
