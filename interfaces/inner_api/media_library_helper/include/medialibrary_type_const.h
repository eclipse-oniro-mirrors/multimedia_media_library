/*
 * Copyright (C) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_LIB_SERVICE_CONST_H_
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_LIB_SERVICE_CONST_H_

#include <unordered_set>

#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
enum {
    MEDIA_GET_MEDIA_ASSETS = 0,
    MEDIA_GET_IMAGE_ASSETS = 1,
    MEDIA_GET_AUDIO_ASSETS = 2,
    MEDIA_GET_VIDEO_ASSETS = 3,
    MEDIA_GET_IMAGEALBUM_ASSETS = 4,
    MEDIA_GET_VIDEOALBUM_ASSETS = 5,
    MEDIA_CREATE_MEDIA_ASSET = 6,
    MEDIA_DELETE_MEDIA_ASSET = 7,
    MEDIA_MODIFY_MEDIA_ASSET = 8,
    MEDIA_COPY_MEDIA_ASSET   = 9,
    MEDIA_CREATE_MEDIA_ALBUM_ASSET  = 10,
    MEDIA_DELETE_MEDIA_ALBUM_ASSET  = 11,
    MEDIA_MODIFY_MEDIA_ALBUM_ASSET  = 12,
};

/* ENUM Asset types */
enum AssetType {
    ASSET_MEDIA = 0,
    ASSET_IMAGE,
    ASSET_AUDIO,
    ASSET_VIDEO,
    ASSET_GENERIC_ALBUM,
    ASSET_IMAGEALBUM,
    ASSET_VIDEOALBUM,
    ASSET_NONE
};

enum DirType {
    DIR_CAMERA = 0,
    DIR_VIDEO,
    DIR_IMAGE,
    DIR_AUDIOS,
    DIR_DOCUMENTS,
    DIR_DOWNLOAD,
    DIR_TYPE_MAX
};

enum class CloudSyncErrType : int32_t {
    OTHER_ERROR = 0,
    CONTENT_NOT_FOUND,
    THM_NOT_FOUND,
    LCD_NOT_FOUND,
    LCD_SIZE_IS_TOO_LARGE,
    CONTENT_SIZE_IS_ZERO,
    ALBUM_NOT_FOUND,
    THM_GENERATE_FAILED,
};

enum PrivateAlbumType {
    TYPE_FAVORITE = 0,
    TYPE_TRASH,
    TYPE_HIDE,
    TYPE_SMART,
    TYPE_SEARCH
};

enum class DataType : int32_t {
    TYPE_NULL = 0,
    TYPE_INT,
    TYPE_LONG,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_BOOL
};

enum class FetchResType : int32_t {
    TYPE_FILE = 1,
    TYPE_ALBUM,
    TYPE_SMARTALBUM,
    TYPE_PHOTOALBUM,
    TYPE_CUSTOMRECORD,
    TYPE_ALBUMORDER,
};

enum class DirtyType : int32_t {
    TYPE_SYNCED,
    TYPE_NEW,
    TYPE_MDIRTY,
    TYPE_FDIRTY,
    TYPE_DELETED,
    TYPE_RETRY,
    TYPE_SDIRTY,
    TYPE_COPY,
    TYPE_TDIRTY
};

enum class CompositeDisplayMode : int32_t {
    DEFAULT = 0,
    CLOUD_ENHANCEMENT,
};

enum class CloudEnhancementAvailableType : int32_t {
    NOT_SUPPORT = 0,
    SUPPORT,
    PROCESSING_MANUAL,
    FAILED_RETRY,
    FAILED,
    SUCCESS,
    EDIT,
    TRASH,
    PROCESSING_AUTO,
    FINISH = 120,
};

enum class CompositeDisplayStatus : int32_t {
    PLAIN_PICTURE = 0,
    ORIGINAL,
    ENHANCED,
    ORIGINAL_EDIT,
    ENHANCED_EDIT,
};

enum class CloudEnhancementIsAutoType : int32_t {
    NOT_AUTO = 0,
    AUTO,
};

enum class CloudEnhancementTriggerModeType : int32_t {
    TRIGGER_MANUAL = 0,
    TRIGGER_AUTO,
};

enum class CEErrorCodeType : int32_t {
    // failed retry
    LIMIT_USAGE = 100,
    LIMIT_REQUEST,
    TASK_CACHE_TIMEOUT,
    NETWORK_UNAVAILABLE,
    TEMPERATURES_GUARD,
    NETWORK_WEAK,
    // failed
    EXECUTE_FAILED,
    // failed retry
    DO_AUTH_FAILED,
    TASK_CANNOT_EXECUTE,
    // failed
    NON_RECOVERABLE = 200,
};

enum class StrongAssociationType : int32_t {
    NORMAL = 0,
    CLOUD_ENHANCEMENT
};

enum class SyncStatusType : int32_t {
    TYPE_BACKUP = -1, // Only for clone! Set backup data invisible before moving
    TYPE_VISIBLE = 0,
    TYPE_DOWNLOAD,
    TYPE_UPLOAD,
};

