/*
 * Copyright (C) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_DATA_ABILITY_CONST_H_
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_DATA_ABILITY_CONST_H_

#include <map>

#include "medialibrary_type_const.h"
#include "media_column.h"

namespace OHOS {
namespace Media {

const int32_t MEDIA_RDB_VERSION = 274;

enum {
    VERSION_ADD_CLOUD = 2,
    VERSION_ADD_META_MODIFED = 3,
    VERSION_MODIFY_SYNC_STATUS = 4,
    VERSION_ADD_API10_TABLE = 5,
    VERSION_MODIFY_DELETE_TRIGGER = 6,
    VERSION_ADD_CLOUD_VERSION = 7,
    VERSION_UPDATE_CLOUD_PATH = 8,
    VERSION_UPDATE_API10_TABLE = 9,
    VERSION_ADD_TABLE_TYPE = 10,
    VERSION_ADD_PACKAGE_NAME = 11,
    VERSION_ADD_CLOUD_ALBUM = 12,
    VERSION_ADD_CAMERA_SHOT_KEY = 13,
    /**
     * Remove album count triggers for batch operation performance,
     * update PhotoAlbum.count by a query and an update(in a single transaction of course)
     * if number of assets in an album changes.
     */
    VERSION_REMOVE_ALBUM_COUNT_TRIGGER = 14,
    VERSION_ADD_ALL_EXIF = 15,
    VERSION_ADD_UPDATE_CLOUD_SYNC_TRIGGER = 16,
    VERSION_ADD_YEAR_MONTH_DAY = 17,
    VERSION_UPDATE_YEAR_MONTH_DAY = 18,
    VERSION_ADD_VISION_TABLE = 20,
    VERSION_ADD_PHOTO_EDIT_TIME = 21,
    VERSION_ADD_SHOOTING_MODE = 22,
    VERSION_FIX_INDEX_ORDER = 23,
    VERSION_ADD_FACE_TABLE = 24,
    VERSION_ADD_HIDDEN_VIEW_COLUMNS = 26,
    VERSION_ADD_HIDDEN_TIME = 27,
    VERSION_ADD_LAST_VISIT_TIME = 28,
    VERSION_ADD_LOCATION_TABLE = 29,
    VERSION_ADD_ALBUM_ORDER = 30,
    VERSION_ADD_SOURCE_ALBUM_TRIGGER = 31,
    VERSION_ADD_VISION_ALBUM = 32,
    VERSION_ADD_AESTHETIC_COMPOSITION_TABLE = 33,
    VERSION_ADD_FORM_MAP = 34,
    VERSION_UPDATE_LOCATION_TABLE = 35,
    VERSION_ADD_PHOTO_CLEAN_FLAG_AND_THUMB_STATUS = 36,
    VERSION_ADD_SEARCH_TABLE = 37,
    VERSION_FIX_DOCS_PATH = 38,
    VERSION_ADD_SALIENCY_TABLE = 39,
    VERSION_UPDATE_SOURCE_ALBUM_TRIGGER = 40,
    VERSION_ADD_IMAGE_VIDEO_COUNT = 41,
    VERSION_ADD_SCHPT_HIDDEN_TIME_INDEX = 42,
    VERSION_ADD_SHOOTING_MODE_TAG = 43,
    VERSION_CLEAR_LABEL_DATA = 44,
    VERSION_ADD_PORTRAIT_IN_ALBUM = 45,
    VERSION_UPDATE_GEO_TABLE = 46,
    VERSION_REOMOVE_SOURCE_ALBUM_TO_ANALYSIS = 47,
    VERSION_ADD_MULTISTAGES_CAPTURE = 48,
    VERSION_UPDATE_DATE_TO_MILLISECOND = 49,
    VERSION_ADD_HAS_ASTC = 50,
    VERSION_ADD_ADDRESS_DESCRIPTION = 51,
    VERSION_UPDATE_SPEC_FOR_ADD_SCREENSHOT = 52,
    VERSION_MOVE_SOURCE_ALBUM_TO_PHOTO_ALBUM_AND_ADD_COLUMNS = 53,
    VERSION_ADD_CLOUD_ID_INDEX = 54,
    VERSION_UPDATE_PHOTOS_MDIRTY_TRIGGER = 55,
    VERSION_ALBUM_REFRESH = 56,
    VERSION_ADD_FAVORITE_INDEX = 57,
    VERSION_MODIFY_SOURCE_ALBUM_TRIGGERS = 58,
    VERSION_ADD_IS_LOCAL_ALBUM = 59,
    VERSION_ADD_MISSING_UPDATES = 60,
    VERSION_UPDATE_MDIRTY_TRIGGER_FOR_SDIRTY = 61,
    VERSION_ADD_STOYR_TABLE = 62,
    VERSION_ADD_HEAD_AND_POSE_TABLE = 63,
    VERSION_ADD_OWNER_APPID = 64,
    VERSION_SHOOTING_MODE_CLOUD = 65,
    VERSION_ADD_IS_COVER_SATISFIED_COLUMN = 66,
    VERSION_ADD_VIDEO_LABEL_TABEL = 67,
    VERSION_ADD_SEGMENTATION_COLUMNS = 68,
    VERSION_UPDATE_SEARCH_INDEX = 69,
    VERSION_UPDATE_HIGHLIGHT_TABLE = 70,
    VERSION_UPDATE_HIGHLIGHT_COVER_TABLE = 71,
    VERSION_CREATE_PHOTOS_EXT_TABLE = 72,
    VERSION_UPDATE_VIDEO_LABEL_TABEL = 73,
    VERSION_ADD_FACE_OCCLUSION_AND_POSE_TYPE_COLUMN = 74,
    VERSION_MOVE_KVDB = 75,
    VERSION_ADD_DYNAMIC_RANGE_TYPE = 76,
    VERSION_UPDATE_PHOTO_ALBUM_BUNDLENAME = 77,
    VERSION_UPDATE_PHOTO_ALBUM_TIGGER = 78,
    VERSION_ADD_THUMB_LCD_SIZE_COLUMN = 79,
    VERSION_ADD_MOVING_PHOTO_EFFECT_MODE = 80,
    VERSION_UPDATE_HIGHLIGHT_TABLE_PRIMARY_KEY = 81,
    VERSION_UPDATE_VISION_TRIGGER_FOR_VIDEO_LABEL = 82,
    VERSION_ADD_IS_TEMP = 83,
    VERSION_ADD_OWNER_APPID_TO_FILES_TABLE = 84,
    VERSION_ADD_IS_TEMP_TO_TRIGGER = 85,
    VERSION_UPDATE_ANALYSIS_TABLES = 86,
    VERSION_UPDATE_PHOTO_THUMBNAIL_READY = 87,
    VERSION_ADD_FRONT_CAMERA_TYPE = 88,
    PHOTOS_CREATE_DISPLAYNAME_INDEX = 89,
    VERSION_PORTRAIT_COVER_SELECTION_ADD_COLUMNS = 90,
    VERSION_ADD_BURST_COVER_LEVEL_AND_BURST_KEY = 91,
    VERSION_ADD_COVER_POSITION = 92,
    VERSION_ADD_SCHPT_READY_INEDX = 93,
    VERSION_UPDATE_PORTRAIT_COVER_SELECTION_COLUMNS  = 94,
    VERSION_ADD_APP_URI_PERMISSION_INFO = 95,
    VERSION_UPDATE_SOURCE_ALBUM_AND_ALBUM_BUNDLENAME_TRIGGERS = 96,
    VERSION_CREATE_BURSTKEY_INDEX = 98,
    VERSION_UPDATE_PHOTO_INDEX_FOR_ALBUM_COUNT_COVER = 99,
    VERSION_UPDATE_VIDEO_LABEL_TABLE_FOR_SUB_LABEL_TYPE = 100,
    VERSION_UPGRADE_THUMBNAIL = 101,
    VISION_UPDATE_DATA_ADDED_INDEX = 102,
    VISION_UPDATE_SEARCH_INDEX_TRIGGER = 103,
    VISION_UPDATE_MULTI_CROP_INFO = 104,
    VISION_ADD_ORIGINAL_SUBTYPE = 105,
    VERSION_UPDATE_BURST_DIRTY = 106,
    VERSION_UDAPTE_DATA_UNIQUE = 107,
    VERSION_ADD_DETAIL_TIME = 108,
    VERSION_ADD_VIDEO_FACE_TABLE = 109,
    VERSION_ADD_OWNER_ALBUM_ID = 110,
    VERSION_CLOUD_ENAHCNEMENT = 111,
    VERSION_UPDATE_MDIRTY_TRIGGER_FOR_UPLOADING_MOVING_PHOTO = 112,
    VERSION_MOVE_AUDIOS = 113,
    VERSION_ADD_INDEX_FOR_FILEID = 114,
    VERSION_ADD_OCR_CARD_COLUMNS = 115,
    VERSION_UPDATE_AOI = 116,
    VERSION_UPDATE_VIDEO_FACE_TABLE = 117,
    VERSION_ADD_SUPPORTED_WATERMARK_TYPE = 118,
    VERSION_FIX_PHOTO_SCHPT_MEDIA_TYPE_INDEX = 119,
    VERSION_UPDATE_INDEX_FOR_COVER = 120,
    VERSION_ADD_ANALYSIS_ALBUM_TOTAL_TABLE = 121,
    VERSION_ADD_THUMBNAIL_VISIBLE = 122,
    VERSION_ADD_METARECOVERY = 123,
    VERSION_UPDATE_SEARCH_INDEX_TRIGGER_FOR_CLEAN_FLAG = 124,
    VERSION_ADD_COVER_PLAY_SERVICE_VERSION = 125,
    VERSION_ADD_HIGHLIGHT_MAP_TABLES = 126,
    VERSION_COMPAT_LIVE_PHOTO = 127,
    VERSION_CREATE_TAB_OLD_PHOTOS = 128,
    VERSION_ADD_HIGHLIGHT_TRIGGER = 129,
    VERSION_ALTER_THUMBNAIL_VISIBLE = 130,
    VERSION_ADD_HIGHLIGHT_VIDEO_COUNT_CAN_PACK = 131,
    VERSION_ADD_GEO_DEFAULT_VALUE = 132,
    VERSION_HDR_AND_CLOUD_ENHANCEMENT_FIX = 133,
    VERSION_THUMBNAIL_READY_FIX = 134,
    VERSION_UPDATE_DATETAKEN_AND_DETAILTIME = 135,
    VERSION_UPDATE_SOURCE_PHOTO_ALBUM_TRIGGER = 136,
    VERSION_UPDATE_URIPERMISSION_SOURCE_TOKEN_AND_TARGET_TOKEN = 137,
    VERSION_ADD_READY_COUNT_INDEX = 138,
    VERSION_FIX_PICTURE_LCD_SIZE = 139,
    VERSION_FIX_DATE_ADDED_INDEX = 140,
    VERSION_UPDATE_NEW_SOURCE_PHOTO_ALBUM_TRIGGER = 141,
    VERSION_REVERT_FIX_DATE_ADDED_INDEX = 142,
    VERSION_UPDATE_SEARCH_STATUS_TRIGGER_FOR_OWNER_ALBUM_ID = 143,
    VERSION_ADD_CLOUD_ENHANCEMENT_ALBUM_INDEX = 144,
    VERSION_HIGHLIGHT_CHANGE_FUNCTION = 145,
    VERSION_ADD_PHOTO_DATEADD_INDEX = 146,
    VERSION_ADD_ALBUM_INDEX = 147,
    VERSION_REFRESH_PERMISSION_APPID = 148,
    VERSION_UPDATE_PHOTOS_DATE_AND_IDX = 149,
    VERSION_ADD_CHECK_FLAG = 150,
    VERSION_ADD_HIGHLIGHT_ANALYSIS_PROGRESS = 151,
    VERSION_FIX_SOURCE_PHOTO_ALBUM_DATE_MODIFIED = 152,
    VERSION_UPDATE_LATITUDE_AND_LONGITUDE_DEFAULT_NULL = 153,
    VERSION_ADD_REFRESH_ALBUM_STATUS_COLUMN = 154,
    VERSION_FIX_SOURCE_ALBUM_UPDATE_TRIGGER_TO_USE_LPATH = 155,
    VERSION_UPDATE_PHOTOS_DATE_IDX = 156,
    VERSION_UPDATE_MEDIA_TYPE_AND_THUMBNAIL_READY_IDX = 157,
    VERSION_FIX_PHOTO_QUALITY_CLONED = 158,
    VERSION_ADD_STAGE_VIDEO_TASK_STATUS = 159,
    VERSION_HIGHLIGHT_SUBTITLE = 160,
    VERSION_ADD_IS_AUTO = 161,
    VERSION_ADD_MEDIA_SUFFIX_COLUMN = 162,
    VERSION_UPDATE_SOURCE_PHOTO_ALBUM_TRIGGER_AGAIN = 163,
    VERSION_ADD_MEDIA_IS_RECENT_SHOW_COLUMN = 164,
    VERSION_CREATE_TAB_FACARD_PHOTOS = 165, // ABANDONED
    VERSION_FIX_SOURCE_ALBUM_CREATE_TRIGGERS_TO_USE_LPATH = 166,
    VERSION_ADD_ALBUM_PLUGIN_BUNDLE_NAME = 167,
    VERSION_ADD_FOREGROUND_ANALYSIS = 168,
    VERSION_HIGHLIGHT_MOVING_PHOTO = 169,
    VERSION_UPDATE_LOCATION_KNOWLEDGE_INDEX = 170,
    VERSION_IMAGE_FACE_TAG_ID_INDEX = 171,
    VERSION_ADD_GROUP_TAG_INDEX = 172,
    VERSION_ANALYZE_PHOTOS = 173,
    VERSION_CREATE_TAB_ASSET_ALBUM_OPERATION = 174,
    VERSION_MDIRTY_TRIGGER_UPLOAD_DETAIL_TIME = 175,
    VERSION_CREATE_OPERATION_ALBUM_UPDATE_TRIGGER = 176,
    VERSION_ADD_ANALYSIS_PHOTO_MAP_MAP_ASSET_INDEX = 177,
    VERSION_UPDATE_MDIRTY_TRIGGER_FOR_TDIRTY = 178,
    VERSION_ADD_ALBUM_SUBTYPE_AND_NAME_INDEX = 179,
    VERSION_FIX_DB_UPGRADE_FROM_API15 = 250,
    VERSION_CREATE_TAB_ASSET_ALBUM_OPERATION_FOR_SYNC = 251,
    VERSION_CREATE_TAB_FACARD_PHOTOS_RETRY = 252,
    VERSION_UPDATE_SEARCH_STATUS_TRIGGER_FOR_IS_FAVORITE = 253,
    VERSION_UPGRADE_ANALYSIS_UPDATE_SEARCH_TRIGGER = 254,
    VERSION_ADD_DC_ANALYSIS = 255,
    VERSION_CLOUD_MEDIA_UPGRADE = 256,
    VERSION_ADD_VISIT_COUNT = 257,
    VERSION_CREATE_TAB_CUSTOM_RECORDS = 258,
    VERSION_FIX_TAB_EXT_DIRTY_DATA = 259,
    VERSION_ADD_DC_ANALYSIS_INDEX_UPDATE = 260,
    VERSION_ADD_IS_RECTIFICATION_COVER = 261,
    VERSION_ADD_COVER_URI_SOURCE = 262,
    VERSION_FIX_ORIENTATION_180_DIRTY_THUMBNAIL = 263,
    VERSION_ADD_PHOTO_ALBUM_REFRESH_COLUMNS = 264,
    VERSION_ADD_EDITDATA_SIZE_COLUMN = 265,
    VERSION_ADD_AESTHETICS_SCORE_FIELDS = 266,
    VERSION_ADD_INDEX_FOR_PHOTO_SORT = 267,
    VERSION_ADD_HIGHLIGHT_LOCATION = 268,
    VERSION_ADD_PHOTO_QUERY_THUMBNAIL_WHITE_BLOCKS_INDEX = 269,
    VERSION_ADD_PRIORITY_COLUMN = 270,
    VERSION_ADD_ALBUMS_ORDER_KEYS_COLUMNS = 271,
    VERSION_ADD_IS_PRIMARY_FACE = 272,
    VERSION_DROP_PHOTO_INSERT_SOURCE_PHOTO_TRIGGER = 273,
    VERSION_SHOOTING_MODE_ALBUM_SECOND_INTERATION = 274,
};
enum {
    MEDIA_API_VERSION_DEFAULT = 8,
    MEDIA_API_VERSION_V9,
    MEDIA_API_VERSION_V10,
};

