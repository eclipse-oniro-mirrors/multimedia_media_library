/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "media_column.h"

#include "base_column.h"
#include "medialibrary_db_const.h"
#include "highlight_column.h"

namespace OHOS {
namespace Media {
using namespace std;

const std::string MediaColumn::MEDIA_ID = "file_id";
const std::string MediaColumn::MEDIA_FILE_PATH = "data";
const std::string MediaColumn::MEDIA_SIZE = "size";
const std::string MediaColumn::MEDIA_TITLE = "title";
const std::string MediaColumn::MEDIA_NAME = "display_name";
const std::string MediaColumn::MEDIA_TYPE = "media_type";
const std::string MediaColumn::MEDIA_MIME_TYPE = "mime_type";
const std::string MediaColumn::MEDIA_OWNER_PACKAGE = "owner_package";
const std::string MediaColumn::MEDIA_OWNER_APPID = "owner_appid";
const std::string MediaColumn::MEDIA_PACKAGE_NAME = "package_name";
const std::string MediaColumn::MEDIA_DEVICE_NAME = "device_name";
const std::string MediaColumn::MEDIA_DATE_MODIFIED = "date_modified";
const std::string MediaColumn::MEDIA_DATE_ADDED = "date_added";
const std::string MediaColumn::MEDIA_DATE_TAKEN = "date_taken";
const std::string MediaColumn::MEDIA_DURATION = "duration";
const std::string MediaColumn::MEDIA_TIME_PENDING = "time_pending";
const std::string MediaColumn::MEDIA_IS_FAV = "is_favorite";
const std::string MediaColumn::MEDIA_DATE_TRASHED = "date_trashed";
const std::string MediaColumn::MEDIA_DATE_DELETED = "date_deleted";
const std::string MediaColumn::MEDIA_HIDDEN = "hidden";
const std::string MediaColumn::MEDIA_PARENT_ID = "parent";
const std::string MediaColumn::MEDIA_RELATIVE_PATH = "relative_path";
const std::string MediaColumn::MEDIA_VIRTURL_PATH = "virtual_path";
const std::set<std::string> MediaColumn::MEDIA_COLUMNS = {
    MEDIA_ID, MEDIA_FILE_PATH, MEDIA_SIZE, MEDIA_TITLE, MEDIA_NAME, MEDIA_TYPE, MEDIA_MIME_TYPE,
    MEDIA_OWNER_PACKAGE, MEDIA_OWNER_APPID, MEDIA_PACKAGE_NAME, MEDIA_DEVICE_NAME, MEDIA_DATE_MODIFIED,
    MEDIA_DATE_ADDED, MEDIA_DATE_TAKEN, MEDIA_DURATION, MEDIA_TIME_PENDING, MEDIA_IS_FAV, MEDIA_DATE_TRASHED,
    MEDIA_DATE_DELETED, MEDIA_HIDDEN, MEDIA_PARENT_ID, MEDIA_RELATIVE_PATH, MEDIA_VIRTURL_PATH
};
const std::set<std::string> MediaColumn::DEFAULT_FETCH_COLUMNS = {
    MEDIA_ID, MEDIA_FILE_PATH, MEDIA_NAME, MEDIA_TYPE
};

const std::string PhotoColumn::PHOTO_DIRTY = "dirty";
const std::string PhotoColumn::PHOTO_CLOUD_ID = "cloud_id";
const std::string PhotoColumn::PHOTO_META_DATE_MODIFIED = "meta_date_modified";
const std::string PhotoColumn::PHOTO_SYNC_STATUS = "sync_status";
const std::string PhotoColumn::PHOTO_CLOUD_VERSION = "cloud_version";
const std::string PhotoColumn::PHOTO_ORIENTATION = "orientation";
const std::string PhotoColumn::PHOTO_EXIF_ROTATE = "exif_rotate";
const std::string PhotoColumn::PHOTO_LATITUDE = "latitude";
const std::string PhotoColumn::PHOTO_LONGITUDE = "longitude";
const std::string PhotoColumn::PHOTO_HEIGHT = "height";
const std::string PhotoColumn::PHOTO_WIDTH = "width";
const std::string PhotoColumn::PHOTO_LCD_VISIT_TIME = "lcd_visit_time";
const std::string PhotoColumn::PHOTO_EDIT_TIME = "edit_time";
const std::string PhotoColumn::PHOTO_POSITION = "position";
const std::string PhotoColumn::PHOTO_SUBTYPE = "subtype";
const std::string PhotoColumn::CAMERA_SHOT_KEY = "camera_shot_key";
const std::string PhotoColumn::PHOTO_USER_COMMENT = "user_comment";
const std::string PhotoColumn::PHOTO_SHOOTING_MODE = "shooting_mode";
const std::string PhotoColumn::PHOTO_SHOOTING_MODE_TAG = "shooting_mode_tag";
const std::string PhotoColumn::PHOTO_ALL_EXIF = "all_exif";
const std::string PhotoColumn::PHOTO_DATE_YEAR = "date_year";
const std::string PhotoColumn::PHOTO_DATE_MONTH = "date_month";
const std::string PhotoColumn::PHOTO_DATE_DAY = "date_day";
const std::string PhotoColumn::PHOTO_LAST_VISIT_TIME = "last_visit_time";
const std::string PhotoColumn::PHOTO_HIDDEN_TIME = "hidden_time";
const std::string PhotoColumn::PHOTO_THUMB_STATUS = "thumb_status";
const std::string PhotoColumn::PHOTO_CLEAN_FLAG = "clean_flag";
const std::string PhotoColumn::PHOTO_ID = "photo_id";
const std::string PhotoColumn::PHOTO_QUALITY = "photo_quality";
const std::string PhotoColumn::PHOTO_FIRST_VISIT_TIME = "first_visit_time";
const std::string PhotoColumn::PHOTO_DEFERRED_PROC_TYPE = "deferred_proc_type";
const std::string PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE = "dynamic_range_type";
const std::string PhotoColumn::PHOTO_HDR_MODE = "hdr_mode";
const std::string PhotoColumn::MOVING_PHOTO_EFFECT_MODE = "moving_photo_effect_mode";
const std::string PhotoColumn::PHOTO_LCD_SIZE = "lcd_size";
const std::string PhotoColumn::PHOTO_THUMB_SIZE = "thumb_size";
const std::string PhotoColumn::PHOTO_HAS_ASTC = "has_astc"; // This attribute has been replaced by "thumbnail_ready"
const std::string PhotoColumn::PHOTO_IS_TEMP = "is_temp";
const std::string PhotoColumn::PHOTO_THUMBNAIL_READY = "thumbnail_ready";
const std::string PhotoColumn::PHOTO_THUMBNAIL_VISIBLE = "thumbnail_visible";
const std::string PhotoColumn::PHOTO_FRONT_CAMERA = "front_camera";
const std::string PhotoColumn::PHOTO_BURST_COVER_LEVEL = "burst_cover_level";
const std::string PhotoColumn::PHOTO_BURST_KEY = "burst_key";
const std::string PhotoColumn::PHOTO_COVER_POSITION = "cover_position";
const std::string PhotoColumn::PHOTO_IS_RECTIFICATION_COVER = "is_rectification_cover";
const std::string PhotoColumn::PHOTO_ORIGINAL_SUBTYPE = "original_subtype";
const std::string PhotoColumn::PHOTO_DETAIL_TIME = "detail_time";
const std::string PhotoColumn::PHOTO_OWNER_ALBUM_ID = "owner_album_id";
const std::string PhotoColumn::PHOTO_ORIGINAL_ASSET_CLOUD_ID = "original_asset_cloud_id";
const std::string PhotoColumn::PHOTO_SOURCE_PATH = "source_path";
const std::string PhotoColumn::PHOTO_CE_AVAILABLE = "ce_available";
const std::string PhotoColumn::PHOTO_CE_STATUS_CODE = "ce_status_code";
const std::string PhotoColumn::PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE = "moving_photo_enhancement_type";
const std::string PhotoColumn::PHOTO_STRONG_ASSOCIATION = "strong_association";
const std::string PhotoColumn::PHOTO_ASSOCIATE_FILE_ID = "associate_file_id";
const std::string PhotoColumn::PHOTO_HAS_CLOUD_WATERMARK = "has_cloud_watermark";
const std::string PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS = "composite_display_status";
const std::string PhotoColumn::SUPPORTED_WATERMARK_TYPE = "supported_watermark_type";
const std::string PhotoColumn::PHOTO_METADATA_FLAGS = "metadata_flags";
const std::string PhotoColumn::PHOTO_CHECK_FLAG = "check_flag";
const std::string PhotoColumn::STAGE_VIDEO_TASK_STATUS = "stage_video_task_status";
const std::string PhotoColumn::PHOTO_IS_AUTO = "is_auto";
const std::string PhotoColumn::PHOTO_MEDIA_SUFFIX = "media_suffix";
const std::string PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME = "real_lcd_visit_time";
const std::string PhotoColumn::PHOTO_VISIT_COUNT = "visit_count";
const std::string PhotoColumn::PHOTO_LCD_VISIT_COUNT = "lcd_visit_count";
const std::string PhotoColumn::PHOTO_FILE_SOURCE_TYPE = "file_source_type";
const std::string PhotoColumn::PHOTO_VIDEO_MODE = "video_mode";
const std::string PhotoColumn::PHOTO_IS_CRITICAL =  "is_critical";
const std::string PhotoColumn::PHOTO_CRITICAL_TYPE =  "critical_type";
const std::string PhotoColumn::PHOTO_IS_RECENT_SHOW = "is_recent_show";
const std::string PhotoColumn::PHOTO_HAS_APPLINK = "has_applink";
const std::string PhotoColumn::PHOTO_APPLINK = "applink";
const std::string PhotoColumn::PHOTO_CHANGE_TIME = "change_time";
const std::string PhotoColumn::PHOTO_TRANSCODE_TIME = "transcode_time";
const std::string PhotoColumn::PHOTO_TRANS_CODE_FILE_SIZE = "trans_code_file_size";
const std::string PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE = "exist_compatible_duplicate";
const std::string PhotoColumn::PHOTO_ASPECT_RATIO = "aspect_ratio";
const std::string PhotoColumn::PHOTO_EDIT_DATA_EXIST = "edit_data_exist";

const std::string PhotoColumn::PHOTO_FILE_INODE = "inode";
const std::string PhotoColumn::PHOTO_STORAGE_PATH = "storage_path";

const std::string PhotoColumn::PHOTO_CLOUD_ID_INDEX = "cloud_id_index";
const std::string PhotoColumn::PHOTO_DATE_YEAR_INDEX = "date_year_index";
const std::string PhotoColumn::PHOTO_DATE_MONTH_INDEX = "date_month_index";
const std::string PhotoColumn::PHOTO_DATE_DAY_INDEX = "date_day_index";
const std::string PhotoColumn::PHOTO_SCHPT_ADDED_INDEX = "idx_schpt_date_added";
// index of date_added for photo table
const std::string PhotoColumn::PHOTO_SCHPT_PHOTO_DATEADDED_INDEX = "idx_schpt_date_added_new";
const std::string PhotoColumn::PHOTO_SCHPT_ADDED_ALBUM_INDEX = "idx_schpt_date_added_album";
const std::string PhotoColumn::PHOTO_SCHPT_ALBUM_GENERAL_INDEX = "idx_schpt_album_general";
const std::string PhotoColumn::PHOTO_SCHPT_ALBUM_INDEX = "idx_schpt_album";
const std::string PhotoColumn::PHOTO_SCHPT_MEDIA_TYPE_INDEX = "idx_schpt_media_type";
const std::string PhotoColumn::PHOTO_SCHPT_DAY_INDEX = "idx_schpt_date_day";
const std::string PhotoColumn::PHOTO_HIDDEN_TIME_INDEX = "hidden_time_index";
const std::string PhotoColumn::PHOTO_SCHPT_HIDDEN_TIME_INDEX = "idx_schpt_hidden_time";
const std::string PhotoColumn::PHOTO_FAVORITE_INDEX = "idx_photo_is_favorite";
const std::string PhotoColumn::PHOTO_DISPLAYNAME_INDEX = "idx_display_name";
const std::string PhotoColumn::PHOTO_SCHPT_READY_INDEX = "idx_schpt_thumbnail_ready";
const std::string PhotoColumn::PHOTO_BURSTKEY_INDEX = "idx_burstkey";
const std::string PhotoColumn::PHOTO_SCHPT_MEDIA_TYPE_COUNT_READY_INDEX = "idx_schpt_media_type_ready";
const std::string PhotoColumn::PHOTO_SCHPT_DATE_YEAR_COUNT_READY_INDEX = "idx_schpt_date_year_ready";
const std::string PhotoColumn::PHOTO_SCHPT_DATE_MONTH_COUNT_READY_INDEX = "idx_schpt_date_month_ready";
const std::string PhotoColumn::PHOTO_SCHPT_CLOUD_ENHANCEMENT_ALBUM_INDEX = "idx_schpt_cloud_enhancement_album_index";
const std::string PhotoColumn::LATITUDE_INDEX = "idx_latitude";
const std::string PhotoColumn::LONGITUDE_INDEX = "idx_longtitude";
const std::string PhotoColumn::PHOTO_SORT_MEDIA_TYPE_DATE_ADDED_INDEX = "idx_photo_sort_media_type_date_added";
const std::string PhotoColumn::PHOTO_SORT_MEDIA_TYPE_DATE_TAKEN_INDEX = "idx_photo_sort_media_type_date_taken";
const std::string PhotoColumn::PHOTO_SORT_IN_ALBUM_DATE_ADDED_INDEX = "idx_photo_sort_in_album_date_added";
const std::string PhotoColumn::PHOTO_SORT_IN_ALBUM_DATE_TAKEN_INDEX = "idx_photo_sort_in_album_date_taken";
const std::string PhotoColumn::PHOTO_SORT_IN_ALBUM_SIZE_INDEX = "idx_photo_sort_in_album_size";
const std::string PhotoColumn::PHOTO_SORT_MEDIA_TYPE_SIZE_INDEX = "idx_photo_sort_media_type_size";
const std::string PhotoColumn::PHOTO_SORT_IN_ALBUM_DISPLAY_NAME_INDEX = "idx_photo_sort_in_album_display_name";
const std::string PhotoColumn::PHOTO_SORT_MEDIA_TYPE_DISPLAY_NAME_INDEX = "idx_photo_sort_media_type_display_name";
const std::string PhotoColumn::PHOTO_QUERY_THUMBNAIL_WHITE_BLOCKS_INDEX = "idx_query_thumbnail_white_blocks_num";
const std::string PhotoColumn::PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX = "idx_shooting_mode_album_general";
const std::string PhotoColumn::PHOTO_BURST_MODE_ALBUM_INDEX = "idx_burst_mode_album";
const std::string PhotoColumn::PHOTO_FRONT_CAMERA_ALBUM_INDEX = "idx_front_camera_album";
const std::string PhotoColumn::PHOTO_RAW_IMAGE_ALBUM_INDEX = "idx_raw_image_album";
const std::string PhotoColumn::PHOTO_MOVING_PHOTO_ALBUM_INDEX = "idx_moving_photo_album";

const std::string PhotoColumn::PHOTO_DATE_YEAR_FORMAT = "%Y";
const std::string PhotoColumn::PHOTO_DATE_MONTH_FORMAT = "%Y%m";
const std::string PhotoColumn::PHOTO_DATE_DAY_FORMAT = "%Y%m%d";
const std::string PhotoColumn::PHOTO_DETAIL_TIME_FORMAT = "%Y:%m:%d %H:%M:%S";

const std::string PhotoColumn::PHOTOS_TABLE = "Photos";
const std::string PhotoColumn::TAB_OLD_PHOTOS_TABLE = "tab_old_photos";
const std::string PhotoColumn::TAB_ASSET_AND_ALBUM_OPERATION_TABLE = "tab_asset_and_album_operation";

const std::string PhotoColumn::FILES_CLOUD_DIR = "/storage/cloud/files/";
const std::string PhotoColumn::FILES_LOCAL_DIR = "/storage/media/local/files/";

const std::string PhotoColumn::HIGHLIGHT_TABLE = "tab_analysis_video_label";
const std::string PhotoColumn::MEDIA_DATA_DB_HIGHLIGHT_TRIGGER = "trigger_generate_thumbnail";

const std::string PhotoColumn::PHOTO_URI_PREFIX = "file://media/Photo/";
const std::string PhotoColumn::DEFAULT_PHOTO_URI = "file://media/Photo";
const std::string PhotoColumn::PHOTO_CACHE_URI_PREFIX = "file://media/Photo/cache/";
const std::string PhotoColumn::PHOTO_TYPE_URI = "/Photo";
const std::string PhotoColumn::HIGHTLIGHT_COVER_URI = "/highlight";
const std::string PhotoColumn::HIGHTLIGHT_URI = "/highlight/video";
const std::string PhotoColumn::HIDDEN_PHOTO_URI_PREFIX = "file://media/hiddenPhoto/";
const std::string PhotoColumn::TRASHED_PHOTO_URI_PREFIX = "file://media/trashedPhoto/";

const std::string PhotoColumn::PHOTO_CLOUD_URI_PREFIX = "file://cloudsync/Photo/";
const std::string PhotoColumn::PHOTO_CLOUD_TRIGGER_PREFIX = "file://cloudsync/triggerGeneration/";
const std::string PhotoColumn::PHOTO_GALLERY_CLOUD_URI_PREFIX = "file://cloudsync/gallery/Photo/";

const std::string PhotoColumn::PHOTO_HEIGHT_ERROR_URI_PREFIX = "file://cloudsync/Photo/HeightError/";
const std::string PhotoColumn::PHOTO_DOWNLOAD_SUCCEED_URI_PREFIX = "file://cloudsync/Photo/DownloadSucceed/";

const std::string PhotoColumn::PHOTO_CLOUD_GALLERY_REBUILD_URI_PREFIX = "file://cloudsync/gallery/rebuild/";

const std::string PhotoColumn::PHOTO_REQUEST_PICTURE = "file://media/Photo/picture/";
const std::string PhotoColumn::PHOTO_REQUEST_PICTURE_BUFFER = "file://media/Photo/pictureBuffer/";

const std::string PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE = "south_device_type";
const std::string PhotoColumn::CLOUD_TYPE = "cloud_type";

const std::set<std::string> PhotoColumn::DEFAULT_FETCH_COLUMNS = {
    PHOTO_SUBTYPE, PHOTO_BURST_KEY,
};

const std::string PhotoColumn::CREATE_PHOTO_TABLE = "CREATE TABLE IF NOT EXISTS " +
    PHOTOS_TABLE + " (" +
    MEDIA_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    MEDIA_FILE_PATH + " TEXT, " +
    MEDIA_SIZE + " BIGINT, " +
    MEDIA_TITLE + " TEXT, " +
    MEDIA_NAME + " TEXT, " +
    MEDIA_TYPE + " INT, " +
    MEDIA_MIME_TYPE + " TEXT, " +
    MEDIA_OWNER_PACKAGE + " TEXT, " +
    MEDIA_OWNER_APPID + " TEXT, " +
    MEDIA_PACKAGE_NAME + " TEXT, " +
    MEDIA_DEVICE_NAME + " TEXT, " +
    MEDIA_DATE_ADDED + " BIGINT, " +
    MEDIA_DATE_MODIFIED + " BIGINT, " +
    MEDIA_DATE_TAKEN + " BIGINT DEFAULT 0, " +
    MEDIA_DURATION + " INT, " +
    MEDIA_TIME_PENDING + " BIGINT DEFAULT 0, " +
    MEDIA_IS_FAV + " INT DEFAULT 0, " +
    MEDIA_DATE_TRASHED + " BIGINT DEFAULT 0, " +
    MEDIA_DATE_DELETED + " BIGINT DEFAULT 0, " +
    MEDIA_HIDDEN + " INT DEFAULT 0, " +
    MEDIA_PARENT_ID + " INT DEFAULT 0, " +
    MEDIA_RELATIVE_PATH + " TEXT, " +
    MEDIA_VIRTURL_PATH + " TEXT UNIQUE, " +
    PHOTO_DIRTY + " INT DEFAULT 1, " +
    PHOTO_CLOUD_ID + " TEXT, " +
    PHOTO_META_DATE_MODIFIED + "  BIGINT DEFAULT 0, " +
    PHOTO_SYNC_STATUS + "  INT DEFAULT 0, " +
    PHOTO_CLOUD_VERSION + " BIGINT DEFAULT 0, " +
    PHOTO_ORIENTATION + " INT DEFAULT 0, " +
    PHOTO_EXIF_ROTATE + " INT NOT NULL DEFAULT 0, " +
    PHOTO_LATITUDE + " DOUBLE DEFAULT 0, " +
    PHOTO_LONGITUDE + " DOUBLE DEFAULT 0, " +
    PHOTO_HEIGHT + " INT, " +
    PHOTO_WIDTH + " INT, " +
    PHOTO_EDIT_TIME + " BIGINT DEFAULT 0, " +
    PHOTO_LCD_VISIT_TIME + " BIGINT DEFAULT 0, " +
    PHOTO_POSITION + " INT DEFAULT 1, " +
    PHOTO_SUBTYPE + " INT DEFAULT 0, " +
    PHOTO_ORIGINAL_SUBTYPE + " INT," +
    CAMERA_SHOT_KEY + " TEXT, " +
    PHOTO_USER_COMMENT + " TEXT, " +
    PHOTO_ALL_EXIF  + " TEXT, " +
    PHOTO_DATE_YEAR + " TEXT, " +
    PHOTO_DATE_MONTH + " TEXT, " +
    PHOTO_DATE_DAY + " TEXT, " +
    PHOTO_SHOOTING_MODE + " TEXT, " +
    PHOTO_SHOOTING_MODE_TAG + " TEXT, " +
    PHOTO_LAST_VISIT_TIME + " BIGINT DEFAULT 0, " +
    PHOTO_HIDDEN_TIME + " BIGINT DEFAULT 0, " +
    PHOTO_THUMB_STATUS + " INT DEFAULT 0, " +
    PHOTO_CLEAN_FLAG + " INT DEFAULT 0, " +
    PHOTO_ID + " TEXT, " +
    PHOTO_QUALITY + " INT, " +
    PHOTO_FIRST_VISIT_TIME + " BIGINT DEFAULT 0, " +
    PHOTO_DEFERRED_PROC_TYPE + " INT DEFAULT 0, " +
    PHOTO_DYNAMIC_RANGE_TYPE + " INT DEFAULT 0, " +
    MOVING_PHOTO_EFFECT_MODE + " INT DEFAULT 0, " +
    PHOTO_COVER_POSITION + " BIGINT DEFAULT 0, " +
    PHOTO_IS_RECTIFICATION_COVER + " INT NOT NULL DEFAULT 0, " +
    PHOTO_THUMBNAIL_READY + " BIGINT DEFAULT 0, " +
    PHOTO_LCD_SIZE + " TEXT, " +
    PHOTO_THUMB_SIZE + " TEXT," +
    PHOTO_FRONT_CAMERA + " TEXT, " +
    PHOTO_IS_TEMP + " INT DEFAULT 0," +
    PHOTO_BURST_COVER_LEVEL + " INT DEFAULT 1, " +
    PHOTO_BURST_KEY + " TEXT, " +
    PHOTO_DETAIL_TIME + " TEXT, " +
    PHOTO_OWNER_ALBUM_ID + " INT DEFAULT 0, " +
    PHOTO_ORIGINAL_ASSET_CLOUD_ID + " TEXT, " +
    PHOTO_SOURCE_PATH + " TEXT, " +
    PHOTO_CE_AVAILABLE + " INT DEFAULT 0, " +
    PHOTO_CE_STATUS_CODE + " INT, " +
    PHOTO_STRONG_ASSOCIATION + " INT DEFAULT 0, " +
    PHOTO_ASSOCIATE_FILE_ID + " INT DEFAULT 0, " +
    PHOTO_HAS_CLOUD_WATERMARK + " INT DEFAULT 0, " +
    PHOTO_THUMBNAIL_VISIBLE + " INT DEFAULT 0, " +
    SUPPORTED_WATERMARK_TYPE + " INT, " +
    PHOTO_METADATA_FLAGS + " INT DEFAULT 0, " +
    PHOTO_CHECK_FLAG + " INT DEFAULT 0, " +
    STAGE_VIDEO_TASK_STATUS + " INT NOT NULL DEFAULT 0, " +
    PHOTO_IS_AUTO + " INT NOT NULL DEFAULT 0, " +
    PHOTO_MEDIA_SUFFIX + " TEXT, " +
    PHOTO_IS_RECENT_SHOW + " INT NOT NULL DEFAULT 1, " +
    PHOTO_REAL_LCD_VISIT_TIME + " BIGINT NOT NULL DEFAULT 0, " +
    PHOTO_VISIT_COUNT + " INT NOT NULL DEFAULT 0, " +
    PHOTO_LCD_VISIT_COUNT + " INT NOT NULL DEFAULT 0," +
    PHOTO_HAS_APPLINK + " INT NOT NULL DEFAULT 0," +
    PHOTO_APPLINK + " TEXT, " +
    PHOTO_TRANSCODE_TIME + " BIGINT NOT NULL DEFAULT 0, " +
    PHOTO_TRANS_CODE_FILE_SIZE + " BIGINT NOT NULL DEFAULT 0, " +
    PHOTO_EXIST_COMPATIBLE_DUPLICATE + " INT NOT NULL DEFAULT 0, " +
    PHOTO_SOUTH_DEVICE_TYPE + " INT NOT NULL DEFAULT 0, " +
    PHOTO_COMPOSITE_DISPLAY_STATUS + " INT NOT NULL DEFAULT 0, " +
    PHOTO_FILE_INODE + " TEXT, " +
    PHOTO_STORAGE_PATH + " TEXT, " +
    PHOTO_FILE_SOURCE_TYPE + " INT NOT NULL DEFAULT 0, " +
    PHOTO_HDR_MODE + " INT NOT NULL DEFAULT 0, " +
    PHOTO_VIDEO_MODE + " INT NOT NULL DEFAULT -1, " +
    PHOTO_ASPECT_RATIO + " DOUBLE NOT NULL DEFAULT -2, " +
    PHOTO_CHANGE_TIME + " BIGINT NOT NULL DEFAULT 0, " +
    PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE + " INT NOT NULL DEFAULT 0, " +
    PHOTO_IS_CRITICAL + " INT NOT NULL DEFAULT 0," +
    PHOTO_CRITICAL_TYPE + " INT NOT NULL DEFAULT 0" +
    ") ";

const std::string PhotoColumn::CREATE_CLOUD_ID_INDEX = BaseColumn::CreateIndex() +
    PHOTO_CLOUD_ID_INDEX + " ON " + PHOTOS_TABLE + " (" + PHOTO_CLOUD_ID + " DESC)";

const std::string PhotoColumn::CREATE_YEAR_INDEX = BaseColumn::CreateIndex() +
    PHOTO_DATE_YEAR_INDEX + " ON " + PHOTOS_TABLE + " (" + PHOTO_DATE_YEAR + " DESC)";

const std::string PhotoColumn::CREATE_MONTH_INDEX = BaseColumn::CreateIndex() +
    PHOTO_DATE_MONTH_INDEX + " ON " + PHOTOS_TABLE + " (" + PHOTO_DATE_MONTH + " DESC)";

const std::string PhotoColumn::CREATE_DAY_INDEX = BaseColumn::CreateIndex() +
    PHOTO_DATE_DAY_INDEX + " ON " + PHOTOS_TABLE + " (" + PHOTO_DATE_DAY + " DESC)";

const std::string PhotoColumn::CREATE_SCHPT_DAY_INDEX = BaseColumn::CreateIndex() + PHOTO_SCHPT_DAY_INDEX + " ON " +
    PHOTOS_TABLE + " (" + PHOTO_SYNC_STATUS + "," + PHOTO_CLEAN_FLAG + "," + MEDIA_DATE_TRASHED + "," +
    MEDIA_TIME_PENDING + "," + MEDIA_HIDDEN + "," + PHOTO_IS_TEMP + "," + PHOTO_BURST_COVER_LEVEL + "," +
    PHOTO_DATE_DAY + " DESC, " + PHOTO_THUMBNAIL_VISIBLE + ");";

const std::string PhotoColumn::DROP_SCHPT_DAY_INDEX = BaseColumn::DropIndex() + PHOTO_SCHPT_DAY_INDEX;

const std::string PhotoColumn::DROP_SCHPT_MEDIA_TYPE_INDEX = "DROP INDEX IF EXISTS " + PHOTO_SCHPT_MEDIA_TYPE_INDEX;

const std::string PhotoColumn::DROP_BURST_MODE_ALBUM_INDEX = BaseColumn::DropIndex() + PHOTO_BURST_MODE_ALBUM_INDEX;

const std::string PhotoColumn::CREATE_SCHPT_MEDIA_TYPE_INDEX = BaseColumn::CreateIndex() +
    PHOTO_SCHPT_MEDIA_TYPE_INDEX + " ON " + PHOTOS_TABLE +
    " (" + PHOTO_SYNC_STATUS + "," + PHOTO_CLEAN_FLAG + "," + MEDIA_DATE_TRASHED + "," + MEDIA_HIDDEN +
    "," + MEDIA_TIME_PENDING + ", " + PHOTO_IS_TEMP + "," + PHOTO_BURST_COVER_LEVEL + "," + MEDIA_TYPE + "," +
    MEDIA_DATE_ADDED + " DESC, " + PHOTO_THUMBNAIL_VISIBLE + ");";

const std::string PhotoColumn::CREATE_SCHPT_YEAR_COUNT_READY_INDEX = BaseColumn::CreateIndex() +
    PHOTO_SCHPT_DATE_YEAR_COUNT_READY_INDEX + " ON " + PHOTOS_TABLE + " (" + PHOTO_SYNC_STATUS + "," +
    PHOTO_CLEAN_FLAG + "," + MEDIA_DATE_TRASHED + "," + MEDIA_TIME_PENDING + "," + MEDIA_HIDDEN + "," + PHOTO_IS_TEMP +
    "," + PHOTO_BURST_COVER_LEVEL + "," + PHOTO_DATE_YEAR + " DESC, " + PHOTO_THUMBNAIL_VISIBLE + ");";

const std::string PhotoColumn::CREATE_SCHPT_MONTH_COUNT_READY_INDEX = BaseColumn::CreateIndex() +
    PHOTO_SCHPT_DATE_MONTH_COUNT_READY_INDEX + " ON " + PHOTOS_TABLE + " (" + PHOTO_SYNC_STATUS + "," +
    PHOTO_CLEAN_FLAG + "," + MEDIA_DATE_TRASHED + "," + MEDIA_TIME_PENDING + "," + MEDIA_HIDDEN + "," + PHOTO_IS_TEMP +
    "," + PHOTO_BURST_COVER_LEVEL + "," + PHOTO_DATE_MONTH + " DESC, " + PHOTO_THUMBNAIL_VISIBLE + ");";

const std::string PhotoColumn::CREATE_SCHPT_MEDIA_TYPE_COUNT_READY_INDEX = BaseColumn::CreateIndex() +
    PHOTO_SCHPT_MEDIA_TYPE_COUNT_READY_INDEX + " ON " + PHOTOS_TABLE + " (" + PHOTO_SYNC_STATUS + "," +
    PHOTO_CLEAN_FLAG + "," + MEDIA_DATE_TRASHED + "," + MEDIA_TIME_PENDING + "," + MEDIA_HIDDEN + ", " + PHOTO_IS_TEMP +
    "," + PHOTO_BURST_COVER_LEVEL + "," + MEDIA_TYPE + "," + PHOTO_THUMBNAIL_VISIBLE + ");";

const std::string PhotoColumn::CREATE_SCHPT_CLOUD_ENHANCEMENT_ALBUM_INDEX =
    BaseColumn::CreateIndex() + PHOTO_SCHPT_CLOUD_ENHANCEMENT_ALBUM_INDEX + " ON " + PHOTOS_TABLE +
    " (" + PHOTO_SYNC_STATUS + "," + PHOTO_CLEAN_FLAG + "," + MEDIA_HIDDEN + "," + MEDIA_TIME_PENDING +
    "," + MEDIA_DATE_TRASHED + "," + PHOTO_IS_TEMP + "," + PHOTO_STRONG_ASSOCIATION + "," + PHOTO_BURST_COVER_LEVEL +
    "," + MEDIA_DATE_TAKEN + " DESC);";

const std::string PhotoColumn::DROP_SCHPT_YEAR_COUNT_READY_INDEX = "DROP INDEX IF EXISTS " +
    PHOTO_SCHPT_DATE_YEAR_COUNT_READY_INDEX;

const std::string PhotoColumn::DROP_SCHPT_MONTH_COUNT_READY_INDEX = "DROP INDEX IF EXISTS " +
    PHOTO_SCHPT_DATE_MONTH_COUNT_READY_INDEX;

const std::string PhotoColumn::DROP_SCHPT_MEDIA_TYPE_COUNT_READY_INDEX = "DROP INDEX IF EXISTS " +
    PHOTO_SCHPT_MEDIA_TYPE_COUNT_READY_INDEX;

const std::string PhotoColumn::CREATE_HIDDEN_TIME_INDEX = BaseColumn::CreateIndex() +
    PHOTO_HIDDEN_TIME_INDEX + " ON " + PHOTOS_TABLE + " (" + PHOTO_HIDDEN_TIME + " DESC)";

const std::string PhotoColumn::CREATE_SCHPT_HIDDEN_TIME_INDEX =
    BaseColumn::CreateIndex() + PHOTO_SCHPT_HIDDEN_TIME_INDEX + " ON " + PHOTOS_TABLE +
    " (" + PHOTO_SYNC_STATUS + "," + PHOTO_CLEAN_FLAG + "," + MEDIA_HIDDEN + "," + MEDIA_TIME_PENDING +
    "," + MEDIA_DATE_TRASHED + "," + PHOTO_IS_TEMP + "," + PHOTO_BURST_COVER_LEVEL +
    "," + PHOTO_HIDDEN_TIME + " DESC, " + MEDIA_TYPE + "," + PHOTO_OWNER_ALBUM_ID + ");";

const std::string PhotoColumn::DROP_SCHPT_HIDDEN_TIME_INDEX = BaseColumn::DropIndex() + PHOTO_SCHPT_HIDDEN_TIME_INDEX;

const std::string PhotoColumn::CREATE_PHOTO_FAVORITE_INDEX =
    BaseColumn::CreateIndex() + PHOTO_FAVORITE_INDEX + " ON " + PHOTOS_TABLE +
    " (" + PHOTO_SYNC_STATUS + "," + PHOTO_CLEAN_FLAG + "," + MEDIA_HIDDEN + "," + MEDIA_TIME_PENDING +
    "," + MEDIA_DATE_TRASHED + "," + PHOTO_IS_TEMP + "," + MEDIA_IS_FAV + "," + PHOTO_BURST_COVER_LEVEL +
    "," + MEDIA_DATE_TAKEN + " DESC);";

const std::string PhotoColumn::DROP_PHOTO_FAVORITE_INDEX = BaseColumn::DropIndex() + PHOTO_FAVORITE_INDEX;

const std::string PhotoColumn::CREATE_PHOTO_DISPLAYNAME_INDEX = BaseColumn::CreateIndex() +
    PHOTO_DISPLAYNAME_INDEX + " ON " + PHOTOS_TABLE + " (" + MediaColumn::MEDIA_NAME + ")";

const std::string PhotoColumn::CREATE_PHOTO_BURSTKEY_INDEX = BaseColumn::CreateIndex() + PHOTO_BURSTKEY_INDEX +
    " ON " + PHOTOS_TABLE + " (" + PHOTO_BURST_KEY + "," + MEDIA_TIME_PENDING  + "," + MEDIA_NAME + " ASC);";

const std::string PhotoColumn::QUERY_MEDIA_VOLUME = "SELECT sum(" + MediaColumn::MEDIA_SIZE + ") AS " +
    MediaColumn::MEDIA_SIZE + "," +
    MediaColumn::MEDIA_TYPE + " FROM " +
    PhotoColumn::PHOTOS_TABLE + " WHERE " +
    "(" + MediaColumn::MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_IMAGE) + " OR " +
    MediaColumn::MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_VIDEO) + ") AND " +
    PhotoColumn::PHOTO_POSITION + " != 2" + " GROUP BY " +
    MediaColumn::MEDIA_TYPE;

