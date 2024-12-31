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

#include <unordered_map>

namespace OHOS {
namespace Media {
constexpr int32_t THUMBNAIL_VERSION = 0;
constexpr int32_t DEFAULT_YEAR_SIZE = 64;
constexpr int32_t DEFAULT_MTH_SIZE = 128;
constexpr int32_t DEFAULT_THUMB_SIZE = 256;
constexpr int32_t MAX_DEFAULT_THUMB_SIZE = 768;
constexpr int32_t DEFAULT_LCD_SIZE = 1080;
constexpr uint32_t INT32_MAX_VALUE_LENGTH = 10;
constexpr int32_t VERTICAL_ANGLE = 90;
constexpr int32_t STRAIGHT_ANGLE = 180;
constexpr int32_t STAMP_PARAM = 4;
constexpr int32_t STAMP_PARAM_ZERO = 0;
constexpr int32_t STAMP_PARAM_ONE = 1;
constexpr int32_t STAMP_PARAM_TWO = 2;
constexpr int32_t STAMP_PARAM_THREE = 3;
enum class ThumbnailType : int32_t {
    LCD,
    THUMB,
    MTH,
    YEAR,
    THUMB_ASTC,
    MTH_ASTC,
    YEAR_ASTC,
    LCD_EX,
    THUMB_EX,
};

enum class GenerateScene : int32_t {
    LOCAL = 0,
    CLOUD,
    BACKGROUND,
    FOREGROUND,
    RESTORE,
    UPGRADE,
};

enum class LoadSourceType : int32_t {
    LOCAL_PHOTO = 0,
    CLOUD_THUMB,
    CLOUD_LCD,
    CLOUD_PHOTO,
};

enum class ThumbnailReady : int32_t {
    GENERATE_THUMB_LATER,
    GENERATE_THUMB_NOW,
    GENERATE_THUMB_RETRY,
    GENERATE_THUMB_COMPLETED,
    THUMB_TO_UPLOAD,
    THUMB_UPLOAD_COMPLETED,
    THUMB_UPGRADE,
};

enum class LcdReady : int64_t {
    GENERATE_LCD_LATER,
    GENERATE_LCD_FAILED,
    GENERATE_LCD_COMPLETED,
};

const std::unordered_map<ThumbnailType, std::string> TYPE_NAME_MAP = {
    { ThumbnailType::LCD, "LCD" },
    { ThumbnailType::THUMB, "THUMB" },
    { ThumbnailType::MTH, "MTH" },
    { ThumbnailType::YEAR, "YEAR" },
    { ThumbnailType::THUMB_ASTC, "THUMB_ASTC" },
    { ThumbnailType::MTH_ASTC, "MTH_ASTC" },
    { ThumbnailType::YEAR_ASTC, "YEAR_ASTC" },
};

constexpr uint32_t DEVICE_UDID_LENGTH = 65;

constexpr int32_t THUMBNAIL_LCD_GENERATE_THRESHOLD = 5000;
constexpr int32_t THUMBNAIL_LCD_AGING_THRESHOLD = 10000;
constexpr int32_t WAIT_FOR_MS = 1000;
constexpr int32_t WAIT_FOR_SECOND = 3;

constexpr float EPSILON = 1e-6;
constexpr int32_t SHORT_SIDE_THRESHOLD = 350;
constexpr int32_t MAXIMUM_SHORT_SIDE_THRESHOLD = 1050;
constexpr int32_t LCD_SHORT_SIDE_THRESHOLD = 512;
constexpr int32_t LCD_LONG_SIDE_THRESHOLD = 1920;
constexpr int32_t MAXIMUM_LCD_LONG_SIDE = 4096;
constexpr int32_t ASPECT_RATIO_THRESHOLD = 3;
constexpr int32_t MIN_COMPRESS_BUF_SIZE = 8192;
constexpr int32_t DECODE_SCALE_BASE = 2;
constexpr int32_t FLAT_ANGLE = 180;
constexpr int32_t THUMBNAIL_GENERATE_BATCH_COUNT = 200;
constexpr int32_t ASTC_GENERATE_COUNT_AFTER_RESTORE = 500;
constexpr int32_t READY_TEMPERATURE_LEVEL = 4;
constexpr int32_t EVEN_BASE_NUMBER = 2;
constexpr int32_t LOCAL_GENERATION_BATTERY_CAPACITY = 10;
const std::string DEFAULT_EXIF_ORIENTATION = "1";

const std::string THUMBNAIL_LCD_SUFFIX = "LCD";     // The size fit to screen
const std::string THUMBNAIL_THUMB_SUFFIX = "THM";   // The size which height is 256 and width is 256
const std::string THUMBNAIL_THUMBASTC_SUFFIX = "THM_ASTC";
const std::string THUMBNAIL_MTH_SUFFIX = "MTH";     // The size which height is 128 and width is 128
const std::string THUMBNAIL_YEAR_SUFFIX = "YEAR";   // The size which height is 64 and width is 64
const std::string THUMBNAIL_LCD_EX_SUFFIX = "THM_EX/LCD";
const std::string THUMBNAIL_THUMB_EX_SUFFIX = "THM_EX/THM";

const std::string FILE_URI_PREX = "file://";

const std::string PHOTO_URI_PREFIX = "file://media/Photo/";

const std::string THUMBNAIL_FORMAT = "image/jpeg";
const std::string THUMBASTC_FORMAT = "image/astc/4*4";
constexpr uint8_t THUMBNAIL_MID = 90;
constexpr uint8_t THUMBNAIL_HIGH = 100;
constexpr uint8_t ASTC_LOW_QUALITY = 20;

constexpr uint32_t THUMBNAIL_QUERY_MAX = 2000;
constexpr uint32_t THUMBNAIL_QUERY_MIN = 200;
constexpr int64_t AV_FRAME_TIME = 0;
constexpr int64_t MS_TRANSFER_US = 1000;

constexpr uint8_t NUMBER_HINT_1 = 1;

constexpr int32_t DEFAULT_ORIGINAL = -1;

const std::string DYNAMIC_RANGE = "decodeDynamicRange";

const std::string THUMBNAIL_OPERN_KEYWORD = "operation";
const std::string THUMBNAIL_OPER = "oper";
const std::string THUMBNAIL_HEIGHT = "height";
const std::string THUMBNAIL_WIDTH = "width";
const std::string THUMBNAIL_PATH = "path";
const std::string THUMBNAIL_BEGIN_STAMP = "begin_stamp";
const std::string THUMBNAIL_TYPE = "type";

// create thumbnail in close operation
const std::string CLOSE_CREATE_THUMB_STATUS = "create_thumbnail_sync_status";
const int32_t CREATE_THUMB_SYNC_STATUS = 1;
const int32_t CREATE_THUMB_ASYNC_STATUS = 0;

constexpr float FLOAT_EPSILON = 1e-6;

// request photo type
const std::string REQUEST_PHOTO_TYPE = "requestPhotoType";

const int32_t CLOUD_PHOTO_POSITION = 2;
const int32_t CLOUD_THUMB_STATUS_DOWNLOAD = 0;
const int32_t CLOUD_THUMBNAIL_DOWNLOAD_FINISH_NUMBER = 1000;
const std::string RDB_QUERY_COUNT = "count";

const int32_t THUMBNAIL_READY_FAILED = 2;

// LCD that is over 2MB would not be uploaded
const size_t LCD_UPLOAD_LIMIT_SIZE = 2048000;

// Only check the latest 3000 data to avoid opreation taking too long time
const uint32_t MAXIMUM_LCD_CHECK_NUM = 3000;

static inline std::string GetThumbnailPath(const std::string &path, const std::string &key)
{
    if (path.length() < ROOT_MEDIA_DIR.length()) {
        return "";
    }
    std::string suffix = (key == "THM_ASTC") ? ".astc" : ".jpg";
    return ROOT_MEDIA_DIR + ".thumbs/" + path.substr(ROOT_MEDIA_DIR.length()) + "/" + key + suffix;
}

static inline std::string GetThumbnailPathHighlight(const std::string &path, const std::string &key,
    const std::string &timeStamp)
{
    if (path.length() < ROOT_MEDIA_DIR.length()) {
        return "";
    }
    std::string suffix = (key == "THM_ASTC") ? ".astc" : ".jpg";
    return ROOT_MEDIA_DIR + ".thumbs/" + path.substr(ROOT_MEDIA_DIR.length()) +
        "/beginTimeStamp" + timeStamp + "/" + key + suffix;
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

static std::string GetKeyFrameThumbSuffix(int32_t type)
{
    switch (type) {
        case KEY_FRAME_LCD:
            return THUMBNAIL_LCD_SUFFIX;
        case KEY_FRAME_THM:
            return THUMBNAIL_THUMB_SUFFIX;
        case KEY_FRAME_THM_ASTC:
            return THUMBNAIL_THUMBASTC_SUFFIX;
        default:
            return "";
    }
}

static inline ThumbnailType GetThumbType(const int32_t width, const int32_t height, bool isAstc = false)
{
    if (width == DEFAULT_ORIGINAL && height == DEFAULT_ORIGINAL) {
        return ThumbnailType::LCD;
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

static inline std::string GetKeyFrameSandboxPath(const std::string &path, const int32_t &beginStamp,
    const int32_t &type)
{
    if (path.length() < ROOT_MEDIA_DIR.length()) {
        return "";
    }
    std::string suffix = (type == KEY_FRAME_THM_ASTC) ? ".astc" : ".jpg";
    std::string suffixStr = path.substr(ROOT_MEDIA_DIR.length()) + "/beginTimeStamp" + std::to_string(beginStamp) +
        "/" + GetKeyFrameThumbSuffix(type) + suffix;
    return ROOT_MEDIA_DIR + ".thumbs/" + suffixStr;
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
