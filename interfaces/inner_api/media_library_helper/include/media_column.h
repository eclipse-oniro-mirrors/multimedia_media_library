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
    MEDIA_SHARE_ALBUM,
};

enum class SouthDeviceType : int32_t {
    SOUTH_DEVICE_VISIT = -1,
    SOUTH_DEVICE_NULL = 0,
    SOUTH_DEVICE_CLOUD = 1,
    SOUTH_DEVICE_HDC = 2
};

enum class PhotoRiskStatus : int32_t {
    UNIDENTIFIED = 0,
    APPROVED = 1,
    SUSPICIOUS = 2,
    REJECTED = 3,
};

class MediaColumn {
public:
    // Asset Base Parameter
    static const std::string MEDIA_ID;
    static const std::string MEDIA_FILE_PATH;
    static const std::string MEDIA_SIZE;
    static const std::string MEDIA_TITLE;
    static const std::string MEDIA_NAME;
    static const std::string MEDIA_TYPE;
    static const std::string MEDIA_MIME_TYPE;
    static const std::string MEDIA_OWNER_PACKAGE;
    static const std::string MEDIA_OWNER_APPID;
    static const std::string MEDIA_PACKAGE_NAME;
    static const std::string MEDIA_DEVICE_NAME;

    // As set Parameter about time
    static const std::string MEDIA_DATE_MODIFIED;
    static const std::string MEDIA_DATE_ADDED;
    static const std::string MEDIA_DATE_TAKEN;
    static const std::string MEDIA_DURATION;
    static const std::string MEDIA_TIME_PENDING;
    static const std::string MEDIA_IS_FAV;
    static const std::string MEDIA_DATE_TRASHED;
    static const std::string MEDIA_DATE_DELETED;
    static const std::string MEDIA_HIDDEN;

    // Asset Parameter deperated
    static const std::string MEDIA_PARENT_ID;
    static const std::string MEDIA_RELATIVE_PATH;
    static const std::string MEDIA_VIRTUAL_PATH;
    // deprecated since 6.1-release
    static const std::string MEDIA_VIRTURL_PATH;

    // All Columns
    static const std::set<std::string> MEDIA_COLUMNS;
    // Default fetch columns
    static const std::set<std::string> DEFAULT_FETCH_COLUMNS;
};

class PhotoColumn : public MediaColumn {
public:
    // column only in PhotoTable
    static const std::string PHOTO_ORIENTATION;
    static const std::string PHOTO_EXIF_ROTATE;
    static const std::string PHOTO_LATITUDE;
    static const std::string PHOTO_LONGITUDE;
    static const std::string PHOTO_HEIGHT;
    static const std::string PHOTO_WIDTH;
    static const std::string PHOTO_LCD_VISIT_TIME;
    static const std::string PHOTO_EDIT_TIME;
    static const std::string PHOTO_POSITION;
    static const std::string PHOTO_DIRTY;
    static const std::string PHOTO_CLOUD_ID;
    static const std::string PHOTO_SUBTYPE;
    static const std::string PHOTO_META_DATE_MODIFIED;
    static const std::string PHOTO_SYNC_STATUS;
    static const std::string PHOTO_CLOUD_VERSION;
    static const std::string CAMERA_SHOT_KEY;
    static const std::string PHOTO_USER_COMMENT;
    static const std::string PHOTO_ALL_EXIF;
    static const std::string PHOTO_CLEAN_FLAG;
    static const std::string PHOTO_DYNAMIC_RANGE_TYPE;
    static const std::string PHOTO_HDR_MODE;
    static const std::string PHOTO_EDIT_DATA_EXIST;
    static const std::string MOVING_PHOTO_EFFECT_MODE;
    static const std::string PHOTO_HAS_ASTC; // This attribute has been replaced by "thumbnail_ready"
    static const std::string PHOTO_THUMBNAIL_READY;
    static const std::string PHOTO_THUMBNAIL_VISIBLE;

