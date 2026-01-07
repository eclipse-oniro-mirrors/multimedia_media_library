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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_COLUMN_H_
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_COLUMN_H_

#include <set>
#include <string>

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
enum class DirtyTypes : int32_t {
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

enum class MetadataFlags : int32_t {
    TYPE_NEW,
    TYPE_DIRTY,
    TYPE_UPTODATE,
    TYPE_RECOVERYING,
};

enum class ExtraChangeType : uint32_t {
    PHOTO_TIME_UPDATE = 100
};

enum class FileSourceTypes : int32_t {
    MEDIA,
    FILE_MANAGER,
    PERIPHERAL,
    MEDIA_HO_LAKE,
    TEMP_FILE_MANAGER,
};

enum class SouthDeviceType : int32_t {
    SOUTH_DEVICE_NULL = 0,
    SOUTH_DEVICE_CLOUD = 1,
    SOUTH_DEVICE_HDC = 2
};

enum CriticalType : int32_t {
    UNKNOWN_CRITICAL_TYPE = 0,
    NOT_CRITICAL_TYPE = 1,
    SUSPECTED_CRITICAL_TYPE = 2,
    CRITICAL_TYPE = 3,
};

class MediaColumn {
public:
    // Asset Base Parameter
    static const std::string MEDIA_ID EXPORT;
    static const std::string MEDIA_FILE_PATH EXPORT;
    static const std::string MEDIA_SIZE EXPORT;
    static const std::string MEDIA_TITLE EXPORT;
    static const std::string MEDIA_NAME EXPORT;
    static const std::string MEDIA_TYPE EXPORT;
    static const std::string MEDIA_MIME_TYPE EXPORT;
    static const std::string MEDIA_OWNER_PACKAGE EXPORT;
    static const std::string MEDIA_OWNER_APPID EXPORT;
    static const std::string MEDIA_PACKAGE_NAME EXPORT;
    static const std::string MEDIA_DEVICE_NAME EXPORT;

    // As set Parameter about time
    static const std::string MEDIA_DATE_MODIFIED EXPORT;
    static const std::string MEDIA_DATE_ADDED EXPORT;
    static const std::string MEDIA_DATE_TAKEN EXPORT;
    static const std::string MEDIA_DURATION EXPORT;
    static const std::string MEDIA_TIME_PENDING EXPORT;
    static const std::string MEDIA_IS_FAV EXPORT;
    static const std::string MEDIA_DATE_TRASHED EXPORT;
    static const std::string MEDIA_DATE_DELETED EXPORT;
    static const std::string MEDIA_HIDDEN EXPORT;

    // Asset Parameter deperated
    static const std::string MEDIA_PARENT_ID EXPORT;
    static const std::string MEDIA_RELATIVE_PATH EXPORT;
    static const std::string MEDIA_VIRTURL_PATH EXPORT;

    // All Columns
    static const std::set<std::string> MEDIA_COLUMNS EXPORT;
    // Default fetch columns
    static const std::set<std::string> DEFAULT_FETCH_COLUMNS EXPORT;