// Create indexes
const std::string PhotoColumn::INDEX_SCTHP_ADDTIME =
    BaseColumn::CreateIndex() + PHOTO_SCHPT_ADDED_INDEX + " ON " + PHOTOS_TABLE +
    " (" + PHOTO_SYNC_STATUS + "," + PHOTO_CLEAN_FLAG + "," + MEDIA_DATE_TRASHED + "," + MEDIA_HIDDEN + "," +
    MEDIA_TIME_PENDING + "," + PHOTO_IS_TEMP + "," + PHOTO_BURST_COVER_LEVEL + "," + MEDIA_DATE_TAKEN + " DESC, " +
    MEDIA_ID + " DESC);";

const std::string PhotoColumn::INDEX_SCHPT_ALBUM_GENERAL =
    BaseColumn::CreateIndex() + PHOTO_SCHPT_ALBUM_GENERAL_INDEX + " ON " + PHOTOS_TABLE +
    " (" + PHOTO_SYNC_STATUS + "," + PHOTO_CLEAN_FLAG + "," + MEDIA_DATE_TRASHED + "," +
    MEDIA_HIDDEN + "," + MEDIA_TIME_PENDING + "," + PHOTO_IS_TEMP + "," + PHOTO_BURST_COVER_LEVEL +
    "," + PHOTO_OWNER_ALBUM_ID + ");";

