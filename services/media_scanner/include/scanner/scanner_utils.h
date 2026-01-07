/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef SCANNER_UTILS_H
#define SCANNER_UTILS_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <sys/stat.h>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
enum EXPORT ErrorCodes {
    ERR_FAIL = -1,
    ERR_SUCCESS,
    ERR_EMPTY_ARGS,
    ERR_NOT_ACCESSIBLE,
    ERR_INCORRECT_PATH,
    ERR_MEM_ALLOC_FAIL,
    ERR_MIMETYPE_NOTSUPPORT,
    ERR_SCAN_NOT_INIT
};

constexpr int32_t MAX_BATCH_SIZE = 5;

constexpr int32_t UNKNOWN_ID = -1;

// Const for File Metadata defaults
const std::string FILE_PATH_DEFAULT = "";
const std::string FILE_NAME_DEFAULT = "";
const int64_t FILE_SIZE_DEFAULT = 0;
const std::string URI_DEFAULT = "";
const int64_t FILE_DATE_ADDED_DEFAULT = 0;
const int64_t FILE_DATE_MODIFIED_DEFAULT = 0;
const int32_t FILE_ID_DEFAULT = 0;
const std::string FILE_EXTENSION_DEFAULT = "";

const int32_t FILE_DURATION_DEFAULT = 0;
const std::string FILE_TITLE_DEFAULT = "";
const std::string FILE_ARTIST_DEFAULT = "";
const int32_t FILE_HEIGHT_DEFAULT = 0;
const int32_t FILE_WIDTH_DEFAULT = 0;
const double FILE_ASPECT_RATIO_DEFAULT = -2;
const int32_t FILE_ALBUM_ID_DEFAULT = 0;
const std::string FILE_ALBUM_NAME_DEFAULT = "";
const int32_t FILE_ORIENTATION_DEFAULT = 0;
const std::string FILE_SHOOTINGMODE_DEFAULT = "";
const std::string FILE_RELATIVE_PATH_DEFAULT = "";
const std::string FILE_RECYCLE_PATH_DEFAULT = "";
const int64_t FILE_DATE_TAKEN_DEFAULT = 0;
const double FILE_LONGITUDE_DEFAULT = 0;
const double FILE_LATITUDE_DEFAULT = 0;
const int64_t FILE_TIME_PENDING_DEFAULT = 0;
const std::string FILE_All_EXIF_DEFAULT = "";
const std::string FILE_USER_COMMENT_DEFAULT = "";
const int64_t FILE_LAST_VISIT_TIME_DEFAULT = 0;
const int32_t FILE_DYNAMIC_RANGE_TYPE_DEFAULT = 0;
const int32_t FILE_HDR_MODE_DEFAULT = 0;
const int32_t FILE_VIDEO_MODE_DEFAULT = 0;
const int32_t FILE_IS_TEMP_DEFAULT = 0;
const std::string FILE_FRONT_CAMERA_DEFAULT = "";
const std::string FILE_DETAIL_TIME_DEFAULT = "";
const int32_t FILE_FILE_SOURCE_TYPE_DEFAULT = 0;
const int32_t FILE_VIDEO_MODE = -1;
const int32_t COVER = 1;
const int32_t BURST_COVER_LEVEL_DEFAULT = COVER;
const int32_t STAGE_VIDEO_TASK_STATUS = 0;

const std::string DEFAULT_AUDIO_MIME_TYPE = "audio/*";
const std::string DEFAULT_VIDEO_MIME_TYPE = "video/*";
const std::string DEFAULT_IMAGE_MIME_TYPE = "image/*";
const std::string DEFAULT_FILE_MIME_TYPE = "file/*";

static std::vector<std::string> EXTRACTOR_SUPPORTED_MIME = {
    DEFAULT_AUDIO_MIME_TYPE,
    DEFAULT_VIDEO_MIME_TYPE,
    DEFAULT_IMAGE_MIME_TYPE
};

class ScannerUtils {
public:
    EXPORT ScannerUtils();
    EXPORT ~ScannerUtils();

    EXPORT static bool IsExists(const std::string &path);
    EXPORT static std::string GetFileNameFromUri(const std::string &path);
    EXPORT static std::string GetFileExtension(const std::string &path);
    EXPORT static std::string GetParentPath(const std::string &path);
    EXPORT static bool IsFileHidden(const std::string &path);
    EXPORT static bool IsDirectory(const std::string &path);
    EXPORT static bool IsRegularFile(const std::string &path);
    EXPORT static void GetRootMediaDir(std::string &dir);
    EXPORT static std::string GetFileTitle(const std::string &displayName);
    EXPORT static bool IsDirHiddenRecursive(const std::string &path, bool skipPhoto = true);
    EXPORT static bool IsDirHidden(const std::string &path, bool skipPhoto = true);
    EXPORT static bool CheckSkipScanList(const std::string &path);
};
} // namespace Media
} // namespace OHOS

#endif // SCANNER_UTILS_H