    // Util consts
    static const std::string ASSETS_QUERY_FILTER EXPORT;
};

class PhotoColumn : public MediaColumn {
public:
    // column only in PhotoTable
    static const std::string PHOTO_ORIENTATION EXPORT;
    static const std::string PHOTO_EXIF_ROTATE EXPORT;
    static const std::string PHOTO_LATITUDE EXPORT;
    static const std::string PHOTO_LONGITUDE EXPORT;
    static const std::string PHOTO_HEIGHT EXPORT;
    static const std::string PHOTO_WIDTH EXPORT;
    static const std::string PHOTO_LCD_VISIT_TIME EXPORT;
    static const std::string PHOTO_EDIT_TIME EXPORT;
    static const std::string PHOTO_POSITION EXPORT;
    static const std::string PHOTO_DIRTY EXPORT;
    static const std::string PHOTO_CLOUD_ID EXPORT;
    static const std::string PHOTO_SUBTYPE EXPORT;
    static const std::string PHOTO_META_DATE_MODIFIED EXPORT;
    static const std::string PHOTO_SYNC_STATUS EXPORT;
    static const std::string PHOTO_CLOUD_VERSION EXPORT;
    static const std::string CAMERA_SHOT_KEY EXPORT;
    static const std::string PHOTO_USER_COMMENT EXPORT;
    static const std::string PHOTO_ALL_EXIF EXPORT;
    static const std::string PHOTO_CLEAN_FLAG EXPORT;
    static const std::string PHOTO_DYNAMIC_RANGE_TYPE EXPORT;
    static const std::string PHOTO_HDR_MODE EXPORT;
    static const std::string MOVING_PHOTO_EFFECT_MODE EXPORT;
    static const std::string PHOTO_HAS_ASTC EXPORT; // This attribute has been replaced by "thumbnail_ready"
    static const std::string PHOTO_THUMBNAIL_READY EXPORT;
    static const std::string PHOTO_THUMBNAIL_VISIBLE EXPORT;

    static const std::string PHOTO_SYNCING EXPORT;
    static const std::string PHOTO_DATE_YEAR EXPORT;
    static const std::string PHOTO_DATE_MONTH EXPORT;
    static const std::string PHOTO_DATE_DAY EXPORT;
    static const std::string PHOTO_SHOOTING_MODE EXPORT;
    static const std::string PHOTO_SHOOTING_MODE_TAG EXPORT;
    static const std::string PHOTO_LAST_VISIT_TIME EXPORT;
    static const std::string PHOTO_HIDDEN_TIME EXPORT;
    static const std::string PHOTO_THUMB_STATUS EXPORT;
    static const std::string PHOTO_ID EXPORT;
    static const std::string PHOTO_QUALITY EXPORT;
    static const std::string PHOTO_FIRST_VISIT_TIME EXPORT;
    static const std::string PHOTO_DEFERRED_PROC_TYPE EXPORT;
    static const std::string PHOTO_LCD_SIZE EXPORT;
    static const std::string PHOTO_THUMB_SIZE EXPORT;
    static const std::string PHOTO_IS_TEMP EXPORT;
    static const std::string PHOTO_BURST_COVER_LEVEL EXPORT;
    static const std::string PHOTO_BURST_KEY EXPORT;
    static const std::string PHOTO_COVER_POSITION EXPORT;
    static const std::string PHOTO_IS_RECTIFICATION_COVER EXPORT;
    static const std::string PHOTO_OWNER_ALBUM_ID EXPORT;
    static const std::string PHOTO_ORIGINAL_ASSET_CLOUD_ID EXPORT;
    static const std::string PHOTO_SOURCE_PATH EXPORT;
    static const std::string PHOTO_ORIGINAL_SUBTYPE EXPORT;
    static const std::string PHOTO_DETAIL_TIME EXPORT;
    static const std::string SUPPORTED_WATERMARK_TYPE EXPORT;
    static const std::string PHOTO_METADATA_FLAGS EXPORT;
    static const std::string PHOTO_CHECK_FLAG EXPORT;
    static const std::string STAGE_VIDEO_TASK_STATUS EXPORT;
    static const std::string PHOTO_IS_AUTO EXPORT;
    static const std::string PHOTO_MEDIA_SUFFIX EXPORT;
    static const std::string PHOTO_REAL_LCD_VISIT_TIME EXPORT;
    static const std::string PHOTO_VISIT_COUNT EXPORT;
    static const std::string PHOTO_LCD_VISIT_COUNT EXPORT;
    static const std::string PHOTO_TRANSCODE_TIME EXPORT;
    static const std::string PHOTO_TRANS_CODE_FILE_SIZE EXPORT;
    static const std::string PHOTO_EXIST_COMPATIBLE_DUPLICATE EXPORT;
    static const std::string PHOTO_FILE_SOURCE_TYPE EXPORT;
    static const std::string PHOTO_IS_RECENT_SHOW EXPORT;
    static const std::string PHOTO_HAS_APPLINK EXPORT;
    static const std::string PHOTO_APPLINK EXPORT;
    static const std::string PHOTO_SOUTH_DEVICE_TYPE EXPORT;
    static const std::string PHOTO_VIDEO_MODE EXPORT;
    static const std::string PHOTO_FILE_INODE EXPORT;
    static const std::string PHOTO_STORAGE_PATH EXPORT;
    static const std::string PHOTO_ASPECT_RATIO EXPORT;
    static const std::string PHOTO_IS_CRITICAL EXPORT;
    static const std::string PHOTO_CRITICAL_TYPE EXPORT;
    static const std::string PHOTO_CHANGE_TIME EXPORT;
    static const std::string PHOTO_EDIT_DATA_EXIST EXPORT;