const std::string PhotoColumn::INDEX_SCHPT_ALBUM =
    BaseColumn::CreateIndex() + PHOTO_SCHPT_ALBUM_INDEX + " ON " + PHOTOS_TABLE +
    " (" + PHOTO_SYNC_STATUS + "," + PHOTO_CLEAN_FLAG + "," + MEDIA_DATE_TRASHED + "," + MEDIA_HIDDEN + "," +
    MEDIA_TIME_PENDING + "," + PHOTO_IS_TEMP + "," + PHOTO_BURST_COVER_LEVEL + "," + PHOTO_OWNER_ALBUM_ID + "," +
    MEDIA_DATE_TAKEN + " DESC, " + MEDIA_ID + " DESC);";

// Create dateadded index
const std::string PhotoColumn::INDEX_SCTHP_PHOTO_DATEADDED =
    BaseColumn::CreateIndex() + PHOTO_SCHPT_PHOTO_DATEADDED_INDEX + " ON " + PHOTOS_TABLE +
    " (" + PHOTO_SYNC_STATUS + "," + PHOTO_CLEAN_FLAG + "," + MEDIA_DATE_TRASHED + "," + MEDIA_HIDDEN + "," +
    MEDIA_TIME_PENDING + "," + PHOTO_IS_TEMP + "," + PHOTO_BURST_COVER_LEVEL + "," + MEDIA_DATE_ADDED + " DESC, " +
    PHOTO_THUMBNAIL_VISIBLE + ");";