    static const std::string IS_STYLE_PHOTO;
    static const std::string PHOTO_SYNCING;
    static const std::string PHOTO_DATE_YEAR;
    static const std::string PHOTO_DATE_MONTH;
    static const std::string PHOTO_DATE_DAY;
    static const std::string PHOTO_SHOOTING_MODE;
    static const std::string PHOTO_SHOOTING_MODE_TAG;
    static const std::string PHOTO_LAST_VISIT_TIME;
    static const std::string PHOTO_HIDDEN_TIME;
    static const std::string PHOTO_THUMB_STATUS;
    static const std::string PHOTO_ID;
    static const std::string PHOTO_QUALITY;
    static const std::string PHOTO_FIRST_VISIT_TIME;
    static const std::string PHOTO_DEFERRED_PROC_TYPE;
    static const std::string PHOTO_LCD_SIZE;
    static const std::string PHOTO_THUMB_SIZE;
    static const std::string PHOTO_IS_TEMP;
    static const std::string PHOTO_BURST_COVER_LEVEL;
    static const std::string PHOTO_BURST_KEY;
    static const std::string PHOTO_COVER_POSITION;
    static const std::string PHOTO_IS_RECTIFICATION_COVER;
    static const std::string PHOTO_OWNER_ALBUM_ID;
    static const std::string PHOTO_ORIGINAL_ASSET_CLOUD_ID;
    static const std::string PHOTO_SOURCE_PATH;
    static const std::string PHOTO_ORIGINAL_SUBTYPE;
    static const std::string PHOTO_DETAIL_TIME;
    static const std::string SUPPORTED_WATERMARK_TYPE;
    static const std::string PHOTO_METADATA_FLAGS;
    static const std::string PHOTO_CHECK_FLAG;
    static const std::string STAGE_VIDEO_TASK_STATUS;
    static const std::string PHOTO_IS_AUTO;
    static const std::string PHOTO_MEDIA_SUFFIX;
    static const std::string PHOTO_REAL_LCD_VISIT_TIME;
    static const std::string PHOTO_VISIT_COUNT;
    static const std::string PHOTO_LCD_VISIT_COUNT;
    static const std::string PHOTO_TRANSCODE_TIME;
    static const std::string PHOTO_TRANS_CODE_FILE_SIZE;
    static const std::string PHOTO_EXIST_COMPATIBLE_DUPLICATE;
    static const std::string PHOTO_FILE_SOURCE_TYPE;
    static const std::string PHOTO_IS_RECENT_SHOW;
    static const std::string PHOTO_HAS_APPLINK;
    static const std::string PHOTO_APPLINK;
    static const std::string PHOTO_SOUTH_DEVICE_TYPE;
    static const std::string PHOTO_VIDEO_MODE;
    static const std::string PHOTO_FILE_INODE;
    static const std::string PHOTO_STORAGE_PATH;
    static const std::string PHOTO_ASPECT_RATIO;
    static const std::string PHOTO_IS_CRITICAL;
    static const std::string PHOTO_CRITICAL_TYPE;
    static const std::string PHOTO_RISK_STATUS;
    static const std::string PHOTO_CHANGE_TIME;
    static const std::string PHOTO_DATE_ADDED_YEAR;
    static const std::string PHOTO_DATE_ADDED_MONTH;
    static const std::string PHOTO_DATE_ADDED_DAY;
    static const std::string UNIQUE_ID;
    static const std::string MOVING_PHOTO_LIVEPHOTO_4D_STATUS;
    static const std::string MOVING_PHOTO_LIVEPHOTO_4D_LATEST_PAIR;
    static const std::string LOCAL_ASSET_SIZE;
    static const std::string ATTACHMENT_SIZE;
    static const std::string MUSIC_MASTER_MODE;
    static const std::string PHOTO_FILE_HIDDEN;
    static const std::string PHOTO_NEED_THUMBNAIL;
    static const std::string PHOTO_LCD_FILE_SIZE;
    static const std::string COMPRESSION_QUALITY;
    static const std::string C2PA_CONFIG_INFO;