    // Photo-only default fetch columns
    static const std::set<std::string> DEFAULT_FETCH_COLUMNS EXPORT;

    // index in PhotoTable
    static const std::string PHOTO_CLOUD_ID_INDEX EXPORT;
    static const std::string PHOTO_DATE_YEAR_INDEX EXPORT;
    static const std::string PHOTO_DATE_MONTH_INDEX EXPORT;
    static const std::string PHOTO_DATE_DAY_INDEX EXPORT;
    static const std::string PHOTO_SCHPT_ADDED_INDEX EXPORT;
    static const std::string PHOTO_SCHPT_ALBUM_GENERAL_INDEX EXPORT;
    static const std::string PHOTO_SCHPT_ALBUM_INDEX EXPORT;
    static const std::string PHOTO_SCHPT_PHOTO_DATEADDED_INDEX EXPORT;
    static const std::string PHOTO_SCHPT_ADDED_ALBUM_INDEX EXPORT;
    static const std::string PHOTO_SCHPT_MEDIA_TYPE_INDEX EXPORT;
    static const std::string PHOTO_SCHPT_DAY_INDEX EXPORT;
    static const std::string PHOTO_HIDDEN_TIME_INDEX EXPORT;
    static const std::string PHOTO_SCHPT_HIDDEN_TIME_INDEX EXPORT;
    static const std::string PHOTO_FAVORITE_INDEX EXPORT;
    static const std::string PHOTO_SCHPT_READY_INDEX EXPORT;
    static const std::string PHOTO_SCHPT_CLOUD_ENHANCEMENT_ALBUM_INDEX EXPORT;
    static const std::string LATITUDE_INDEX EXPORT;
    static const std::string LONGITUDE_INDEX EXPORT;
    static const std::string PHOTO_SORT_MEDIA_TYPE_DATE_ADDED_INDEX EXPORT;
    static const std::string PHOTO_SORT_MEDIA_TYPE_DATE_TAKEN_INDEX EXPORT;
    static const std::string PHOTO_SORT_IN_ALBUM_DATE_ADDED_INDEX EXPORT;
    static const std::string PHOTO_SORT_IN_ALBUM_DATE_TAKEN_INDEX EXPORT;
    static const std::string PHOTO_SORT_IN_ALBUM_SIZE_INDEX EXPORT;
    static const std::string PHOTO_SORT_MEDIA_TYPE_SIZE_INDEX EXPORT;
    static const std::string PHOTO_SORT_IN_ALBUM_DISPLAY_NAME_INDEX EXPORT;
    static const std::string PHOTO_SORT_MEDIA_TYPE_DISPLAY_NAME_INDEX EXPORT;
    static const std::string PHOTO_QUERY_THUMBNAIL_WHITE_BLOCKS_INDEX EXPORT;
    static const std::string PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX EXPORT;
    static const std::string PHOTO_BURST_MODE_ALBUM_INDEX EXPORT;
    static const std::string PHOTO_FRONT_CAMERA_ALBUM_INDEX EXPORT;
    static const std::string PHOTO_RAW_IMAGE_ALBUM_INDEX EXPORT;
    static const std::string PHOTO_MOVING_PHOTO_ALBUM_INDEX EXPORT;
    // for clone query
    static const std::string PHOTO_DISPLAYNAME_INDEX EXPORT;
    // for burst query
    static const std::string PHOTO_BURSTKEY_INDEX EXPORT;
    // for count query
    static const std::string PHOTO_SCHPT_MEDIA_TYPE_COUNT_READY_INDEX EXPORT;
    static const std::string PHOTO_SCHPT_DATE_YEAR_COUNT_READY_INDEX EXPORT;
    static const std::string PHOTO_SCHPT_DATE_MONTH_COUNT_READY_INDEX EXPORT;
    // format in PhotoTable year month day
    static const std::string PHOTO_DATE_YEAR_FORMAT EXPORT;
    static const std::string PHOTO_DATE_MONTH_FORMAT EXPORT;
    static const std::string PHOTO_DATE_DAY_FORMAT EXPORT;
    static const std::string PHOTO_FRONT_CAMERA EXPORT;
    // cloud enhancement
    static const std::string PHOTO_CE_AVAILABLE EXPORT;
    static const std::string PHOTO_CE_STATUS_CODE EXPORT;
    static const std::string PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE EXPORT;
    static const std::string PHOTO_STRONG_ASSOCIATION EXPORT;
    static const std::string PHOTO_ASSOCIATE_FILE_ID EXPORT;
    static const std::string PHOTO_HAS_CLOUD_WATERMARK EXPORT;
    static const std::string PHOTO_COMPOSITE_DISPLAY_STATUS EXPORT;
    // format in PhotoTable detail time
    static const std::string PHOTO_DETAIL_TIME_FORMAT EXPORT;