const std::string PhotoColumn::INDEX_LATITUDE =
    BaseColumn::CreateIndex() + LATITUDE_INDEX + " ON " + PHOTOS_TABLE + " (" + PHOTO_LATITUDE + ");";

const std::string PhotoColumn::INDEX_LONGITUDE =
    BaseColumn::CreateIndex() + LONGITUDE_INDEX + " ON " + PHOTOS_TABLE + " (" + PHOTO_LONGITUDE + ");";

const std::string PhotoColumn::UPDATE_LATITUDE_AND_LONGITUDE_DEFAULT_NULL =
    " UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_LATITUDE +
    " = NULL, " + PhotoColumn::PHOTO_LONGITUDE + " = NULL " + " WHERE " +
    PhotoColumn::PHOTO_LATITUDE + " = 0 AND " + PhotoColumn::PHOTO_LONGITUDE + " = 0;";

const std::string PhotoColumn::INDEX_QUERY_THUMBNAIL_WHITE_BLOCKS =
    BaseColumn::CreateIndex() + PHOTO_QUERY_THUMBNAIL_WHITE_BLOCKS_INDEX + " ON " + PHOTOS_TABLE +
    " (" + MEDIA_TYPE + "," + PHOTO_SYNC_STATUS + "," + PHOTO_LCD_VISIT_TIME + "," + PHOTO_POSITION + "," +
    PHOTO_THUMB_STATUS + "," + PHOTO_CLEAN_FLAG + "," + PHOTO_THUMBNAIL_VISIBLE + " );";