    // Photo-only default fetch columns
    static const std::set<std::string> DEFAULT_FETCH_COLUMNS;

    // index in PhotoTable
    static const std::string PHOTO_CLOUD_ID_INDEX;
    static const std::string PHOTO_DATE_YEAR_INDEX;
    static const std::string PHOTO_DATE_MONTH_INDEX;
    static const std::string PHOTO_DATE_DAY_INDEX;
    static const std::string PHOTO_SCHPT_ADDED_INDEX;
    static const std::string PHOTO_SCHPT_ALBUM_GENERAL_INDEX;
    static const std::string PHOTO_SCHPT_ALBUM_INDEX;
    static const std::string PHOTO_SCHPT_PHOTO_DATEADDED_INDEX;
    static const std::string PHOTO_SCHPT_ADDED_ALBUM_INDEX;
    static const std::string PHOTO_SCHPT_MEDIA_TYPE_INDEX;
    static const std::string PHOTO_SCHPT_DAY_INDEX;
    static const std::string PHOTO_HIDDEN_TIME_INDEX;
    static const std::string PHOTO_SCHPT_HIDDEN_TIME_INDEX;
    static const std::string PHOTO_FAVORITE_INDEX;
    static const std::string PHOTO_SCHPT_READY_INDEX;
    static const std::string PHOTO_SCHPT_CLOUD_ENHANCEMENT_ALBUM_INDEX;
    static const std::string LATITUDE_INDEX;
    static const std::string LONGITUDE_INDEX;
    static const std::string PHOTO_SORT_MEDIA_TYPE_DATE_ADDED_INDEX;
    static const std::string PHOTO_SORT_MEDIA_TYPE_DATE_TAKEN_INDEX;
    static const std::string PHOTO_SORT_IN_ALBUM_DATE_ADDED_INDEX;
    static const std::string PHOTO_SORT_IN_ALBUM_DATE_TAKEN_INDEX;
    static const std::string PHOTO_SORT_IN_ALBUM_SIZE_INDEX;
    static const std::string PHOTO_SORT_MEDIA_TYPE_SIZE_INDEX;
    static const std::string PHOTO_SORT_IN_ALBUM_DISPLAY_NAME_INDEX;
    static const std::string PHOTO_SORT_MEDIA_TYPE_DISPLAY_NAME_INDEX;
    static const std::string PHOTO_QUERY_THUMBNAIL_WHITE_BLOCKS_INDEX;
    static const std::string PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX;
    static const std::string PHOTO_BURST_MODE_ALBUM_INDEX;
    static const std::string PHOTO_FRONT_CAMERA_ALBUM_INDEX;
    static const std::string PHOTO_RAW_IMAGE_ALBUM_INDEX;
    static const std::string PHOTO_MOVING_PHOTO_ALBUM_INDEX;
    // for clone query
    static const std::string PHOTO_DISPLAYNAME_INDEX;
    // for burst query
    static const std::string PHOTO_BURSTKEY_INDEX;
    // for count query
    static const std::string PHOTO_SCHPT_MEDIA_TYPE_COUNT_READY_INDEX;
    static const std::string PHOTO_SCHPT_DATE_YEAR_COUNT_READY_INDEX;
    static const std::string PHOTO_SCHPT_DATE_MONTH_COUNT_READY_INDEX;
    // format in PhotoTable year month day
    static const std::string PHOTO_DATE_YEAR_FORMAT;
    static const std::string PHOTO_DATE_MONTH_FORMAT;
    static const std::string PHOTO_DATE_DAY_FORMAT;
    static const std::string PHOTO_FRONT_CAMERA;
    // cloud enhancement
    static const std::string PHOTO_CE_AVAILABLE;
    static const std::string PHOTO_CE_STATUS_CODE;
    static const std::string PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE;
    static const std::string PHOTO_STRONG_ASSOCIATION;
    static const std::string PHOTO_ASSOCIATE_FILE_ID;
    static const std::string PHOTO_HAS_CLOUD_WATERMARK;
    static const std::string PHOTO_COMPOSITE_DISPLAY_STATUS;
    // format in PhotoTable detail time
    static const std::string PHOTO_DETAIL_TIME_FORMAT;