    // table name
    static const std::string PHOTOS_TABLE EXPORT;
    static const std::string HIGHLIGHT_TABLE EXPORT;

    static const std::string TAB_OLD_PHOTOS_TABLE EXPORT;
    static const std::string TAB_ASSET_AND_ALBUM_OPERATION_TABLE EXPORT;

    // path
    static const std::string FILES_CLOUD_DIR EXPORT;
    static const std::string FILES_LOCAL_DIR EXPORT;

    // create PhotoTable sql
    static const std::string CREATE_PHOTO_TABLE EXPORT;
    static const std::string CREATE_CLOUD_ID_INDEX EXPORT;
    static const std::string CREATE_YEAR_INDEX EXPORT;
    static const std::string CREATE_MONTH_INDEX EXPORT;
    static const std::string CREATE_DAY_INDEX EXPORT;
    static const std::string DROP_SCHPT_MEDIA_TYPE_INDEX EXPORT;
    static const std::string DROP_BURST_MODE_ALBUM_INDEX EXPORT;
    static const std::string CREATE_SCHPT_MEDIA_TYPE_INDEX EXPORT;
    static const std::string CREATE_SCHPT_DAY_INDEX EXPORT;
    static const std::string DROP_SCHPT_DAY_INDEX EXPORT;
    static const std::string CREATE_HIDDEN_TIME_INDEX EXPORT;
    static const std::string CREATE_SCHPT_HIDDEN_TIME_INDEX EXPORT;
    static const std::string DROP_SCHPT_HIDDEN_TIME_INDEX EXPORT;
    static const std::string CREATE_PHOTO_FAVORITE_INDEX EXPORT;
    static const std::string DROP_PHOTO_FAVORITE_INDEX EXPORT;
    static const std::string CREATE_PHOTO_DISPLAYNAME_INDEX EXPORT;
    static const std::string CREATE_PHOTO_BURSTKEY_INDEX EXPORT;
    static const std::string UPDATE_READY_ON_THUMBNAIL_UPGRADE EXPORT;
    static const std::string UPDATA_PHOTOS_DATA_UNIQUE EXPORT;
    static const std::string UPDATE_LCD_STATUS_NOT_UPLOADED EXPORT;
    static const std::string UPDATE_LATITUDE_AND_LONGITUDE_DEFAULT_NULL EXPORT;
    static const std::string UPDATE_PHOTO_QUALITY_OF_NULL_PHOTO_ID EXPORT;