const std::string PhotoColumn::UPDATE_PHOTO_QUALITY_OF_NULL_PHOTO_ID =
    " UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_QUALITY + " = 0 WHERE " +
    PhotoColumn::PHOTO_QUALITY + " = 1 AND " + PhotoColumn::PHOTO_ID + " IS NULL;";

const std::string PhotoColumn::DROP_INDEX_SCTHP_ADDTIME = BaseColumn::DropIndex() + PHOTO_SCHPT_ADDED_INDEX;

const std::string PhotoColumn::DROP_INDEX_SCHPT_ADDTIME_ALBUM = BaseColumn::DropIndex() + PHOTO_SCHPT_ADDED_ALBUM_INDEX;

const std::string PhotoColumn::INDEX_CAMERA_SHOT_KEY =
    BaseColumn::CreateIndex() + "idx_camera_shot_key" + " ON " + PHOTOS_TABLE +
    " (" + CAMERA_SHOT_KEY + ");";

const std::string PhotoColumn::INDEX_SCHPT_READY = BaseColumn::CreateIndex() + PHOTO_SCHPT_READY_INDEX + " ON " +
    PHOTOS_TABLE + " (" + PHOTO_SYNC_STATUS + "," + PHOTO_CLEAN_FLAG + "," + MEDIA_DATE_TRASHED + "," +
    MEDIA_TIME_PENDING + ", " + MEDIA_HIDDEN + "," + PHOTO_IS_TEMP + "," + PHOTO_BURST_COVER_LEVEL + "," +
    MEDIA_DATE_TAKEN + " DESC, " + MEDIA_ID + " DESC, " + PHOTO_THUMBNAIL_VISIBLE + ");";

const std::string PhotoColumn::DROP_INDEX_SCHPT_READY = BaseColumn::DropIndex() + PHOTO_SCHPT_READY_INDEX;

const std::string PhotoColumn::CREATE_PHOTO_SORT_MEDIA_TYPE_DATE_ADDED_INDEX = BaseColumn::CreateIndex() +
    PHOTO_SORT_MEDIA_TYPE_DATE_ADDED_INDEX + " ON " + PHOTOS_TABLE +
    " (" + PHOTO_SYNC_STATUS + "," + PHOTO_CLEAN_FLAG + "," + MEDIA_DATE_TRASHED + "," + MEDIA_HIDDEN +
    "," + MEDIA_TIME_PENDING + "," + PHOTO_IS_TEMP + "," + PHOTO_BURST_COVER_LEVEL + "," + MEDIA_TYPE +
    "," + MEDIA_DATE_ADDED + " DESC," + MEDIA_NAME + " DESC, " + PHOTO_STRONG_ASSOCIATION + ");";

const std::string PhotoColumn::CREATE_PHOTO_SORT_MEDIA_TYPE_DATE_TAKEN_INDEX = BaseColumn::CreateIndex() +
    PHOTO_SORT_MEDIA_TYPE_DATE_TAKEN_INDEX + " ON " + PHOTOS_TABLE +
    " (" + PHOTO_SYNC_STATUS + "," + PHOTO_CLEAN_FLAG + "," + MEDIA_DATE_TRASHED + "," + MEDIA_HIDDEN +
    "," + MEDIA_TIME_PENDING + "," + PHOTO_IS_TEMP + "," + PHOTO_BURST_COVER_LEVEL + "," + MEDIA_TYPE + "," +
    MEDIA_DATE_TAKEN + " DESC," + MEDIA_NAME + " DESC, " + PHOTO_STRONG_ASSOCIATION + ");";

const std::string PhotoColumn::CREATE_PHOTO_SORT_IN_ALBUM_DATE_ADDED_INDEX = BaseColumn::CreateIndex() +
    PHOTO_SORT_IN_ALBUM_DATE_ADDED_INDEX + " ON " + PHOTOS_TABLE +
    " (" + PHOTO_OWNER_ALBUM_ID + "," + MEDIA_HIDDEN + "," + PHOTO_CLEAN_FLAG + "," +  PHOTO_SYNC_STATUS + "," +
    MEDIA_DATE_TRASHED + "," + MEDIA_TIME_PENDING + "," + PHOTO_IS_TEMP + "," + PHOTO_BURST_COVER_LEVEL + "," +
    MEDIA_DATE_ADDED + " DESC," + MEDIA_NAME + " DESC);";

const std::string PhotoColumn::CREATE_PHOTO_SORT_IN_ALBUM_DATE_TAKEN_INDEX = BaseColumn::CreateIndex() +
    PHOTO_SORT_IN_ALBUM_DATE_TAKEN_INDEX + " ON " + PHOTOS_TABLE +
    " (" + PHOTO_SYNC_STATUS + "," + PHOTO_CLEAN_FLAG + "," + MEDIA_DATE_TRASHED + "," + MEDIA_HIDDEN +
    "," + MEDIA_TIME_PENDING + "," + PHOTO_IS_TEMP + "," + PHOTO_BURST_COVER_LEVEL + "," + PHOTO_OWNER_ALBUM_ID +
    "," + MEDIA_DATE_TAKEN + " DESC," + MEDIA_NAME + " DESC);";