enum CloudFilePosition {
    POSITION_LOCAL = 1 << 0,
    POSITION_CLOUD = 1 << 1,
};

const std::string MEDIA_LIBRARY_VERSION = "1.0";

const int32_t DEVICE_SYNCSTATUSING = 0;
const int32_t DEVICE_SYNCSTATUS_COMPLETE = 1;

const std::string MEDIALIBRARY_TABLE = "Files";
const std::string SMARTALBUM_TABLE = "SmartAlbum";
const std::string SMARTALBUM_MAP_TABLE = "SmartMap";
const std::string CATEGORY_SMARTALBUM_MAP_TABLE = "CategorySmartAlbumMap";
const std::string MEDIATYPE_DIRECTORY_TABLE = "MediaTypeDirectory";
const std::string DEVICE_TABLE = "Device";
const std::string BUNDLE_PERMISSION_TABLE = "BundlePermission";
const std::string MEDIA_DATA_ABILITY_DB_NAME = "media_library.db";

const std::string BUNDLE_NAME = "com.ohos.medialibrary.medialibrarydata";

constexpr int64_t AGING_TIME = 30LL * 60 * 60 * 24 * 1000;

const std::string MEDIA_DATA_DB_ID = "file_id";
const std::string MEDIA_DATA_DB_URI = "uri";
const std::string MEDIA_DATA_DB_FILE_PATH = "data";
const std::string MEDIA_DATA_DB_SIZE = "size";
const std::string MEDIA_DATA_DB_PARENT_ID = "parent";
const std::string MEDIA_DATA_DB_DATE_MODIFIED = "date_modified";
const std::string MEDIA_DATA_DB_DATE_MODIFIED_S = "date_modified_s";
const std::string MEDIA_DATA_DB_DATE_MODIFIED_MS = "date_modified_ms";
const std::string MEDIA_DATA_DB_DATE_MODIFIED_TO_SECOND = "CAST(date_modified / 1000 AS BIGINT) AS date_modified_s";
const std::string MEDIA_DATA_DB_DATE_ADDED = "date_added";
const std::string MEDIA_DATA_DB_DATE_ADDED_S = "date_added_s";
const std::string MEDIA_DATA_DB_DATE_ADDED_MS = "date_added_ms";
const std::string MEDIA_DATA_DB_DATE_ADDED_TO_SECOND = "CAST(date_added / 1000 AS BIGINT) AS date_added_s";
const std::string MEDIA_DATA_DB_MIME_TYPE = "mime_type";
const std::string MEDIA_DATA_DB_TITLE = "title";
const std::string MEDIA_DATA_DB_DESCRIPTION = "description";
const std::string MEDIA_DATA_DB_NAME = "display_name";
const std::string MEDIA_DATA_DB_ORIENTATION = "orientation";
const std::string MEDIA_DATA_DB_LATITUDE = "latitude";
const std::string MEDIA_DATA_DB_LONGITUDE = "longitude";
const std::string MEDIA_DATA_DB_DATE_TAKEN = "date_taken";
const std::string MEDIA_DATA_DB_DATE_TAKEN_S = "date_taken_s";
const std::string MEDIA_DATA_DB_DATE_TAKEN_MS = "date_taken_ms";
const std::string MEDIA_DATA_DB_DATE_TAKEN_TO_SECOND = "CAST(date_taken / 1000 AS BIGINT) AS date_taken_s";
const std::string MEDIA_DATA_DB_THUMBNAIL = "thumbnail";
const std::string MEDIA_DATA_DB_THUMB_ASTC = "astc";
const std::string MEDIA_DATA_DB_HAS_ASTC = "has_astc"; // This attribute has been replaced by "thumbnail_ready"
const std::string MEDIA_DATA_DB_CONTENT_CREATE_TIME = "content_create_time";
const std::string MEDIA_DATA_DB_POSITION = "position";
const std::string MEDIA_DATA_DB_DIRTY = "dirty";
const std::string MEDIA_DATA_DB_CLOUD_ID = "cloud_id";
const std::string MEDIA_DATA_DB_META_DATE_MODIFIED = "meta_date_modified";
const std::string MEDIA_DATA_DB_SYNC_STATUS = "sync_status";
const std::string MEDIA_DATA_DB_THUMBNAIL_READY = "thumbnail_ready";
const std::string MEDIA_DATA_DB_ALL_EXIF = "all_exif";
const std::string MEDIA_DATA_DB_SHOOTING_MODE = "shooting_mode";
const std::string MEDIA_DATA_DB_SHOOTING_MODE_TAG = "shooting_mode_tag";
const std::string MEDIA_DATA_DB_PHOTOS_LATITUDE = "photos." + MEDIA_DATA_DB_LATITUDE;
const std::string MEDIA_DATA_DB_PHOTOS_LONGITUDE = "photos." + MEDIA_DATA_DB_LONGITUDE;
const std::string MEDIA_DATA_DB_USER_OPERATION = "user_operation";
const std::string MEDIA_DATA_DB_RENAME_OPERATION = "rename_operation";
const std::string MEDIA_DATA_DB_COVER_SATISFIED = "is_cover_satisfied";