    // create indexes for Photo
    static const std::string INDEX_SCTHP_ADDTIME EXPORT;
    static const std::string DROP_INDEX_SCTHP_ADDTIME EXPORT;
    static const std::string DROP_INDEX_SCHPT_ADDTIME_ALBUM EXPORT;
    static const std::string INDEX_CAMERA_SHOT_KEY EXPORT;
    static const std::string INDEX_SCHPT_READY EXPORT;
    static const std::string DROP_INDEX_SCHPT_READY EXPORT;
    static const std::string CREATE_SCHPT_YEAR_COUNT_READY_INDEX;
    static const std::string CREATE_SCHPT_MONTH_COUNT_READY_INDEX;
    static const std::string CREATE_SCHPT_MEDIA_TYPE_COUNT_READY_INDEX;
    static const std::string DROP_SCHPT_YEAR_COUNT_READY_INDEX;
    static const std::string DROP_SCHPT_MONTH_COUNT_READY_INDEX;
    static const std::string DROP_SCHPT_MEDIA_TYPE_COUNT_READY_INDEX;
    static const std::string CREATE_SCHPT_CLOUD_ENHANCEMENT_ALBUM_INDEX;
    static const std::string INDEX_SCHPT_ALBUM_GENERAL;
    static const std::string INDEX_SCHPT_ALBUM;
    static const std::string INDEX_SCTHP_PHOTO_DATEADDED;
    static const std::string INDEX_LATITUDE;
    static const std::string INDEX_LONGITUDE;
    static const std::string CREATE_PHOTO_SORT_MEDIA_TYPE_DATE_ADDED_INDEX;
    static const std::string CREATE_PHOTO_SORT_MEDIA_TYPE_DATE_TAKEN_INDEX;
    static const std::string CREATE_PHOTO_SORT_IN_ALBUM_DATE_ADDED_INDEX;
    static const std::string CREATE_PHOTO_SORT_IN_ALBUM_DATE_TAKEN_INDEX;
    static const std::string CREATE_PHOTO_SORT_IN_ALBUM_SIZE_INDEX;
    static const std::string CREATE_PHOTO_SORT_MEDIA_TYPE_SIZE_INDEX;
    static const std::string CREATE_PHOTO_SORT_IN_ALBUM_DISPLAY_NAME_INDEX;
    static const std::string CREATE_PHOTO_SORT_MEDIA_TYPE_DISPLAY_NAME_INDEX;
    static const std::string CREATE_PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX;
    static const std::string CREATE_PHOTO_BURST_MODE_ALBUM_INDEX;
    static const std::string CREATE_PHOTO_FRONT_CAMERA_ALBUM_INDEX;
    static const std::string CREATE_PHOTO_RAW_IMAGE_ALBUM_INDEX;
    static const std::string CREATE_PHOTO_MOVING_PHOTO_ALBUM_INDEX;
    static const std::string INDEX_QUERY_THUMBNAIL_WHITE_BLOCKS;

    // create Photo cloud sync trigger
    static const std::string CREATE_PHOTOS_DELETE_TRIGGER EXPORT;
    static const std::string CREATE_PHOTOS_FDIRTY_TRIGGER EXPORT;
    static const std::string CREATE_PHOTOS_MDIRTY_TRIGGER EXPORT;
    static const std::string CREATE_PHOTOS_INSERT_CLOUD_SYNC EXPORT;
    static const std::string CREATE_PHOTOS_UPDATE_CLOUD_SYNC EXPORT;
    static const std::string CREATE_PHOTOS_METADATA_DIRTY_TRIGGER EXPORT;