const std::string PhotoColumn::CREATE_PHOTO_SORT_IN_ALBUM_SIZE_INDEX = BaseColumn::CreateIndex() +
    PHOTO_SORT_IN_ALBUM_SIZE_INDEX + " ON " + PHOTOS_TABLE +
    " (" + PHOTO_SYNC_STATUS + "," + PHOTO_CLEAN_FLAG + "," + MEDIA_DATE_TRASHED + "," + MEDIA_HIDDEN +
    "," + MEDIA_TIME_PENDING + ", " + PHOTO_IS_TEMP + "," + PHOTO_BURST_COVER_LEVEL + "," + PHOTO_OWNER_ALBUM_ID +
    "," + MEDIA_SIZE + " DESC," + MEDIA_ID + " DESC);";

const std::string PhotoColumn::CREATE_PHOTO_SORT_MEDIA_TYPE_SIZE_INDEX = BaseColumn::CreateIndex() +
    PHOTO_SORT_MEDIA_TYPE_SIZE_INDEX + " ON " + PHOTOS_TABLE +
    " (" + PHOTO_SYNC_STATUS + "," + PHOTO_CLEAN_FLAG + "," + MEDIA_DATE_TRASHED + "," + MEDIA_HIDDEN +
    "," + MEDIA_TIME_PENDING + ", " + PHOTO_IS_TEMP + "," + PHOTO_BURST_COVER_LEVEL + "," + MEDIA_TYPE +
    "," + MEDIA_SIZE + " DESC," + MEDIA_ID + " DESC," + PHOTO_STRONG_ASSOCIATION + ");";

const std::string PhotoColumn::CREATE_PHOTO_SORT_IN_ALBUM_DISPLAY_NAME_INDEX = BaseColumn::CreateIndex() +
    PHOTO_SORT_IN_ALBUM_DISPLAY_NAME_INDEX + " ON " + PHOTOS_TABLE +
    " (" + PHOTO_SYNC_STATUS + "," + PHOTO_CLEAN_FLAG + "," + MEDIA_DATE_TRASHED + "," + MEDIA_HIDDEN +
    "," + MEDIA_TIME_PENDING + ", " + PHOTO_IS_TEMP + "," + PHOTO_BURST_COVER_LEVEL + "," + PHOTO_OWNER_ALBUM_ID +
    "," + MEDIA_NAME + " DESC);";

const std::string PhotoColumn::CREATE_PHOTO_SORT_MEDIA_TYPE_DISPLAY_NAME_INDEX = BaseColumn::CreateIndex() +
    PHOTO_SORT_MEDIA_TYPE_DISPLAY_NAME_INDEX + " ON " + PHOTOS_TABLE +
    " (" + PHOTO_SYNC_STATUS + "," + PHOTO_CLEAN_FLAG + "," + MEDIA_DATE_TRASHED + "," + MEDIA_HIDDEN +
    "," + MEDIA_TIME_PENDING + ", " + PHOTO_IS_TEMP + "," + PHOTO_BURST_COVER_LEVEL + "," + MEDIA_TYPE +
    "," + MEDIA_NAME + " DESC," + PHOTO_STRONG_ASSOCIATION + ");";

const std::string PhotoColumn::CREATE_PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX = BaseColumn::CreateIndex() +
    PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX + " ON " + PHOTOS_TABLE +
    " (" + PHOTO_SYNC_STATUS + "," + PHOTO_CLEAN_FLAG + "," + MEDIA_DATE_TRASHED + "," + MEDIA_HIDDEN +
    "," + MEDIA_TIME_PENDING + ", " + PHOTO_IS_TEMP + "," + PHOTO_BURST_COVER_LEVEL + "," + PHOTO_SHOOTING_MODE +
    "," + MEDIA_DATE_TAKEN + " DESC," + MEDIA_NAME + " DESC);";

const std::string PhotoColumn::CREATE_PHOTO_BURST_MODE_ALBUM_INDEX = BaseColumn::CreateIndex() +
    PHOTO_BURST_MODE_ALBUM_INDEX + " ON " + PHOTOS_TABLE +
    " (" + PHOTO_SYNC_STATUS + "," + PHOTO_CLEAN_FLAG + "," + MEDIA_DATE_TRASHED + "," + MEDIA_HIDDEN +
    "," + MEDIA_TIME_PENDING + ", " + PHOTO_IS_TEMP + "," + PHOTO_BURST_COVER_LEVEL + "," + PHOTO_SUBTYPE +
    "," + MEDIA_DATE_TAKEN + " DESC," + MEDIA_NAME + " DESC," + PHOTO_BURST_KEY + ");";

const std::string PhotoColumn::CREATE_PHOTO_FRONT_CAMERA_ALBUM_INDEX = BaseColumn::CreateIndex() +
    PHOTO_FRONT_CAMERA_ALBUM_INDEX + " ON " + PHOTOS_TABLE +
    " (" + PHOTO_SYNC_STATUS + "," + PHOTO_CLEAN_FLAG + "," + MEDIA_DATE_TRASHED + "," + MEDIA_HIDDEN +
    "," + MEDIA_TIME_PENDING + ", " + PHOTO_IS_TEMP + "," + PHOTO_BURST_COVER_LEVEL + "," + PHOTO_FRONT_CAMERA +
    "," + MEDIA_DATE_TAKEN + " DESC," + MEDIA_NAME + " DESC);";

const std::string PhotoColumn::CREATE_PHOTO_RAW_IMAGE_ALBUM_INDEX = BaseColumn::CreateIndex() +
    PHOTO_RAW_IMAGE_ALBUM_INDEX + " ON " + PHOTOS_TABLE +
    " (" + PHOTO_SYNC_STATUS + "," + PHOTO_CLEAN_FLAG + "," + MEDIA_DATE_TRASHED + "," + MEDIA_HIDDEN +
    "," + MEDIA_TIME_PENDING + ", " + PHOTO_IS_TEMP + "," + PHOTO_BURST_COVER_LEVEL + "," + MEDIA_MIME_TYPE +
    "," + MEDIA_DATE_TAKEN + " DESC," + MEDIA_NAME + " DESC);";

const std::string PhotoColumn::CREATE_PHOTO_MOVING_PHOTO_ALBUM_INDEX = BaseColumn::CreateIndex() +
    PHOTO_MOVING_PHOTO_ALBUM_INDEX + " ON " + PHOTOS_TABLE +
    " (" + PHOTO_SYNC_STATUS + "," + PHOTO_CLEAN_FLAG + "," + MEDIA_DATE_TRASHED + "," + MEDIA_HIDDEN +
    "," + MEDIA_TIME_PENDING + ", " + PHOTO_IS_TEMP + "," + PHOTO_BURST_COVER_LEVEL +
    "," + MEDIA_DATE_TAKEN + " DESC," + MEDIA_NAME + " DESC) "
    "WHERE (subtype = " + to_string(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) +
        " OR (moving_photo_effect_mode = " + to_string(static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY)) +
            " AND subtype = " + to_string(static_cast<int32_t>(PhotoSubType::DEFAULT)) + "));";

const std::string PhotoColumn::CREATE_PHOTOS_DELETE_TRIGGER =
                        "CREATE TRIGGER IF NOT EXISTS photos_delete_trigger AFTER UPDATE ON " +
                        PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW WHEN new." + PhotoColumn::PHOTO_DIRTY +
                        " = " + std::to_string(static_cast<int32_t>(DirtyTypes::TYPE_DELETED)) +
                        " AND OLD." + PhotoColumn::PHOTO_POSITION + " = 1 AND is_caller_self_func() = 'true'" +
                        " AND OLD." + PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " <> " +
                        to_string(static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) +
                        " BEGIN DELETE FROM " + PhotoColumn::PHOTOS_TABLE +
                        " WHERE " + PhotoColumn::MEDIA_ID + " = old." + PhotoColumn::MEDIA_ID + ";" +
                        " END;";

const std::string PhotoColumn::CREATE_PHOTOS_FDIRTY_TRIGGER =
                        "CREATE TRIGGER IF NOT EXISTS photos_fdirty_trigger AFTER UPDATE ON " +
                        PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW WHEN OLD.position <> 1 AND" +
                        " new.date_modified <> old.date_modified " +
                        " AND new.dirty = old.dirty AND is_caller_self_func() = 'true'" +
                        " AND OLD." + PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " <> " +
                        to_string(static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) +
                        " BEGIN " +
                        " UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET dirty = " +
                        std::to_string(static_cast<int32_t>(DirtyTypes::TYPE_FDIRTY)) +
                        " WHERE file_id = old.file_id;" +
                        " SELECT cloud_sync_func(); " +
                        " END;";

