/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "dfx_photo_error_utils.h"

#include "media_column.h"
#include "medialibrary_db_const.h"

namespace OHOS {
namespace Media {
// 维度归一化：当前仅 south_device_type 有 VISIT(-1)->0 规则；其余 passthrough，预留扩展点。

int32_t NormalizeFileSourceType(int32_t fileSourceType)
{
    return fileSourceType;
}

int32_t NormalizeMediaType(int32_t mediaType)
{
    return mediaType;
}

int32_t NormalizeSouthDeviceType(int32_t southDeviceType)
{
    // VISIT(-1) 归一为 NULL(0)（本地原生）
    return (southDeviceType == static_cast<int32_t>(SouthDeviceType::SOUTH_DEVICE_VISIT))
        ? static_cast<int32_t>(SouthDeviceType::SOUTH_DEVICE_NULL)
        : southDeviceType;
}

int32_t NormalizePosition(int32_t position)
{
    return position;
}

int32_t EncodePhotoErrorType(const PhotoErrorDimension& dimension)
{
    int32_t fileSourceType = NormalizeFileSourceType(dimension.fileSourceType);
    int32_t southDeviceType = NormalizeSouthDeviceType(dimension.southDeviceType);
    int32_t position = NormalizePosition(dimension.position);
    int32_t mediaType = NormalizeMediaType(dimension.mediaType);
    int32_t errorSubtype = static_cast<int32_t>(dimension.errorSubtype);
    return fileSourceType * FILE_SOURCE_TYPE_BASE + southDeviceType * SOUTH_DEVICE_TYPE_BASE
        + position * POSITION_BASE + mediaType * MEDIA_TYPE_BASE + errorSubtype * ERROR_SUBTYPE_BASE;
}

std::string ResolveRealPath(int32_t fileSourceType, const std::string& data, const std::string& storagePath)
{
    // file_source_type ∈ {FILE_MANAGER(1), MEDIA_HO_LAKE(3)} -> storage_path；其他 -> data。
    // storage_path 为空时返回空串，调用方应跳过该行（避免假阳性"文件不存在"）。
    if (fileSourceType == static_cast<int32_t>(FileSourceType::FILE_MANAGER) ||
        fileSourceType == static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE)) {
        return storagePath;
    }
    return data;
}

PhotoErrorType ClassifyPhotoError(bool fileExists, int64_t diskSize, int64_t dbSize)
{
    if (!fileExists) {
        return PhotoErrorType::FILE_NOT_EXIST;
    }
    if (diskSize == 0) {
        return PhotoErrorType::FILE_SIZE_ZERO;
    }
    if (diskSize < dbSize) {
        return PhotoErrorType::FILE_SMALLER_THAN_DB;
    }
    if (diskSize > dbSize) {
        return PhotoErrorType::FILE_LARGER_THAN_DB;
    }
    return PhotoErrorType::CONSISTENT;
}

std::vector<PhotoErrorCount> PackPhotoErrors(const std::map<int32_t, int32_t>& typeToCount, int32_t packBatchSize)
{
    // std::map 已按 key 升序；只收集非零 (type, count)，每 packBatchSize 个一事件；空映射 -> 空 vector（静默）。
    std::vector<PhotoErrorCount> batches;
    if (packBatchSize <= 0) {
        return batches;
    }
    PhotoErrorCount current;
    for (const auto& kv : typeToCount) {
        if (kv.second == 0) {
            continue;
        }
        current.photoErrorTypes.push_back(kv.first);
        current.photoErrorCounts.push_back(kv.second);
        if (static_cast<int32_t>(current.photoErrorTypes.size()) >= packBatchSize) {
            batches.push_back(std::move(current));
            current = {};
        }
    }
    if (!current.photoErrorTypes.empty()) {
        batches.push_back(std::move(current));
    }
    return batches;
}
} // namespace Media
} // namespace OHOS