    // highlight trigger
    static const std::string MEDIA_DATA_DB_HIGHLIGHT_TRIGGER EXPORT;
    static const std::string INSERT_GENERATE_HIGHLIGHT_THUMBNAIL EXPORT;
    static const std::string UPDATE_GENERATE_HIGHLIGHT_THUMBNAIL EXPORT;
    static const std::string INDEX_HIGHLIGHT_FILEID EXPORT;

    // photo uri
    static const std::string PHOTO_URI_PREFIX EXPORT;
    static const std::string PHOTO_TYPE_URI EXPORT;
    static const std::string DEFAULT_PHOTO_URI EXPORT;
    static const std::string PHOTO_CACHE_URI_PREFIX EXPORT;

    // cloud sync type
    static const std::string CLOUD_TYPE EXPORT;

    // cloud sync uri
    static const std::string PHOTO_CLOUD_URI_PREFIX EXPORT;
    static const std::string PHOTO_CLOUD_TRIGGER_PREFIX EXPORT;
    static const std::string PHOTO_GALLERY_CLOUD_URI_PREFIX EXPORT;

    // cloud notify uri
    static const std::string PHOTO_HEIGHT_ERROR_URI_PREFIX EXPORT;
    static const std::string PHOTO_DOWNLOAD_SUCCEED_URI_PREFIX EXPORT;
    static const std::string PHOTO_CLOUD_GALLERY_REBUILD_URI_PREFIX EXPORT;
    // yuv uri
    static const std::string PHOTO_REQUEST_PICTURE EXPORT;
    static const std::string PHOTO_REQUEST_PICTURE_BUFFER EXPORT;
    // all columns
    static const std::set<std::string> PHOTO_COLUMNS EXPORT;

    static const std::string QUERY_MEDIA_VOLUME EXPORT;

    static const std::string HIGHTLIGHT_COVER_URI EXPORT;
    static const std::string HIGHTLIGHT_URI EXPORT;
    static const std::string HIDDEN_PHOTO_URI_PREFIX EXPORT;
    static const std::string TRASHED_PHOTO_URI_PREFIX EXPORT;

    EXPORT static bool IsPhotoColumn(const std::string &columnName);
    EXPORT static std::string CheckUploadPhotoColumns();
    EXPORT static std::string CheckMetaRecoveryPhotoColumns();

    static const std::string PHOTOS_QUERY_FILTER EXPORT;
};

class AudioColumn : public MediaColumn {
public:
    // column only in AudioTable
    static const std::string AUDIO_ALBUM EXPORT;
    static const std::string AUDIO_ARTIST EXPORT;
    static const std::string AUDIO_FILE_SOURCE_TYPE EXPORT;
    static const std::string AUDIO_IS_TEMP EXPORT;

    // table name
    static const std::string AUDIOS_TABLE EXPORT;

    // create AudioTable sql
    static const std::string CREATE_AUDIO_TABLE EXPORT;

    // audio uri
    static const std::string AUDIO_URI_PREFIX EXPORT;
    static const std::string AUDIO_TYPE_URI EXPORT;
    static const std::string DEFAULT_AUDIO_URI EXPORT;

    // all columns
    static const std::set<std::string> AUDIO_COLUMNS EXPORT;

    static const std::string QUERY_MEDIA_VOLUME EXPORT;

    static bool IsAudioColumn(const std::string &columnName) EXPORT;
};

class PhotoExtColumn {
public:
    // table name
    static const std::string PHOTOS_EXT_TABLE EXPORT;

    // column name
    static const std::string PHOTO_ID EXPORT;
    static const std::string THUMBNAIL_SIZE EXPORT;
    static const std::string EDITDATA_SIZE EXPORT;

    // create table sql
    static const std::string CREATE_PHOTO_EXT_TABLE EXPORT;
};

} // namespace OHOS::Media
#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_COLUMN_H_
