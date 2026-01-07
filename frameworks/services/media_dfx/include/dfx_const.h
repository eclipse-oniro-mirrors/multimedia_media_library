/*
 * Copyright (C) 2024-2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_DFX_CONST_H
#define OHOS_MEDIA_DFX_CONST_H

#include <string>
#include <unordered_set>
#include <unordered_map>

#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
constexpr int32_t IMAGE_MIN = 4096;
constexpr int32_t VIDEO_8K_MIN = 7680;

constexpr int32_t COMMON_TIME_OUT = 200;
constexpr int32_t OPEN_FILE_TIME_OUT = 500;
constexpr int32_t CLOUD_DEFAULT_TIME_OUT = 100;
constexpr int32_t CLOUD_LCD_TIME_OUT = 800;
constexpr int32_t RDB_TIME_OUT = 100;
constexpr int32_t TO_MILLION = 1000;
constexpr int32_t ONE_MINUTE = 60;
constexpr int32_t ONE_HOUR = 60 * 60;
constexpr int32_t FIVE_MINUTE = 5 * 60 * 1000;
constexpr int32_t SIX_HOUR = 6 * 60 * 60;
constexpr int32_t HALF_DAY = 12 * 60 * 60;
constexpr int32_t ONE_DAY = 24 * 60 * 60;
constexpr int32_t TWO_DAY = 48 * 60 * 60;
constexpr int32_t ONE_WEEK = 7 * 24 * 60 * 60;
constexpr int64_t THIRTY_DAYS = 30 * 24 * 60 * 60;

constexpr int32_t NOT_INIT = -1;
constexpr int32_t COMMON_IMAGE = 0;
constexpr int32_t COMMON_VIDEO = 1;
constexpr int32_t OTHER_FORMAT_IMAGE = 2;
constexpr int32_t BIG_IMAGE = 3;
constexpr int32_t BIG_VIDEO = 4;

constexpr int32_t INVALID_DFX = -1;

constexpr uint32_t GARBLE_SMALL = 3;
constexpr uint32_t GARBLE_LARGE = 8;
constexpr uint32_t GARBLE_LAST_TWO = 2;
constexpr uint32_t GARBLE_LAST_ONE = 1;

constexpr int32_t LATEST_THUMBNAIL_ERROR_VERSION = 1;
constexpr int32_t LATEST_DELETE_STATISTIC_VERSION = 1;

constexpr int32_t DIRTY_PHOTO_COUNT = 10;

/**
 * the number from 0 ~ 1000 is reserved for operationtype
 */
enum DfxType {
    ALBUM_REMOVE_PHOTOS = 17,
    DELETE_LOCAL_ASSETS_PERMANENTLY = 18,
    ALBUM_DELETE_ASSETS = 19,
    TRASH_PHOTO = 20,
    CLOUD_DEFAULT_OPEN = 1000,
    CLOUD_LCD_OPEN,
    RDB_INSERT = 1100,
    RDB_DELETE,
    RDB_UPDATE,
    RDB_UPDATE_BY_CMD,
    RDB_QUERY,
    RDB_EXECUTE_SQL,
    RDB_BATCHINSERT,
    IMAGE_SOURCE_CREATE = 1200,
    IMAGE_SOURCE_GET_INFO,
    IMAGE_SOURCE_CREATE_PIXELMAP,
    AV_SET_SOURCE = 1300,
    AV_FETCH_FRAME,
    START_SUCCESS = 1400,
    START_CONTEXT_FAIL,
    START_DATAMANAGER_FAIL,
    START_RDB_STORE_FAIL,
    START_DISTRIBUTE_FAIL,
    START_DIR_SET_FAIL,
    START_THUMBNAIL_SERVICE_FAIL,
    START_SCANNER_FAIL,
    CHECK_USER_UNLOCK_FAIL,
    ADD_DATA_UNIQUE_INDEX_FAIL,
    CHECK_MEDIA_PATH_UNLOCK_FAIL,
    START_MOUNT_FUSE_FAIL,
    START_FUSE_DAEMON_FAIL,
    STOP_WITH_FUSE_TIMEOUT,
    FUSE_OPEN = 1500,
    FUSE_RELEASE,
    FUSE_WRITE,
    FUSE_READ,
    FUSE_CREATE,
    FUSE_UNLINK,
    FUSE_OPENDIR,
    FUSE_READDIR,
    FUSE_RELEASEDIR
};