const std::string MEDIA_DATA_DB_LCD = "lcd";
const std::string MEDIA_DATA_DB_BUCKET_ID = "bucket_id";
const std::string MEDIA_DATA_DB_BUCKET_NAME = "bucket_display_name";
const std::string MEDIA_DATA_DB_DURATION = "duration";
const std::string MEDIA_DATA_DB_ARTIST = "artist";

const std::string MEDIA_DATA_DB_AUDIO_ALBUM = "audio_album";
const std::string MEDIA_DATA_DB_MEDIA_TYPE = "media_type";

const std::string MEDIA_DATA_DB_HEIGHT = "height";
const std::string MEDIA_DATA_DB_WIDTH = "width";
const std::string MEDIA_DATA_DB_OWNER_PACKAGE = "owner_package";
const std::string MEDIA_DATA_DB_OWNER_APPID = "owner_appid";
const std::string MEDIA_DATA_DB_PACKAGE_NAME = "package_name";

const std::string MEDIA_DATA_DB_IS_FAV = "is_favorite";
const std::string MEDIA_DATA_DB_IS_TRASH = "is_trash";
const std::string MEDIA_DATA_DB_RECYCLE_PATH = "recycle_path";
const std::string MEDIA_DATA_DB_DATE_TRASHED = "date_trashed";
const std::string MEDIA_DATA_DB_DATE_TRASHED_S = "date_trashed_s";
const std::string MEDIA_DATA_DB_DATE_TRASHED_MS = "date_trashed_ms";
const std::string MEDIA_DATA_DB_DATE_TRASHED_TO_SECOND = "CAST(date_trashed / 1000 AS BIGINT) AS date_trashed_s";
const std::string MEDIA_DATA_DB_IS_PENDING = "is_pending";
const std::string MEDIA_DATA_DB_TIME_PENDING = "time_pending";
const std::string MEDIA_DATA_DB_RELATIVE_PATH = "relative_path";
const std::string MEDIA_DATA_DB_VOLUME_NAME = "volume_name";
const std::string MEDIA_DATA_DB_SELF_ID = "self_id";
const std::string MEDIA_DATA_DB_DEVICE_NAME = "device_name";