    // table name
    static const std::string PHOTOS_TABLE;
    static const std::string HIGHLIGHT_TABLE;

    static const std::string TAB_OLD_PHOTOS_TABLE;
    static const std::string TAB_ASSET_AND_ALBUM_OPERATION_TABLE;

    // path
    static const std::string FILES_CLOUD_DIR;
    static const std::string FILES_LOCAL_DIR;
    
    static const std::string MEDIA_DATA_DB_HIGHLIGHT_TRIGGER;
    // photo uri
    static const std::string PHOTO_URI_PREFIX;
    static const std::string PHOTO_TYPE_URI;
    static const std::string DEFAULT_PHOTO_URI;
    static const std::string PHOTO_CACHE_URI_PREFIX;

    // cloud sync type
    static const std::string CLOUD_TYPE;

    // cloud sync uri
    static const std::string PHOTO_CLOUD_URI_PREFIX;
    static const std::string PHOTO_CLOUD_TRIGGER_PREFIX;
    static const std::string PHOTO_GALLERY_CLOUD_URI_PREFIX;
    static const std::string PHOTO_THM_DOWNLOAD_URI_PREFIX;

    // cloud notify uri
    static const std::string PHOTO_HEIGHT_ERROR_URI_PREFIX;
    static const std::string PHOTO_DOWNLOAD_SUCCEED_URI_PREFIX;
    static const std::string PHOTO_CLOUD_GALLERY_REBUILD_URI_PREFIX;
    // yuv uri
    static const std::string PHOTO_REQUEST_PICTURE;
    static const std::string PHOTO_REQUEST_PICTURE_BUFFER;
    // all columns
    static const std::set<std::string> PHOTO_COLUMNS;

    static const std::string HIGHTLIGHT_COVER_URI;
    static const std::string HIGHTLIGHT_URI;
    static const std::string HIDDEN_PHOTO_URI_PREFIX;
    static const std::string TRASHED_PHOTO_URI_PREFIX;

    static bool IsPhotoColumn(const std::string &columnName);
    static std::string CheckUploadPhotoColumns();
    static std::string CheckMetaRecoveryPhotoColumns();

    static const std::string SUPPORTED_DEFERRED_EFFECTS;
    static const std::string DEFERRED_EFFECT_STATUS;

    // for cloud sync
    static const std::string LCD_ASPECT_RATIO;

    // clone file info db
    static const std::string CLONE_FILE_INFO_PATH;
    static const std::string CLONE_FILE_INFO_NEW_PATH;

    // for share
    static const std::string PHOTO_IS_SHARED;
    static const std::string PHOTO_SHARE_OWNER_INFO;
    static const std::string PHOTO_SHARE_ALBUM_OWNER;
    static const std::string PHOTO_VISIBILITY;
    static const std::string PHOTO_SHARE_RISK_STATUS;
    static const std::string PHOTO_SHARE_RISK_TYPE;
    static const std::string PHOTO_SHARE_DATE_DAY;
    static const std::string PHOTO_SHARE_GROUP;
};

class PhotoExtColumn {
public:
    // table name
    static const std::string PHOTOS_EXT_TABLE;

    // column name
    static const std::string PHOTO_ID;
    static const std::string THUMBNAIL_SIZE;
    static const std::string EDITDATA_SIZE;
    static const std::string LCD_FILE_MODIFY_TIME;
    static const std::string LCD_USING_STATUS;
    static const std::string LCD_DOWNLOAD_RETRY_COUNTS;
    static const std::string LCD_DOWNLOAD_RETRY_TIME;
};

} // namespace OHOS::Media
#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_COLUMN_H_
