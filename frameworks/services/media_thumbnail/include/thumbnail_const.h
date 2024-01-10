/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_CONST_H_
#define FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_CONST_H_

#include "medialibrary_db_const.h"

namespace OHOS {
namespace Media {
constexpr int32_t DEFAULT_YEAR_SIZE = 64;
constexpr int32_t DEFAULT_MTH_SIZE = 128;
constexpr int32_t DEFAULT_THUMB_SIZE = 256;
constexpr int32_t MAX_DEFAULT_THUMB_SIZE = 768;
constexpr int32_t DEFAULT_LCD_SIZE = 1080;

enum class ThumbnailType : int32_t {
    LCD,
    THUMB,
    MTH,
    YEAR,
    THUMB_ASTC,
    MTH_ASTC,
    YEAR_ASTC,
};

constexpr uint32_t DEVICE_UDID_LENGTH = 65;

constexpr int32_t THUMBNAIL_LCD_GENERATE_THRESHOLD = 5000;
constexpr int32_t THUMBNAIL_LCD_AGING_THRESHOLD = 10000;
constexpr int32_t WAIT_FOR_MS = 1000;
constexpr int32_t WAIT_FOR_SECOND = 3;

const std::string THUMBNAIL_LCD_SUFFIX = "LCD";     // The size fit to screen
const std::string THUMBNAIL_THUMB_SUFFIX = "THM";   // The size which height is 256 and width is 256
const std::string THUMBNAIL_THUMBASTC_SUFFIX = "THM_ASTC";
const std::string THUMBNAIL_MTH_SUFFIX = "MTH";     // The size which height is 128 and width is 128
const std::string THUMBNAIL_YEAR_SUFFIX = "YEAR";   // The size which height is 64 and width is 64

const std::string FILE_URI_PREX = "file://";

const std::string PHOTO_URI_PREFIX = "file://media/Photo/";

const std::string THUMBNAIL_FORMAT = "image/jpeg";
const std::string THUMBASTC_FORMAT = "image/astc/4*4";
constexpr uint8_t THUMBNAIL_MID = 90;
constexpr uint8_t THUMBNAIL_HIGH = 100;
constexpr uint8_t ASTC_LOW_QUALITY = 20;

constexpr uint32_t THUMBNAIL_QUERY_MAX = 2000;
constexpr int64_t AV_FRAME_TIME = 0;

constexpr uint8_t NUMBER_HINT_1 = 1;

constexpr int32_t DEFAULT_ORIGINAL = -1;

const std::string THUMBNAIL_OPERN_KEYWORD = "operation";
const std::string THUMBNAIL_OPER = "oper";
const std::string THUMBNAIL_HEIGHT = "height";
const std::string THUMBNAIL_WIDTH = "width";
const std::string THUMBNAIL_PATH = "path";

// create thumbnail in close operation
const std::string CLOSE_CREATE_THUMB_STATUS = "create_thumbnail_sync_status";
const int32_t CREATE_THUMB_SYNC_STATUS = 1;
const int32_t CREATE_THUMB_ASYNC_STATUS = 0;

constexpr float FLOAT_EPSILON = 1e-6;

// request photo type
const std::string REQUEST_PHOTO_TYPE = "requestPhotoType";

static inline std::string GetThumbnailPath(const std::string &path, const std::string &key)
{
    if (path.length() < ROOT_MEDIA_DIR.length()) {
        return "";
    }
    std::string suffix = (key == "THM_ASTC") ? ".astc" : ".jpg";
    return ROOT_MEDIA_DIR + ".thumbs/" + path.substr(ROOT_MEDIA_DIR.length()) + "/" + key + suffix;
}

static std::string GetThumbSuffix(ThumbnailType type)
{
    switch (type) {
        case ThumbnailType::MTH:
            return THUMBNAIL_MTH_SUFFIX;
        case ThumbnailType::YEAR:
            return THUMBNAIL_YEAR_SUFFIX;
        case ThumbnailType::THUMB:
            return THUMBNAIL_THUMB_SUFFIX;
        case ThumbnailType::THUMB_ASTC:
            return THUMBNAIL_THUMBASTC_SUFFIX;
        case ThumbnailType::LCD:
            return THUMBNAIL_LCD_SUFFIX;
        default:
            return "";
    }
}

static inline ThumbnailType GetThumbType(const int32_t width, const int32_t height, bool isAstc = false)
{
    if (width == DEFAULT_ORIGINAL && height == DEFAULT_ORIGINAL) {
        return ThumbnailType::LCD;
    }

    if (width == DEFAULT_MTH_SIZE && height == DEFAULT_MTH_SIZE) {
        return ThumbnailType::MTH;
    }

    if (width == DEFAULT_YEAR_SIZE && height == DEFAULT_YEAR_SIZE) {
        return ThumbnailType::YEAR;
    }

    if (std::min(width, height) <= DEFAULT_THUMB_SIZE &&
        std::max(width, height) <= MAX_DEFAULT_THUMB_SIZE) {
        return isAstc ? ThumbnailType::THUMB_ASTC : ThumbnailType::THUMB;
    }

    return ThumbnailType::LCD;
}

static inline std::string GetSandboxPath(const std::string &path, ThumbnailType type)
{
    if (path.length() < ROOT_MEDIA_DIR.length()) {
        return "";
    }
    std::string suffix = (type == ThumbnailType::THUMB_ASTC) ? ".astc" : ".jpg";
    std::string suffixStr = path.substr(ROOT_MEDIA_DIR.length()) + "/" + GetThumbSuffix(type) + suffix;
    return ROOT_SANDBOX_DIR + ".thumbs/" + suffixStr;
}

static inline bool IsThumbnail(const int32_t width, const int32_t height)
{
    if (width == DEFAULT_ORIGINAL && height == DEFAULT_ORIGINAL) {
        return false;
    }
    return std::min(width, height) <= DEFAULT_THUMB_SIZE &&
           std::max(width, height) <= MAX_DEFAULT_THUMB_SIZE;
}

} // namespace Media
} // namespace OHOS

#endif  // FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_CONST_H_
