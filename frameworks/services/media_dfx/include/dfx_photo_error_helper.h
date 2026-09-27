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

#ifndef OHOS_MEDIA_DFX_PHOTO_ERROR_HELPER_H
#define OHOS_MEDIA_DFX_PHOTO_ERROR_HELPER_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "dfx_reporter.h"

namespace OHOS {
namespace Media {

enum class PhotoErrorType : int32_t {
    CONSISTENT = 0,
    FILE_NOT_EXIST = 1,
    FILE_SIZE_ZERO = 2,
    FILE_SMALLER_THAN_DB = 3,
    FILE_LARGER_THAN_DB = 4,
    OPEN_CREATE_EMPTY_FILE = 5,
};

enum PhotoErrorEncodeBase : int32_t {
    FILE_SOURCE_TYPE_BASE = 1000000,
    SOUTH_DEVICE_TYPE_BASE = 100000,
    POSITION_BASE = 10000,
    MEDIA_TYPE_BASE = 1000,
};

struct PhotoErrorDimension {
    int32_t fileSourceType = 0;
    int32_t southDeviceType = 0;
    int32_t position = 0;
    int32_t mediaType = 0;
    PhotoErrorType errorSubtype = PhotoErrorType::CONSISTENT;
};

namespace DfxPhotoErrorHelper {
int32_t NormalizeFileSourceType(int32_t fileSourceType);
int32_t NormalizeMediaType(int32_t mediaType);
int32_t NormalizeSouthDeviceType(int32_t southDeviceType);
int32_t NormalizePosition(int32_t position);
int32_t EncodePhotoErrorType(const PhotoErrorDimension& dimension);
std::string ResolveRealPath(int32_t fileSourceType, const std::string& data, const std::string& storagePath);
PhotoErrorType ClassifyPhotoError(bool fileExists, int64_t diskSize, int64_t dbSize);
std::vector<PhotoErrorCount> PackPhotoErrors(const std::map<int32_t, int32_t>& typeToCount, int32_t packBatchSize);
} // namespace DfxPhotoErrorHelper
} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_DFX_PHOTO_ERROR_HELPER_H