const std::string PhotoColumn::CREATE_PHOTOS_MDIRTY_TRIGGER =
                        "CREATE TRIGGER IF NOT EXISTS photos_mdirty_trigger AFTER UPDATE ON " +
                        PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW WHEN OLD.position <> 1" +
                        " AND new.date_modified = old.date_modified AND ( old.dirty = " +
                        std::to_string(static_cast<int32_t>(DirtyTypes::TYPE_SYNCED)) + " OR old.dirty =" +
                        std::to_string(static_cast<int32_t>(DirtyTypes::TYPE_SDIRTY)) + " OR old.dirty =" +
                        std::to_string(static_cast<int32_t>(DirtyTypes::TYPE_TDIRTY)) +
                        ") AND new.dirty = old.dirty AND is_caller_self_func() = 'true'" +
                        " AND " + PhotoColumn::CheckUploadPhotoColumns() +
                        " AND OLD." + PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " <> " +
                        to_string(static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) +
                        " BEGIN " +
                        " UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET dirty = " +
                        std::to_string(static_cast<int32_t>(DirtyTypes::TYPE_MDIRTY)) +
                        " WHERE file_id = old.file_id;" +
                        " SELECT cloud_sync_func(); " +
                        " END;";

const std::string PhotoColumn::INSERT_GENERATE_HIGHLIGHT_THUMBNAIL =
                        "CREATE TRIGGER IF NOT EXISTS insert_generate_highlight_thumbnail_trigger AFTER INSERT ON " +
                        PhotoColumn::HIGHLIGHT_TABLE + " BEGIN SELECT begin_generate_highlight_thumbnail " +
                        "(NEW." + MEDIA_DATA_DB_ID + ", NEW." + MEDIA_DATA_DB_VIDEO_TRACKS +
                        ", NEW." + MEDIA_DATA_DB_HIGHLIGHT_TRIGGER + ", '" + MEDIA_DATA_DB_INSERT_TYPE + "'); END;";

const std::string PhotoColumn::UPDATE_GENERATE_HIGHLIGHT_THUMBNAIL =
                        "CREATE TRIGGER IF NOT EXISTS update_generate_highlight_thumbnail_trigger AFTER UPDATE ON " +
                        PhotoColumn::HIGHLIGHT_TABLE + " FOR EACH ROW " + " WHEN OLD." +
                        MEDIA_DATA_DB_HIGHLIGHT_TRIGGER + "= 1 " + "AND NEW." + MEDIA_DATA_DB_HIGHLIGHT_TRIGGER +
                        "= 0 BEGIN SELECT begin_generate_highlight_thumbnail " +
                        "(NEW." + MEDIA_DATA_DB_ID + ", NEW." + MEDIA_DATA_DB_VIDEO_TRACKS +
                        ", NEW." + MEDIA_DATA_DB_HIGHLIGHT_TRIGGER + ", '" + MEDIA_DATA_DB_UPDATE_TYPE + "'); END;";

const std::string PhotoColumn::INDEX_HIGHLIGHT_FILEID =
                        BaseColumn::CreateIndex() + MEDIA_DATA_DB_HIGHLIGHT_INDEX + " ON " +
                        HIGHLIGHT_TABLE + " (" + MEDIA_ID + ");";

const std::string  PhotoColumn::CREATE_PHOTOS_INSERT_CLOUD_SYNC =
                        " CREATE TRIGGER IF NOT EXISTS photo_insert_cloud_sync_trigger AFTER INSERT ON " +
                        PhotoColumn::PHOTOS_TABLE +
                        " WHEN new." + PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " <> " +
                        to_string(static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) +
                        " BEGIN SELECT cloud_sync_func(); END;";

const std::string PhotoColumn::CREATE_PHOTOS_UPDATE_CLOUD_SYNC =
                        " CREATE TRIGGER IF NOT EXISTS photo_update_cloud_sync_trigger AFTER UPDATE ON " +
                        PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW WHEN OLD.dirty IN (1,2,3,5) AND new.dirty != " +
                        std::to_string(static_cast<int32_t>(DirtyTypes::TYPE_SYNCED)) +
                        " AND OLD." + PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " <> " +
                        to_string(static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) +
                        " BEGIN SELECT cloud_sync_func(); END;";

const std::string PhotoColumn::UPDATE_READY_ON_THUMBNAIL_UPGRADE =
                        " UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_THUMBNAIL_READY +
                        " = 6 " + " WHERE " + PhotoColumn::PHOTO_THUMBNAIL_READY + " != 0; END;";

const std::string PhotoColumn::CREATE_PHOTOS_METADATA_DIRTY_TRIGGER =
                        "CREATE TRIGGER IF NOT EXISTS photos_metadata_dirty_trigger AFTER UPDATE ON " +
                        PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW WHEN old." + PhotoColumn::PHOTO_POSITION + " != 2" +
                        " AND old.metadata_flags = " +
                        std::to_string(static_cast<int32_t>(MetadataFlags::TYPE_UPTODATE)) +
                        " AND new.metadata_flags = old.metadata_flags" +
                        " AND " + PhotoColumn::CheckMetaRecoveryPhotoColumns() +
                        " AND OLD." + PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " <> " +
                        to_string(static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) +
                        " BEGIN " +
                        " UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET metadata_flags = " +
                        std::to_string(static_cast<int32_t>(MetadataFlags::TYPE_DIRTY)) +
                        " WHERE file_id = old.file_id;" +
                        " END;";

const std::string PhotoColumn::UPDATA_PHOTOS_DATA_UNIQUE = "CREATE UNIQUE INDEX IF NOT EXISTS photo_data_index ON " +
    PhotoColumn::PHOTOS_TABLE + " (" + MEDIA_FILE_PATH + ");";

const std::string PhotoColumn::UPDATE_LCD_STATUS_NOT_UPLOADED =
                        " UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_LCD_VISIT_TIME +
                        " = 0 " + " WHERE " + PhotoColumn::PHOTO_DIRTY + " = 1; END;";

const std::set<std::string> PhotoColumn::PHOTO_COLUMNS = {
    PhotoColumn::PHOTO_ORIENTATION, PhotoColumn::PHOTO_LATITUDE, PhotoColumn::PHOTO_LONGITUDE,
    PhotoColumn::PHOTO_HEIGHT, PhotoColumn::PHOTO_WIDTH, PhotoColumn::PHOTO_LCD_VISIT_TIME, PhotoColumn::PHOTO_POSITION,
    PhotoColumn::PHOTO_DIRTY, PhotoColumn::PHOTO_CLOUD_ID, PhotoColumn::CAMERA_SHOT_KEY, PhotoColumn::PHOTO_ALL_EXIF,
    PhotoColumn::PHOTO_USER_COMMENT, PhotoColumn::PHOTO_DATE_YEAR, PhotoColumn::PHOTO_DATE_MONTH,
    PhotoColumn::PHOTO_DATE_DAY, PhotoColumn::PHOTO_EDIT_TIME, PhotoColumn::PHOTO_CLEAN_FLAG,
    PhotoColumn::PHOTO_SHOOTING_MODE, PhotoColumn::PHOTO_SHOOTING_MODE_TAG, PhotoColumn::PHOTO_THUMB_STATUS,
    PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, PhotoColumn::PHOTO_LCD_SIZE,
    PhotoColumn::PHOTO_THUMB_SIZE, PhotoColumn::MOVING_PHOTO_EFFECT_MODE, PhotoColumn::PHOTO_FRONT_CAMERA,
    PhotoColumn::PHOTO_BURST_COVER_LEVEL, PhotoColumn::PHOTO_BURST_KEY, PhotoColumn::PHOTO_COVER_POSITION,
    PhotoColumn::PHOTO_THUMBNAIL_READY, PhotoColumn::PHOTO_ORIGINAL_SUBTYPE, PhotoColumn::PHOTO_DETAIL_TIME,
    PhotoColumn::PHOTO_CE_AVAILABLE, PhotoColumn::PHOTO_OWNER_ALBUM_ID, PhotoColumn::SUPPORTED_WATERMARK_TYPE,
    PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, PhotoColumn::PHOTO_QUALITY, PhotoColumn::PHOTO_IS_AUTO,
    PhotoColumn::PHOTO_MEDIA_SUFFIX, PhotoColumn::PHOTO_IS_RECENT_SHOW, PhotoColumn::PHOTO_IS_RECTIFICATION_COVER,
    PhotoColumn::PHOTO_EXIF_ROTATE, PhotoColumn::PHOTO_HAS_APPLINK, PhotoColumn::PHOTO_APPLINK,
    PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE, PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS,
    PhotoColumn::PHOTO_HDR_MODE,
    PhotoColumn::PHOTO_STORAGE_PATH, PhotoColumn::PHOTO_FILE_SOURCE_TYPE, PhotoColumn::PHOTO_ASPECT_RATIO,
    PhotoColumn::PHOTO_CHANGE_TIME, PhotoColumn::PHOTO_IS_CRITICAL, PhotoColumn::PHOTO_CRITICAL_TYPE,
};

bool PhotoColumn::IsPhotoColumn(const std::string &columnName)
{
    if (columnName == MEDIA_COLUMN_COUNT) {
        return true;
    }
    return (PHOTO_COLUMNS.find(columnName) != PHOTO_COLUMNS.end()) ||
        (MEDIA_COLUMNS.find(columnName) != MEDIA_COLUMNS.end());
}