const std::string MEDIA_DATA_DB_ALBUM = "album";
const std::string MEDIA_DATA_DB_ALBUM_ID = "album_id";
const std::string MEDIA_DATA_DB_REFERENCE_ALBUM_ID = "reference_album_id";
const std::string MEDIA_DATA_DB_ALBUM_NAME = "album_name";
const std::string MEDIA_DATA_DB_COUNT = "count";
const std::string MEDIA_DATA_BUNDLENAME = "bundle_name";
const std::string MEDIA_DATA_DB_IS_LOCAL = "is_local";

const std::string MEDIA_DATA_CALLING_UID = "calling_uid";

const std::map<std::string, std::string> DATE_TRANSITION_MAP = {
    { MEDIA_DATA_DB_DATE_MODIFIED_MS, MEDIA_DATA_DB_DATE_MODIFIED },
    { MEDIA_DATA_DB_DATE_ADDED_MS, MEDIA_DATA_DB_DATE_ADDED },
    { MEDIA_DATA_DB_DATE_TRASHED_MS, MEDIA_DATA_DB_DATE_TRASHED },
    { MEDIA_DATA_DB_DATE_TAKEN_MS, MEDIA_DATA_DB_DATE_TAKEN },
};

const std::string MEDIA_DATA_DB_ALARM_URI = "alarm_uri";