const std::string NULL_STRING = "";
const std::string SPLIT_CHAR = "|";

const std::string THUMBNAIL_ERROR_XML = "/data/storage/el2/base/preferences/thumbnail_error.xml";
const std::string COMMON_BEHAVIOR_XML = "/data/storage/el2/base/preferences/common_behavior.xml";
const std::string DELETE_BEHAVIOR_XML = "/data/storage/el2/base/preferences/delete_behavior.xml";
const std::string ADAPTATION_TO_MOVING_PHOTO_XML = "/data/storage/el2/base/preferences/adaptation_to_moving_photo.xml";
const std::string DFX_CINEMATIC_VIDEO_XML = "/data/storage/el2/base/preferences/dfx_cinematic_video.xml";
const std::string ALIB_HEIF_DUPLICATE_XML = "/data/storage/el2/base/preferences/alib_heif_duplicate.xml";
const std::string DFX_COMMON_XML = "/data/storage/el2/base/preferences/dfx_common.xml";
const std::string LAST_REPORT_TIME = "last_report_time";
const std::string LAST_MIDDLE_REPORT_TIME = "last_middle_report_time";
const std::string LAST_HALF_DAY_REPORT_TIME = "last_half_day_report_time";
const std::string LAST_TWO_DAY_REPORT_TIME = "last_two_day_report_time";
const std::string LAST_WEEK_REPORT_TIME = "last_week_report_time";
const std::string THUMBNAIL_ERROR_VERSION = "thumbnail_error_version";
const std::string DELETE_STATISTIC_VERSION = "delete_statistic_version";
const std::string MOVING_PHOTO_KEY_UNADAPTED_NUM = "unadapted_app_num";
const std::string MOVING_PHOTO_KEY_UNADAPTED_PACKAGE = "unadapted_app_package";
const std::string MOVING_PHOTO_KEY_ADAPTED_NUM = "adapted_app_num";
const std::string MOVING_PHOTO_KEY_ADAPTED_PACKAGE = "adapted_app_package";
const std::string CINEMATIC_VIDEO_KEY_LOW_QUALITY_ACCESS_TIMES = "low_quality_access_times";
const std::string CINEMATIC_VIDEO_KEY_HIGH_QUALITY_ACCESS_TIMES = "high_quality_access_times";
const std::string CINEMATIC_VIDEO_KEY_LOW_QUALITY_URI_ACCESS_TIMES = "low_quality_uri_access_times";
const std::string CINEMATIC_VIDEO_KEY_HIGH_QUALITY_URI_ACCESS_TIMES = "high_quality_uri_access_times";
const std::string CINEMATIC_VIDEO_KEY_CANCEL_NUM = "cancel_num";
const std::string CINEMATIC_VIDEO_KEY_CANCEL_WAIT_AVG_TIME = "cancel_wait_avg_time";
const std::string CINEMATIC_VIDEO_PROCESS_AVG_TIME = "process_avg_time";
const std::string CINEMATIC_VIDEO_KEY_MULTISTAGE_SUCCESS_TIMES = "multistage_success_times";
const std::string CINEMATIC_VIDEO_KEY_MULTISTAGE_FAILED_TIMES = "multistage_failed_times";
const std::string CLOUD_SYNC_START_TIME = "cloud_sync_start_time";
const std::string CLOUDSYNC_STATUS_KEY = "persist.kernel.cloudsync.status";
const std::string IS_REPORTED = "is_reported";
const std::string LAST_CLEAR_DISK_DIRTY_DATA_TIME = "last_clear_disk_dirty_data_time";