std::string PhotoColumn::CheckUploadPhotoColumns()
{
    // Since date_modified has been checked in mdirty and fdirty, omit it here.
    const std::vector<std::string> uploadPhotoColumns = {
        MEDIA_FILE_PATH, MEDIA_SIZE, MEDIA_NAME, MEDIA_TYPE, MEDIA_MIME_TYPE, MEDIA_OWNER_PACKAGE,
        MEDIA_OWNER_APPID, MEDIA_DEVICE_NAME, MEDIA_DATE_ADDED, MEDIA_DATE_TAKEN, MEDIA_DURATION,
        MEDIA_IS_FAV, MEDIA_DATE_TRASHED, MEDIA_DATE_DELETED, MEDIA_HIDDEN, PHOTO_META_DATE_MODIFIED,
        PHOTO_ORIENTATION, PHOTO_LATITUDE, PHOTO_LONGITUDE, PHOTO_HEIGHT, PHOTO_WIDTH, PHOTO_SUBTYPE,
        PHOTO_USER_COMMENT, PHOTO_DATE_YEAR, PHOTO_DATE_MONTH, PHOTO_DATE_DAY, PHOTO_DETAIL_TIME,
        PHOTO_SHOOTING_MODE, PHOTO_SHOOTING_MODE_TAG, MOVING_PHOTO_EFFECT_MODE, PHOTO_COVER_POSITION,
        PHOTO_ORIGINAL_SUBTYPE, PHOTO_OWNER_ALBUM_ID, PHOTO_SOURCE_PATH, PHOTO_IS_RECTIFICATION_COVER,
        PHOTO_STRONG_ASSOCIATION, PHOTO_HDR_MODE,
    };

    std::string result = "(";
    size_t size = uploadPhotoColumns.size();
    for (size_t i = 0; i < size; i++) {
        std::string column = uploadPhotoColumns[i];
        if (i != size - 1) {
            result += "new." + column + " <> old." + column + " OR ";
        } else {
            result += "new." + column + " <> old." + column + ")";
        }
    }
    return result;
}

std::string PhotoColumn::CheckMetaRecoveryPhotoColumns()
{
    const std::vector<std::string> metaDirtyPhotoColumns = {
        MEDIA_DATE_MODIFIED, MEDIA_FILE_PATH, MEDIA_SIZE, MEDIA_TYPE, MEDIA_MIME_TYPE, MEDIA_OWNER_PACKAGE,
        MEDIA_OWNER_APPID, MEDIA_DEVICE_NAME, MEDIA_DATE_ADDED, MEDIA_DATE_TAKEN, MEDIA_DURATION, MEDIA_IS_FAV,
        MEDIA_DATE_TRASHED, MEDIA_DATE_DELETED, MEDIA_HIDDEN, PHOTO_META_DATE_MODIFIED, PHOTO_ORIENTATION,
        PHOTO_LATITUDE, PHOTO_LONGITUDE, PHOTO_HEIGHT, PHOTO_WIDTH, PHOTO_SUBTYPE, PHOTO_USER_COMMENT,
        PHOTO_DATE_YEAR, PHOTO_DATE_MONTH, PHOTO_DATE_DAY, PHOTO_SHOOTING_MODE, PHOTO_SHOOTING_MODE_TAG,
        PHOTO_OWNER_ALBUM_ID, PHOTO_SOURCE_PATH, MOVING_PHOTO_EFFECT_MODE, PHOTO_COVER_POSITION, PHOTO_ORIGINAL_SUBTYPE,
        PHOTO_CLOUD_ID, PHOTO_POSITION, PHOTO_DIRTY, PHOTO_SYNC_STATUS, PHOTO_CLOUD_VERSION, PHOTO_CLEAN_FLAG,
    };

    std::string result = "(";
    size_t size = metaDirtyPhotoColumns.size();
    for (size_t i = 0; i < size; i++) {
        std::string column = metaDirtyPhotoColumns[i];
        if (i != size - 1) {
            result += "new." + column + " <> old." + column + " OR ";
        } else {
            result += "new." + column + " <> old." + column + ")";
        }
    }
    return result;
}

const std::string AudioColumn::AUDIO_ALBUM = "audio_album";
const std::string AudioColumn::AUDIO_ARTIST = "artist";
const std::string AudioColumn::AUDIO_FILE_SOURCE_TYPE = "file_source_type";
const std::string AudioColumn::AUDIO_IS_TEMP = "is_temp";

const std::string AudioColumn::AUDIOS_TABLE = "Audios";

const std::string AudioColumn::AUDIO_URI_PREFIX = "file://media/Audio/";
const std::string AudioColumn::DEFAULT_AUDIO_URI = "file://media/Audio";
const std::string AudioColumn::AUDIO_TYPE_URI = "/Audio";

const std::string AudioColumn::CREATE_AUDIO_TABLE = "CREATE TABLE IF NOT EXISTS " +
    AUDIOS_TABLE + " (" +
    MEDIA_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    MEDIA_FILE_PATH + " TEXT, " +
    MEDIA_SIZE + " BIGINT, " +
    MEDIA_TITLE + " TEXT, " +
    MEDIA_NAME + " TEXT, " +
    MEDIA_TYPE + " INT, " +
    MEDIA_MIME_TYPE + " TEXT, " +
    MEDIA_OWNER_PACKAGE + " TEXT, " +
    MEDIA_OWNER_APPID + " TEXT, " +
    MEDIA_PACKAGE_NAME + " TEXT, " +
    MEDIA_DEVICE_NAME + " TEXT, " +
    AUDIO_ARTIST + " TEXT, " +
    AUDIO_FILE_SOURCE_TYPE + " INT NOT NULL DEFAULT 0, " +
    AUDIO_IS_TEMP + " INT DEFAULT 0," +
    MEDIA_DATE_ADDED + " BIGINT, " +
    MEDIA_DATE_MODIFIED + " BIGINT, " +
    MEDIA_DATE_TAKEN + " BIGINT DEFAULT 0, " +
    MEDIA_DURATION + " INT, " +
    MEDIA_TIME_PENDING + " BIGINT DEFAULT 0, " +
    MEDIA_IS_FAV + " INT DEFAULT 0, " +
    MEDIA_DATE_TRASHED + " BIGINT DEFAULT 0, " +
    MEDIA_DATE_DELETED + " BIGINT DEFAULT 0, " +
    MEDIA_PARENT_ID + " INT DEFAULT 0, " +
    MEDIA_RELATIVE_PATH + " TEXT, " +
    MEDIA_VIRTURL_PATH + " TEXT UNIQUE, " +
    AUDIO_ALBUM + " TEXT)";

const std::string AudioColumn::QUERY_MEDIA_VOLUME = "SELECT sum(" + MediaColumn::MEDIA_SIZE + ") AS " +
    MediaColumn::MEDIA_SIZE + "," +
    MediaColumn::MEDIA_TYPE + " FROM " +
    AudioColumn::AUDIOS_TABLE + " WHERE " +
    MediaColumn::MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_AUDIO) + " GROUP BY " +
    MediaColumn::MEDIA_TYPE;

const std::set<std::string> AudioColumn::AUDIO_COLUMNS = {
    AudioColumn::AUDIO_ALBUM, AudioColumn::AUDIO_ARTIST, AudioColumn::AUDIO_FILE_SOURCE_TYPE, AudioColumn::AUDIO_IS_TEMP
};

bool AudioColumn::IsAudioColumn(const std::string &columnName)
{
    return (AUDIO_COLUMNS.find(columnName) != AUDIO_COLUMNS.end()) ||
        (MEDIA_COLUMNS.find(columnName) != MEDIA_COLUMNS.end());
}

const std::string MediaColumn::ASSETS_QUERY_FILTER =
    PhotoColumn::PHOTO_SYNC_STATUS + " = 0" + " AND " +
    MediaColumn::MEDIA_DATE_TRASHED + " = 0" + " AND " +
    MediaColumn::MEDIA_HIDDEN + " = 0" + " AND " +
    MediaColumn::MEDIA_TIME_PENDING + " = 0 ";

const std::string PhotoExtColumn::PHOTOS_EXT_TABLE = "tab_photos_ext";

const std::string PhotoExtColumn::PHOTO_ID = "photo_id";
const std::string PhotoExtColumn::THUMBNAIL_SIZE = "thumbnail_size";
const std::string PhotoExtColumn::EDITDATA_SIZE = "editdata_size";

const std::string PhotoExtColumn::CREATE_PHOTO_EXT_TABLE =
    "CREATE TABLE IF NOT EXISTS " +
    PHOTOS_EXT_TABLE + " (" +
    PHOTO_ID + " INTEGER PRIMARY KEY, " +
    THUMBNAIL_SIZE + " BIGINT DEFAULT 0)";

// For Photos table query filter
const std::string PhotoColumn::PHOTOS_QUERY_FILTER =
    MediaColumn::MEDIA_DATE_TRASHED + " = 0" + " AND " +
    MediaColumn::MEDIA_HIDDEN + " = 0" + " AND " +
    MediaColumn::MEDIA_TIME_PENDING + " = 0" + " AND " +
    PhotoColumn::PHOTO_IS_TEMP + " = 0" + " AND " +
    PhotoColumn::PHOTO_BURST_COVER_LEVEL + " = 1 ";

}  // namespace Media
}  // namespace OHOS