enum class ThumbStatus : int32_t {
    DOWNLOADED,
    TO_DOWNLOAD,
};

enum class TableType : int32_t {
    TYPE_FILES = 0,
    TYPE_PHOTOS,
    TYPE_AUDIOS,
};

enum ResultSetDataType {
    TYPE_NULL = 0,
    TYPE_STRING,
    TYPE_INT32,
    TYPE_INT64,
    TYPE_DOUBLE,
    TYPE_BLOB
};

enum class CleanType : int32_t {
    TYPE_NOT_CLEAN = 0,
    TYPE_NEED_CLEAN
};

enum class CloudType : int32_t {
    CLOUD_TYPE_CLOSE = 0,
    CLOUD_TYPE_CLOUD,
    CLOUD_TYPE_HDC
};

enum class MultiStagesPhotoQuality : int32_t {
    FULL = 0,
    LOW,
};

enum class StageVideoTaskStatus : int32_t {
    NO_NEED_TO_STAGE = 0,
    NEED_TO_STAGE,
    STAGE_TASK_TO_DELIVER,
    STAGE_TASK_DELIVERED,
    STAGE_TASK_FAIL,
    STAGE_TASK_SUCCESS,
};

enum class FirstStageModifyType : int32_t {
    NOT_MODIFIED = 0,
    EDITED,
    TRASHED,
};

enum class BatchDownloadStatusType : int32_t {
    TYPE_WAITING = 0,
    TYPE_DOWNLOADING,
    TYPE_PAUSE,
    TYPE_FAIL,
    TYPE_SUCCESS,
    TYPE_AUTO_PAUSE
};

enum class BatchDownloadAutoPauseReasonType : int32_t {
    TYPE_DEFAULT = 0,
    TYPE_NETWORK_DISCONNECT,
    TYPE_CELLNET_LIMIT,
    TYPE_POWER_LOW,
    TYPE_ROM_LOW
};

struct ColumnSchema {
    bool defaultNull;
    std::string defaultValue;
    std::string columnType;
};

constexpr int32_t DEFAULT_INT32 = 0;
constexpr int64_t DEFAULT_INT64 = 0;
constexpr int64_t DEFAULT_DOUBLE = 0;
const std::string DEFAULT_STR = "";
const bool DEFAULT_BOOL = false;
const std::string DEFAULT_MEDIA_PATH = "";

const int32_t DEFAULT_ALBUM_ID = 0;
const std::string DEFAULT_ALBUM_NAME = "Unknown";
const std::string DEFAULT_ALBUM_PATH = "";
const std::string DEFAULT_ALBUM_URI = "";
const std::string DEFAULT_SMART_ALBUM_TAG = "";
const PrivateAlbumType DEFAULT_SMART_ALBUM_PRIVATE_TYPE = TYPE_SMART;
const int32_t DEFAULT_SMART_ALBUM_ALBUMCAPACITY = 0;
const int32_t DEFAULT_SMART_ALBUM_CATEGORYID = 0;
const int64_t DEFAULT_SMART_ALBUM_DATE_MODIFIED = 0;
const std::string DEFAULT_SMART_ALBUM_CATEGORYNAME = "";
const int64_t DEFAULT_ALBUM_DATE_MODIFIED = 0;
const int32_t DEFAULT_COUNT = 0;
const std::string DEFAULT_ALBUM_RELATIVE_PATH = "";
const std::string DEFAULT_COVERURI = "";
const int32_t DEFAULT_MEDIA_PARENT = 0;
const std::string DEFAULT_DESCRIPTION;
constexpr int32_t DEFAULT_EXPIREDTIME = 0;
const bool DEFAULT_ALBUM_VIRTUAL = false;
const uint64_t DEFAULT_MEDIA_DATE_TAKEN = 0;
const std::string DEFAULT_MEDIA_ALBUM_URI = "";
const bool DEFAULT_MEDIA_IS_PENDING = false;
const int32_t DEFAULT_DIR_TYPE = -1;
const std::string DEFAULT_DIRECTORY = "";
const std::string DEFAULT_STRING_MEDIA_TYPE = "";
const std::string DEFAULT_EXTENSION = "";
const int32_t DEFAULT_MEDIAVOLUME = 0;
const int32_t DEFAULT_ALBUMS_ORDER = -1;
const int32_t DEFAULT_ORDER_SECTION = -1;
const int32_t DEFAULT_ORDER_TYPE = -1;
const int32_t DEFAULT_ORDER_STATUS = 0;
const std::string ROOT_MEDIA_DIR = "/storage/cloud/files/";
const std::string ROOT_SANDBOX_DIR = "/storage/Share/";
const std::string FS_TYPE_EPFS = "epfs";
const std::string EPFS_MOUNT_POINT = "/storage/cloud/epfs";
const std::string MEDIA_CACHE_DIR = ROOT_MEDIA_DIR + ".cache/";
const std::string MEDIA_HIGHLIGHT_DIR = ROOT_MEDIA_DIR + "highlight/";
const std::string MEDIA_EDIT_DATA_DIR = ROOT_MEDIA_DIR + ".editData/";
const std::string META_RECOVERY_PHOTO_RELATIVE_PATH = "/Photo/";
const std::string META_RECOVERY_META_RELATIVE_PATH = "/.meta/Photo/";
const std::string META_RECOVERY_META_FILE_SUFFIX = ".json";
const std::string ASTC_JSON_FILE_PATH = "/data/storage/el2/base/preferences/astcphase.json";
const char SLASH_CHAR = '/';
const std::string SLASH_STR = "/";
const int32_t KEY_FRAME_LCD = 1;
const int32_t KEY_FRAME_THM = 2;
const int32_t KEY_FRAME_THM_ASTC = 3;
const std::string HIGHLIGHT_CONFIG = "/data/storage/el2/base/preferences/highlight.xml";
const std::string HIGHLIGHT_DIRECTORY_SIZE = "highlight_directory_size";