const std::string CLOUD_PHOTO_PATH = "/storage/cloud/files/Photo/";
const std::string CLOUD_FILE_PATH = "/storage/cloud/files/";
const std::string GARBLE = "*";
const std::string SPLIT_PATH = "/";
const std::string DOT = ".";

const std::string TRANSCODE_ACCESS_TIMES = "transcode_access_times";
const std::string TRANSCODE_ACCESS_MEDIALIB = "transcode_access_medialib";
const std::string TRANSCODE_ACCESS_LIBC = "transcode_access_libc";
const std::string TRANSCODE_AVG_TIME = "transcode_avg_time";
const std::string TRANSCODE_TIMES = "transcode_times";
const std::string TRANSCODE_FAILED_TIMES = "transcode_failed_times";
const std::string INNER_FAILED_TIMES = "inner_failed_times";
const std::string CODEC_FAILED_TIMES = "codec_failed_times";

struct ThumbnailErrorInfo {
    int32_t method;
    int32_t errCode;
    int64_t time;
};

struct CommonBehavior {
    int32_t times;
};

struct AlbumInfo {
    int32_t count = 0;
    int32_t imageCount = 0;
    int32_t videoCount = 0;
    bool isLocal = false;
};

struct PhotoInfo {
    std::string data;
    int32_t dirty;
    int32_t cloudVersion;
};

struct PhotoRecordInfo {
    int32_t imageCount;
    int32_t videoCount;
    int32_t abnormalSizeCount;
    int32_t abnormalWidthOrHeightCount;
    int32_t abnormalVideoDurationCount;
    int32_t toBeUpdatedRecordCount;
    int64_t dbFileSize;
    int64_t slaveDbFileSize;
    int32_t duplicateLpathCount;
    int32_t abnormalLpathCount;
    int32_t photoWaitUploadCloudCount;
    int32_t photoWaitUploadHdcCount;
};

struct AdaptationToMovingPhotoInfo {
    std::unordered_set<std::string> unadaptedAppPackages;
    std::unordered_set<std::string> adaptedAppPackages;
};

enum class CinematicWaitType : int32_t {
    CANCEL_CINEMATIC = 0,
    PROCESS_CINEMATIC,
};

struct TimeRange {
    uint64_t startTime {0};
    uint64_t endTime {0};
};

struct CinematicVideoInfo {
    int32_t accessTimesLow;
    int32_t accessTimesHigh;
    int32_t uriAccessTimesLow;
    int32_t uriAccessTimesHigh;
    int32_t multistageSuccessTimes;
    int32_t multistageFailedTimes;
    std::unordered_map<std::string, TimeRange> cancelWaitTimeMap;
    std::unordered_map<std::string, TimeRange> processWaitTimeMap;

    CinematicVideoInfo()
        : accessTimesLow(0),
          accessTimesHigh(0),
          uriAccessTimesLow(0),
          uriAccessTimesHigh(0),
          multistageSuccessTimes(0),
          multistageFailedTimes(0) {}
};

const std::unordered_map<int32_t, std::string> ALBUM_MAP = {
    { static_cast<int32_t>(PhotoAlbumSubType::IMAGE), "IMAGE" },
    { static_cast<int32_t>(PhotoAlbumSubType::VIDEO), "VIDEO" },
    { static_cast<int32_t>(PhotoAlbumSubType::FAVORITE), "FAVORITE" },
    { static_cast<int32_t>(PhotoAlbumSubType::HIDDEN), "HIDDEN" },
    { static_cast<int32_t>(PhotoAlbumSubType::TRASH), "TRASH" },
};

struct OperationRecordInfo {
    int32_t addTotalCount;
    int32_t delTotalCount;
    int32_t updateTotalCount;
    int32_t totalCount;
};

enum TranscodeAccessType {
    ACCESS_MEDIALIB = 0,
    ACCESS_LIBC,
};

enum TranscodeErrorType {
    INNER_FAILED = 0,
    CODEC_FAILED,
};
} // namespace Media
} // namespace OHOS

#endif  // OHOS_MEDIA_DFX_CONST_H