const std::string MEDIA_DATA_DB_PHOTO_ID = "photo_id";
const std::string MEDIA_DATA_DB_PHOTO_QUALITY = "photo_quality";
const std::string MEDIA_DATA_DB_STAGE_VIDEO_TASK_STATUS = "stage_video_task_status";

const std::string MEDIA_COLUMN_COUNT = "count(*)";
const std::string MEDIA_COLUMN_COUNT_1 = "count(1)";
const std::string MEDIA_SUM_SIZE = "sum(size)";

const std::string PHOTO_INDEX = "photo_index";

const std::string PERMISSION_ID = "id";
const std::string PERMISSION_BUNDLE_NAME = "bundle_name";
const std::string PERMISSION_FILE_ID = "file_id";
const std::string PERMISSION_MODE = "mode";
const std::string PERMISSION_TABLE_TYPE = "table_type";

const std::string ALBUM_TABLE = "album";
const std::string ALBUM_VIEW_NAME = "Album";

const std::string ASSETMAP_VIEW_NAME = "AssetMap";

const std::string IMAGE_ASSET_TYPE = "image";
const std::string VIDEO_ASSET_TYPE = "video";
const std::string AUDIO_ASSET_TYPE = "audio";

// Caution: Keep same definition as MediaColumn! Only for where clause check in API9 getAlbums and album.getFileAssets
const std::string COMPAT_ALBUM_SUBTYPE = "album_subtype";
const std::string COMPAT_HIDDEN = "hidden";
const std::string COMPAT_PHOTO_SYNC_STATUS = "sync_status";
const std::string COMPAT_FILE_SUBTYPE = "subtype";
const std::string COMPAT_CAMERA_SHOT_KEY = "camera_shot_key";

