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

#define MLOG_TAG "DfxPhotoError"

#include "dfx_photo_error_helper.h"

#include "media_column.h"
#include "media_log.h"
#include "medialibrary_db_const.h"

namespace OHOS {
namespace Media {
namespace DfxPhotoErrorHelper {

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
    return (southDeviceType == static_cast<int32_t>(SouthDeviceType::SOUTH_DEVICE_VISIT))
        ? static_cast<int32_t>(SouthDeviceType::SOUTH_DEVICE_NULL) : southDeviceType;
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
        + position * POSITION_BASE + mediaType * MEDIA_TYPE_BASE + errorSubtype;
}

std::string ResolveRealPath(int32_t fileSourceType, const std::string& data, const std::string& storagePath)
{
    return (fileSourceType == static_cast<int32_t>(FileSourceType::FILE_MANAGER) ||
        fileSourceType == static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE)) ? storagePath : data;
}

PhotoErrorType ClassifyPhotoError(bool fileExists, bool thumbExists, int64_t diskSize, int64_t dbSize)
{
    if (!fileExists) {
        return thumbExists ? PhotoErrorType::FILE_NOT_EXIST_THUMB_EXIST
                           : PhotoErrorType::FILE_NOT_EXIST_THUMB_NOT_EXIST;
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
    std::vector<PhotoErrorCount> batches;
    CHECK_AND_RETURN_RET(packBatchSize > 0, batches);

    PhotoErrorCount current;
    for (const auto& kv : typeToCount) {
        CHECK_AND_CONTINUE(kv.second != 0);
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
} // namespace DfxPhotoErrorHelper
} // namespace Media
} // namespace OHOS