const std::string SKIPLIST_FILE_PATH = "/data/SkipScanFile.txt";

const std::string DOCS_PATH = "Docs/";
const int CAMERA_DIRECTORY_TYPE_VALUES = DIR_CAMERA;
const std::string CAMERA_DIR_VALUES = "Camera/";
const std::string CAMERA_TYPE_VALUES = std::to_string(MEDIA_TYPE_IMAGE) + "?" + std::to_string(MEDIA_TYPE_VIDEO);
const int VIDEO_DIRECTORY_TYPE_VALUES = DIR_VIDEO;
const std::string VIDEO_DIR_VALUES = "Videos/";
const std::string VIDEO_TYPE_VALUES = std::to_string(MEDIA_TYPE_VIDEO);
const int PIC_DIRECTORY_TYPE_VALUES = DIR_IMAGE;
const std::string PIC_DIR_VALUES = "Pictures/";
const std::string PIC_TYPE_VALUES = std::to_string(MEDIA_TYPE_IMAGE);
const int AUDIO_DIRECTORY_TYPE_VALUES = DIR_AUDIOS;
const std::string AUDIO_DIR_VALUES = "Audios/";
const std::string AUDIO_TYPE_VALUES = std::to_string(MEDIA_TYPE_AUDIO);
const int DOC_DIRECTORY_TYPE_VALUES = DIR_DOCUMENTS;
const std::string DOC_DIR_VALUES = "Documents/";
const int DOWNLOAD_DIRECTORY_TYPE_VALUES = DIR_DOWNLOAD;
const std::string DOWNLOAD_DIR_VALUES = "Download/";
const std::string DOCUMENT_BUCKET = "Document";
const std::string PHOTO_BUCKET = "Photo";
const std::string AUDIO_BUCKET = "Audio";
const std::string PRE_PATH_VALUES = "/storage/cloud/";
const std::string DOCUMENT_URI_AUTHORITY = "docs";
const std::string BACKUP_DATA_DIR_VALUE = "data/";
const std::string BACKUP_SINGLE_DATA_DIR_VALUE = ".backup/";
const std::string THUMB_DIR_VALUE = ".thumbs/Photo";
const std::string EDIT_DATA_DIR_VALUE = ".editData";
const std::string MEDIALIBRARY_TEMP_DIR = ".medialibrarytemp";
const std::string CLOUD_ENHANCEMENT_WATER_MARK_DIR = "/sys_prod/resource/camera";
const std::string CACHE_DIR_VALUE = ".cache";
const std::string HIGHLIGHT_INFO_OLD = ".thumbs/highlight";
const std::string HIGHLIGHT_INFO_NEW = "highlight";
const std::string MEDIA_PHOTO_URI = "file://media/Photo/";
const std::string HMDFS = "/mnt/hmdfs/";
const std::string CLOUD_MERGE_VIEW = "/account/cloud_merge_view/files/";
const std::string CUSTOM_RESTORE_VALUES = "custom_restore";

const int TRASH_ALBUM_ID_VALUES = 2;
const int FAVOURITE_ALBUM_ID_VALUES = 1;
const int FAVOURITE_ALBUM_TYPE_VALUES = 1;
const std::string FAVOURTIE_ALBUM_NAME_VALUES = "FavoritAlbum";

static constexpr int UNCREATE_FILE_TIMEPENDING = -1;
static constexpr int UNCLOSE_FILE_TIMEPENDING = -2;
static constexpr int UNOPEN_FILE_COMPONENT_TIMEPENDING = -3;

const int32_t DEFAULT_FILE_ID = 0;
const int32_t DEFAULT_SHARE_COUNT = 0;
const int32_t DEFAULT_LCD_JUMO_COUNT = 0;
} // namespace OHOS
} // namespace Media

#endif  // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_LIB_SERVICE_CONST_H_