// system relativePath and albumName, use for API9 compatible API10
const std::string CAMERA_PATH = "Camera/";
const std::string SCREEN_SHOT_PATH = "Pictures/Screenshots/";
const std::string SCREEN_RECORD_PATH = "Videos/ScreenRecordings/";

const std::string CAMERA_ALBUM_NAME = "Camera";
const std::string SCREEN_SHOT_ALBUM_NAME = "Screenshots";
const std::string SCREEN_RECORD_ALBUM_NAME = "ScreenRecordings";

// extension
const std::string ASSET_EXTENTION = "extention";

// edit param
const std::string EDIT_DATA_REQUEST = "edit_data_request";  // MEDIA_OPERN_KEYWORD=EDIT_DATA_REQUEST
const std::string SOURCE_REQUEST = "source_request";        // MEDIA_OPERN_KEYWORD=SOURCE_REQUEST
const std::string COMMIT_REQUEST = "commit_request";        // MEDIA_OPERN_KEYWORD=COMMIT_REQUEST
const std::string EDIT_DATA = "edit_data";
const std::string COMPATIBLE_FORMAT = "compatible_format";
const std::string FORMAT_VERSION = "format_version";
const std::string APP_ID = "app_id";

// write cache
const std::string CACHE_FILE_NAME = "cache_file_name";
const std::string CACHE_MOVING_PHOTO_VIDEO_NAME = "cache_moving_photo_video_name";

// moving photo param
const std::string CREATE_MOVING_PHOTO_VIDEO = "create_video";
const std::string OPEN_MOVING_PHOTO_VIDEO = "open_video"; // MEDIA_MOVING_PHOTO_OPRN_KEYWORD=OPEN_MOVING_PHOTO_VIDEO
const std::string OPEN_PRIVATE_MOVING_PHOTO_METADATA = "open_metadata";
const std::string OPEN_PRIVATE_LIVE_PHOTO = "open_private_live_photo";
const std::string OPEN_MOVING_PHOTO_VIDEO_CLOUD = "open_moving_photo_video_cloud";
const std::string NOTIFY_VIDEO_SAVE_FINISHED = "notify_video_save_finished"; // movingPhoto video save finish

// db sandbox directory
const std::string MEDIA_DB_DIR = "/data/medialibrary/database";

// slave medialibrary db file path
const std::string MEDIA_DB_FILE_SLAVE = "/data/storage/el2/database/rdb/media_library_slave.db";
const std::string MEDIA_DB_FILE_SLAVE_SHM = "/data/storage/el2/database/rdb/media_library_slave.db-shm";
const std::string MEDIA_DB_FILE_SLAVE_WAL = "/data/storage/el2/database/rdb/media_library_slave.db-wal";

// requestId for generating thumbnail in batches
const std::string THUMBNAIL_BATCH_GENERATE_REQUEST_ID = "thumbnail_request_id";
const std::string IMAGE_FILE_TYPE = "image_file_type";

const std::string RESTORE_REQUEST_ASTC_GENERATE_COUNT = "restore_request_astc_generate_count";
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_DATA_ABILITY_CONST_H_
