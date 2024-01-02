/*
 * Copyright (C) 2021-2023 Huawei Device Co., Ltd.
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

#include "medialibrary_type_const.h"
#include "media_column.h"
#include "userfilemgr_uri.h"

namespace OHOS {
namespace Media {
const int32_t MEDIA_RDB_VERSION = 51;
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
};

enum {
    MEDIA_API_VERSION_DEFAULT = 8,
    MEDIA_API_VERSION_V9,
    MEDIA_API_VERSION_V10,
};

const std::string MEDIA_LIBRARY_VERSION = "1.0";

const int32_t DEVICE_SYNCSTATUSING = 0;
const int32_t DEVICE_SYNCSTATUS_COMPLETE = 1;

const std::string MEDIA_DATA_DEVICE_PATH = "local";
const std::string MEDIALIBRARY_TABLE = "Files";
const std::string SMARTALBUM_TABLE = "SmartAlbum";
const std::string SMARTALBUM_MAP_TABLE = "SmartMap";
const std::string CATEGORY_SMARTALBUM_MAP_TABLE = "CategorySmartAlbumMap";
const std::string MEDIATYPE_DIRECTORY_TABLE = "MediaTypeDirectory";
const std::string DEVICE_TABLE = "Device";
const std::string BUNDLE_PERMISSION_TABLE = "BundlePermission";
const std::string MEDIA_DATA_ABILITY_DB_NAME = "media_library.db";

const std::string BUNDLE_NAME = "com.ohos.medialibrary.medialibrarydata";

const std::string ML_FILE_SCHEME = "file";
const std::string ML_FILE_PREFIX = "file://";
const std::string ML_FILE_URI_PREFIX = "file://media";
const std::string ML_URI_NETWORKID = "networkid";
const std::string ML_URI_NETWORKID_EQUAL = "?networkid=";
const std::string ML_URI_AUTHORITY = "media";
const std::string ML_DATA_SHARE_SCHEME = "datashare";
const std::string MEDIALIBRARY_DATA_ABILITY_PREFIX = "datashare://";
const std::string MEDIALIBRARY_DATA_URI_IDENTIFIER = "/media";
const std::string MEDIALIBRARY_MEDIA_PREFIX = MEDIALIBRARY_DATA_ABILITY_PREFIX +
                                                     MEDIALIBRARY_DATA_URI_IDENTIFIER;
const std::string MEDIALIBRARY_TYPE_AUDIO_URI = "/audio";
const std::string MEDIALIBRARY_TYPE_VIDEO_URI = "/video";
const std::string MEDIALIBRARY_TYPE_IMAGE_URI = "/image";
const std::string MEDIALIBRARY_TYPE_FILE_URI  =  "/file";
const std::string MEDIALIBRARY_TYPE_ALBUM_URI  =  "/album";
const std::string MEDIALIBRARY_TYPE_SMARTALBUM_CHANGE_URI  =  "/smartalbum";
const std::string MEDIALIBRARY_TYPE_DEVICE_URI  =  "/device";
const std::string MEDIALIBRARY_TYPE_SMART_URI = "/smart";

const std::string AUDIO_URI_PREFIX = ML_FILE_URI_PREFIX + MEDIALIBRARY_TYPE_AUDIO_URI;
const std::string VIDEO_URI_PREFIX = ML_FILE_URI_PREFIX + MEDIALIBRARY_TYPE_VIDEO_URI;
const std::string IMAGE_URI_PREFIX = ML_FILE_URI_PREFIX + MEDIALIBRARY_TYPE_IMAGE_URI;
const std::string FILE_URI_PREFIX = ML_FILE_URI_PREFIX + MEDIALIBRARY_TYPE_FILE_URI;
const std::string ALBUM_URI_PREFIX = ML_FILE_URI_PREFIX + MEDIALIBRARY_TYPE_ALBUM_URI;

const std::string URI_TYPE_PHOTO = "Photo";
const std::string URI_TYPE_AUDIO_V10 = "Audio";
const std::string URI_TYPE_PHOTO_ALBUM = "PhotoAlbum";
constexpr int64_t AGING_TIME = 30LL * 60 * 60 * 24 * 1000;

const std::string MEDIALIBRARY_SMARTALBUM_URI = MEDIALIBRARY_DATA_URI + "/" + SMARTALBUM_TABLE;
const std::string MEDIALIBRARY_SMARTALBUM_MAP_URI = MEDIALIBRARY_DATA_URI + "/" + SMARTALBUM_MAP_TABLE;
const std::string MEDIALIBRARY_CATEGORY_SMARTALBUM_MAP_URI = MEDIALIBRARY_DATA_URI + "/"
                                                             + CATEGORY_SMARTALBUM_MAP_TABLE;
const std::string MEDIALIBRARY_DIRECTORY_URI = MEDIALIBRARY_DATA_URI + "/" + MEDIATYPE_DIRECTORY_TABLE;
const std::string MEDIALIBRARY_BUNDLEPERM_URI = MEDIALIBRARY_DATA_URI + "/" + BUNDLE_PERMISSION_INSERT;

const std::string MEDIALIBRARY_AUDIO_URI = MEDIALIBRARY_DATA_URI + '/' + "audio";
const std::string MEDIALIBRARY_VIDEO_URI = MEDIALIBRARY_DATA_URI + '/' + "video";
const std::string MEDIALIBRARY_IMAGE_URI = MEDIALIBRARY_DATA_URI + '/' + "image";
const std::string MEDIALIBRARY_FILE_URI  =  MEDIALIBRARY_DATA_URI + '/' + "file";
const std::string MEDIALIBRARY_ALBUM_URI  =  MEDIALIBRARY_DATA_URI + '/' + "album";
const std::string MEDIALIBRARY_SMARTALBUM_CHANGE_URI  =  MEDIALIBRARY_DATA_URI + '/' + "smartalbum";
const std::string MEDIALIBRARY_DEVICE_URI  =  MEDIALIBRARY_DATA_URI + '/' + "device";
const std::string MEDIALIBRARY_SMART_URI = MEDIALIBRARY_DATA_URI + '/' + "smart";
const std::string MEDIALIBRARY_REMOTEFILE_URI = MEDIALIBRARY_DATA_URI + '/' + "remotfile";

const std::string MEDIA_DATA_DB_ID = "file_id";
const std::string MEDIA_DATA_DB_URI = "uri";
const std::string MEDIA_DATA_DB_FILE_PATH = "data";
const std::string MEDIA_DATA_DB_SIZE = "size";
const std::string MEDIA_DATA_DB_PARENT_ID = "parent";
const std::string MEDIA_DATA_DB_DATE_MODIFIED = "date_modified";
const std::string MEDIA_DATA_DB_DATE_MODIFIED_S = "date_modified_s";
const std::string MEDIA_DATA_DB_DATE_MODIFIED_TO_SECOND = "CAST(date_modified / 1000 AS BIGINT) AS date_modified_s";
const std::string MEDIA_DATA_DB_DATE_ADDED = "date_added";
const std::string MEDIA_DATA_DB_DATE_ADDED_S = "date_added_s";
const std::string MEDIA_DATA_DB_DATE_ADDED_TO_SECOND = "CAST(date_added / 1000 AS BIGINT) AS date_added_s";
const std::string MEDIA_DATA_DB_MIME_TYPE = "mime_type";
const std::string MEDIA_DATA_DB_TITLE = "title";
const std::string MEDIA_DATA_DB_DESCRIPTION = "description";
const std::string MEDIA_DATA_DB_NAME = "display_name";
const std::string MEDIA_DATA_DB_ORIENTATION = "orientation";
const std::string MEDIA_DATA_DB_LATITUDE = "latitude";
const std::string MEDIA_DATA_DB_LONGITUDE = "longitude";
const std::string MEDIA_DATA_DB_DATE_TAKEN = "date_taken";
const std::string MEDIA_DATA_DB_THUMBNAIL = "thumbnail";
const std::string MEDIA_DATA_DB_THUMB_ASTC = "astc";
const std::string MEDIA_DATA_DB_HAS_ASTC = "has_astc";
const std::string MEDIA_DATA_DB_CONTENT_CREATE_TIME = "content_create_time";
const std::string MEDIA_DATA_DB_POSITION = "position";
const std::string MEDIA_DATA_DB_DIRTY = "dirty";
const std::string MEDIA_DATA_DB_CLOUD_ID = "cloud_id";
const std::string MEDIA_DATA_DB_META_DATE_MODIFIED = "meta_date_modified";
const std::string MEDIA_DATA_DB_SYNC_STATUS = "sync_status";

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
const std::string MEDIA_DATA_DB_PACKAGE_NAME = "package_name";

const std::string MEDIA_DATA_DB_IS_FAV = "is_favorite";
const std::string MEDIA_DATA_DB_IS_TRASH = "is_trash";
const std::string MEDIA_DATA_DB_RECYCLE_PATH = "recycle_path";
const std::string MEDIA_DATA_DB_DATE_TRASHED = "date_trashed";
const std::string MEDIA_DATA_DB_DATE_TRASHED_S = "date_trashed_s";
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
// ringtone uri constants
const std::string MEDIA_DATA_DB_RINGTONE_URI = "ringtone_uri";
const std::string MEDIA_DATA_DB_ALARM_URI = "alarm_uri";
const std::string MEDIA_DATA_DB_NOTIFICATION_URI = "notification_uri";
const std::string MEDIA_DATA_DB_RINGTONE_TYPE = "ringtone_type";

const std::string MEDIA_DATA_DB_PHOTO_ID = "photo_id";
const std::string MEDIA_DATA_DB_PHOTO_QUALITY = "photo_quality";
const std::string MEDIA_DATA_DB_FIRST_VISIT_TIME = "first_visit_time";
const std::string MEDIA_DATA_DB_DEFERRED_PROC_TYPE = "deferred_proc_type";

const std::string MEDIA_DATA_IMAGE_BITS_PER_SAMPLE = "BitsPerSample";
const std::string MEDIA_DATA_IMAGE_ORIENTATION = "Orientation";
const std::string MEDIA_DATA_IMAGE_IMAGE_LENGTH = "ImageLength";
const std::string MEDIA_DATA_IMAGE_IMAGE_WIDTH = "ImageWidth";
const std::string MEDIA_DATA_IMAGE_GPS_LATITUDE = "GPSLatitude";
const std::string MEDIA_DATA_IMAGE_GPS_LONGITUDE = "GPSLongitude";
const std::string MEDIA_DATA_IMAGE_GPS_LATITUDE_REF = "GPSLatitudeRef";
const std::string MEDIA_DATA_IMAGE_GPS_LONGITUDE_REF = "GPSLongitudeRef";
const std::string MEDIA_DATA_IMAGE_DATE_TIME_ORIGINAL = "DateTimeOriginalForMedia";
const std::string MEDIA_DATA_IMAGE_EXPOSURE_TIME = "ExposureTime";
const std::string MEDIA_DATA_IMAGE_F_NUMBER = "FNumber";
const std::string MEDIA_DATA_IMAGE_ISO_SPEED_RATINGS = "ISOSpeedRatings";
const std::string MEDIA_DATA_IMAGE_SCENE_TYPE = "SceneType";

const std::string MEDIA_COLUMN_COUNT = "count(*)";
const std::string MEDIA_COLUMN_COUNT_1 = "count(1)";
const std::string MEDIA_COLUMN_COUNT_DISTINCT_FILE_ID = "count(distinct file_id)";

const std::string PHOTO_INDEX = "photo_index";

const std::string PERMISSION_ID = "id";
const std::string PERMISSION_BUNDLE_NAME = "bundle_name";
const std::string PERMISSION_FILE_ID = "file_id";
const std::string PERMISSION_MODE = "mode";
const std::string PERMISSION_TABLE_TYPE = "table_type";

const std::string CREATE_MEDIA_TABLE = "CREATE TABLE IF NOT EXISTS " + MEDIALIBRARY_TABLE + " (" +
                                       MEDIA_DATA_DB_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
                                       MEDIA_DATA_DB_FILE_PATH + " TEXT, " +
                                       MEDIA_DATA_DB_SIZE + " BIGINT, " +
                                       MEDIA_DATA_DB_PARENT_ID + " INT DEFAULT 0, " +
                                       MEDIA_DATA_DB_DATE_ADDED + " BIGINT, " +
                                       MEDIA_DATA_DB_DATE_MODIFIED + " BIGINT, " +
                                       MEDIA_DATA_DB_MIME_TYPE + " TEXT, " +
                                       MEDIA_DATA_DB_TITLE + " TEXT, " +
                                       MEDIA_DATA_DB_DESCRIPTION + " TEXT, " +
                                       MEDIA_DATA_DB_NAME + " TEXT, " +
                                       MEDIA_DATA_DB_ORIENTATION + " INT DEFAULT 0, " +
                                       MEDIA_DATA_DB_LATITUDE + " DOUBLE DEFAULT 0, " +
                                       MEDIA_DATA_DB_LONGITUDE + " DOUBLE DEFAULT 0, " +
                                       MEDIA_DATA_DB_DATE_TAKEN + " BIGINT DEFAULT 0, " +
                                       MEDIA_DATA_DB_THUMBNAIL + " TEXT, " +
                                       MEDIA_DATA_DB_LCD + " TEXT, " +
                                       MEDIA_DATA_DB_BUCKET_ID + " INT DEFAULT 0, " +
                                       MEDIA_DATA_DB_BUCKET_NAME + " TEXT, " +
                                       MEDIA_DATA_DB_DURATION + " INT, " +
                                       MEDIA_DATA_DB_ARTIST + " TEXT, " +
                                       MEDIA_DATA_DB_AUDIO_ALBUM + " TEXT, " +
                                       MEDIA_DATA_DB_MEDIA_TYPE + " INT, " +
                                       MEDIA_DATA_DB_HEIGHT + " INT, " +
                                       MEDIA_DATA_DB_WIDTH + " INT, " +
                                       MEDIA_DATA_DB_IS_TRASH + " INT DEFAULT 0, " +
                                       MEDIA_DATA_DB_RECYCLE_PATH + " TEXT, " +
                                       MEDIA_DATA_DB_IS_FAV + " BOOL DEFAULT 0, " +
                                       MEDIA_DATA_DB_OWNER_PACKAGE + " TEXT, " +
                                       MEDIA_DATA_DB_PACKAGE_NAME + " TEXT, " +
                                       MEDIA_DATA_DB_DEVICE_NAME + " TEXT, " +
                                       MEDIA_DATA_DB_IS_PENDING + " BOOL DEFAULT 0, " +
                                       MEDIA_DATA_DB_TIME_PENDING + " BIGINT DEFAULT 0, " +
                                       MEDIA_DATA_DB_DATE_TRASHED + " BIGINT DEFAULT 0, " +
                                       MEDIA_DATA_DB_RELATIVE_PATH + " TEXT, " +
                                       MEDIA_DATA_DB_VOLUME_NAME + " TEXT, " +
                                       MEDIA_DATA_DB_SELF_ID + " TEXT DEFAULT '1', " +
                                       MEDIA_DATA_DB_ALBUM_NAME + " TEXT, " +
                                       MEDIA_DATA_DB_URI + " TEXT, " +
                                       MEDIA_DATA_DB_ALBUM + " TEXT, " +
                                       MEDIA_DATA_DB_CLOUD_ID + " TEXT, " +
                                       MEDIA_DATA_DB_DIRTY + " INT DEFAULT 1, " +
                                       MEDIA_DATA_DB_POSITION + " INT DEFAULT 1, " +
                                       MEDIA_DATA_DB_META_DATE_MODIFIED + "  BIGINT DEFAULT 0, " +
                                       MEDIA_DATA_DB_SYNC_STATUS + " INT DEFAULT 0)";

const std::string CREATE_BUNDLE_PREMISSION_TABLE = "CREATE TABLE IF NOT EXISTS " +
                                      BUNDLE_PERMISSION_TABLE + " (" +
                                      PERMISSION_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
                                      PERMISSION_BUNDLE_NAME + " TEXT NOT NULL, " +
                                      PERMISSION_FILE_ID + " INT NOT NULL, " +
                                      PERMISSION_MODE + " TEXT NOT NULL, " +
                                      PERMISSION_TABLE_TYPE + " INT )";

const std::string CREATE_IMAGE_VIEW = "CREATE VIEW IF NOT EXISTS Image AS SELECT " +
                                      MEDIA_DATA_DB_ID + ", " +
                                      MEDIA_DATA_DB_FILE_PATH + ", " +
                                      MEDIA_DATA_DB_SIZE + ", " +
                                      MEDIA_DATA_DB_NAME + ", " +
                                      MEDIA_DATA_DB_TITLE + ", " +
                                      MEDIA_DATA_DB_DESCRIPTION + ", " +
                                      MEDIA_DATA_DB_DATE_ADDED + ", " +
                                      MEDIA_DATA_DB_DATE_MODIFIED + ", " +
                                      MEDIA_DATA_DB_DATE_TAKEN + ", " +
                                      MEDIA_DATA_DB_MIME_TYPE + ", " +
                                      MEDIA_DATA_DB_LATITUDE + ", " +
                                      MEDIA_DATA_DB_LONGITUDE + ", " +
                                      MEDIA_DATA_DB_ORIENTATION + ", " +
                                      MEDIA_DATA_DB_WIDTH + ", " +
                                      MEDIA_DATA_DB_HEIGHT + ", " +
                                      MEDIA_DATA_DB_BUCKET_ID + ", " +
                                      MEDIA_DATA_DB_BUCKET_NAME + ", " +
                                      MEDIA_DATA_DB_THUMBNAIL + ", " +
                                      MEDIA_DATA_DB_LCD + ", " +
                                      MEDIA_DATA_DB_SELF_ID + " " +
                                      "FROM " + MEDIALIBRARY_TABLE + " WHERE " +
                                      MEDIA_DATA_DB_MEDIA_TYPE + " = 3";

const std::string CREATE_VIDEO_VIEW = "CREATE VIEW IF NOT EXISTS Video AS SELECT " +
                                      MEDIA_DATA_DB_ID + ", " +
                                      MEDIA_DATA_DB_FILE_PATH + ", " +
                                      MEDIA_DATA_DB_SIZE + ", " +
                                      MEDIA_DATA_DB_NAME + ", " +
                                      MEDIA_DATA_DB_TITLE + ", " +
                                      MEDIA_DATA_DB_DESCRIPTION + ", " +
                                      MEDIA_DATA_DB_DATE_ADDED + ", " +
                                      MEDIA_DATA_DB_DATE_MODIFIED + ", " +
                                      MEDIA_DATA_DB_MIME_TYPE + ", " +
                                      MEDIA_DATA_DB_ORIENTATION + ", " +
                                      MEDIA_DATA_DB_WIDTH + ", " +
                                      MEDIA_DATA_DB_HEIGHT + ", " +
                                      MEDIA_DATA_DB_DURATION + ", " +
                                      MEDIA_DATA_DB_BUCKET_ID + ", " +
                                      MEDIA_DATA_DB_BUCKET_NAME + ", " +
                                      MEDIA_DATA_DB_THUMBNAIL + ", " +
                                      MEDIA_DATA_DB_SELF_ID + " " +
                                      "FROM " + MEDIALIBRARY_TABLE + " WHERE " +
                                      MEDIA_DATA_DB_MEDIA_TYPE + " = 4";

const std::string CREATE_AUDIO_VIEW = "CREATE VIEW IF NOT EXISTS Audio AS SELECT " +
                                      MEDIA_DATA_DB_ID + ", " +
                                      MEDIA_DATA_DB_FILE_PATH + ", " +
                                      MEDIA_DATA_DB_SIZE + ", " +
                                      MEDIA_DATA_DB_NAME + ", " +
                                      MEDIA_DATA_DB_TITLE + ", " +
                                      MEDIA_DATA_DB_DESCRIPTION + ", " +
                                      MEDIA_DATA_DB_DATE_ADDED + ", " +
                                      MEDIA_DATA_DB_DATE_MODIFIED + ", " +
                                      MEDIA_DATA_DB_MIME_TYPE + ", " +
                                      MEDIA_DATA_DB_ARTIST + ", " +
                                      MEDIA_DATA_DB_DURATION + ", " +
                                      MEDIA_DATA_DB_BUCKET_ID + ", " +
                                      MEDIA_DATA_DB_BUCKET_NAME + ", " +
                                      MEDIA_DATA_DB_THUMBNAIL + ", " +
                                      MEDIA_DATA_DB_SELF_ID + " " +
                                      "FROM " + MEDIALIBRARY_TABLE + " WHERE " +
                                      MEDIA_DATA_DB_MEDIA_TYPE + " = 5";

const std::string REMOTE_THUMBNAIL_TABLE = "RemoteThumbnailMap";
const std::string REMOTE_THUMBNAIL_DB_ID = "id";
const std::string REMOTE_THUMBNAIL_DB_FILE_ID = MEDIA_DATA_DB_ID;
const std::string REMOTE_THUMBNAIL_DB_UDID = "udid";
const std::string CREATE_REMOTE_THUMBNAIL_TABLE = "CREATE TABLE IF NOT EXISTS " + REMOTE_THUMBNAIL_TABLE + " (" +
                                            REMOTE_THUMBNAIL_DB_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
                                            REMOTE_THUMBNAIL_DB_FILE_ID + " INT, " +
                                            REMOTE_THUMBNAIL_DB_UDID + " TEXT, " +
                                            MEDIA_DATA_DB_THUMBNAIL + " TEXT, " +
                                            MEDIA_DATA_DB_LCD + " TEXT) ";
const std::string FILE_TABLE = "file";
const std::string ALBUM_TABLE = "album";
const std::string ALBUM_VIEW_NAME = "Album";
const std::string DISTRIBUTED_ALBUM_COLUMNS = "SELECT count( " + FILE_TABLE + "." +
                                              MEDIA_DATA_DB_DATE_TRASHED + " = 0 OR NULL) AS " +
                                              MEDIA_DATA_DB_COUNT + ", " +
                                              ALBUM_TABLE + "." + MEDIA_DATA_DB_RELATIVE_PATH + ", " +
                                              ALBUM_TABLE + "." + MEDIA_DATA_DB_ID + " AS " +
                                              MEDIA_DATA_DB_BUCKET_ID + ", " +
                                              ALBUM_TABLE + "." + MEDIA_DATA_DB_FILE_PATH + ", " +
                                              ALBUM_TABLE + "." + MEDIA_DATA_DB_TITLE + " AS " +
                                              MEDIA_DATA_DB_BUCKET_NAME + ", " +
                                              ALBUM_TABLE + "." + MEDIA_DATA_DB_TITLE + ", " +
                                              ALBUM_TABLE + "." + MEDIA_DATA_DB_DESCRIPTION + ", " +
                                              ALBUM_TABLE + "." + MEDIA_DATA_DB_DATE_ADDED + ", " +
                                              ALBUM_TABLE + "." + MEDIA_DATA_DB_DATE_MODIFIED + ", " +
                                              ALBUM_TABLE + "." + MEDIA_DATA_DB_THUMBNAIL + ", " +
                                              FILE_TABLE + "." + MEDIA_DATA_DB_MEDIA_TYPE + ", " +
                                              ALBUM_TABLE + "." + MEDIA_DATA_DB_SELF_ID + ", " +
                                              ALBUM_TABLE + "." + MEDIA_DATA_DB_IS_TRASH;
const std::string DISTRIBUTED_ALBUM_WHERE_AND_GROUPBY = " WHERE " +
                                                        FILE_TABLE + "." + MEDIA_DATA_DB_BUCKET_ID + " = " +
                                                        ALBUM_TABLE + "." + MEDIA_DATA_DB_ID + " AND " +
                                                        FILE_TABLE + "." + MEDIA_DATA_DB_MEDIA_TYPE + " <> " +
                                                        std::to_string(MEDIA_TYPE_ALBUM) + " AND " +
                                                        FILE_TABLE + "." + MEDIA_DATA_DB_MEDIA_TYPE + " <> " +
                                                        std::to_string(MEDIA_TYPE_FILE) +
                                                        " GROUP BY " +
                                                        FILE_TABLE + "." + MEDIA_DATA_DB_BUCKET_ID +", " +
                                                        FILE_TABLE + "." + MEDIA_DATA_DB_BUCKET_NAME + ", " +
                                                        FILE_TABLE + "." + MEDIA_DATA_DB_MEDIA_TYPE + ", " +
                                                        ALBUM_TABLE + "." + MEDIA_DATA_DB_SELF_ID;
const std::string CREATE_ALBUM_VIEW = "CREATE VIEW IF NOT EXISTS " + ALBUM_VIEW_NAME +
                                      " AS " + DISTRIBUTED_ALBUM_COLUMNS +
                                      " FROM " + MEDIALIBRARY_TABLE + " " + FILE_TABLE + ", " +
                                      MEDIALIBRARY_TABLE + " " + ALBUM_TABLE +
                                      DISTRIBUTED_ALBUM_WHERE_AND_GROUPBY;
const std::string SMARTALBUM_DB_ID = "album_id";
const std::string SMARTALBUM_DB_ALBUM_TYPE = "album_type";
const std::string SMARTALBUM_DB_NAME = "album_name";
const std::string SMARTALBUM_DB_DESCRIPTION = "description";
const std::string SMARTALBUM_DB_CAPACITY = "capacity";
const std::string SMARTALBUM_DB_LATITUDE = "latitude";
const std::string SMARTALBUM_DB_LONGITUDE = "longitude";
const std::string SMARTALBUM_DB_COVER_URI = "cover_uri";
const std::string SMARTALBUM_DB_EXPIRED_TIME = "expired_id";
const std::string SMARTALBUM_DB_SELF_ID = "self_id";
const std::string CREATE_SMARTALBUM_TABLE = "CREATE TABLE IF NOT EXISTS " + SMARTALBUM_TABLE + " (" +
                                            SMARTALBUM_DB_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
                                            SMARTALBUM_DB_NAME + " TEXT, " +
                                            SMARTALBUM_DB_DESCRIPTION + " TEXT, " +
                                            SMARTALBUM_DB_ALBUM_TYPE + " INT, " +
                                            SMARTALBUM_DB_CAPACITY + " INT, " +
                                            SMARTALBUM_DB_LATITUDE + " DOUBLE DEFAULT 0, " +
                                            SMARTALBUM_DB_LONGITUDE + " DOUBLE DEFAULT 0, " +
                                            SMARTALBUM_DB_COVER_URI + " TEXT, " +
                                            SMARTALBUM_DB_EXPIRED_TIME + " INT DEFAULT 30, " +
                                            SMARTALBUM_DB_SELF_ID + " TEXT) ";

const std::string SMARTALBUMMAP_DB_ID = "map_id";
const std::string SMARTALBUMMAP_DB_ALBUM_ID = "album_id";
const std::string SMARTALBUMMAP_DB_CHILD_ALBUM_ID = "child_album_id";
const std::string SMARTALBUMMAP_DB_CHILD_ASSET_ID = "child_asset_id";
const std::string SMARTALBUMMAP_DB_SELF_ID = "self_id";
const std::string CREATE_SMARTALBUMMAP_TABLE = "CREATE TABLE IF NOT EXISTS " + SMARTALBUM_MAP_TABLE + " (" +
                                            SMARTALBUMMAP_DB_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
                                            SMARTALBUMMAP_DB_ALBUM_ID + " INT, " +
                                            SMARTALBUMMAP_DB_CHILD_ALBUM_ID + " INT, " +
                                            SMARTALBUMMAP_DB_CHILD_ASSET_ID + " INT, " +
                                            SMARTALBUMMAP_DB_SELF_ID + " TEXT) ";
const std::string CATEGORY_SMARTALBUMMAP_DB_ID = "category_map_id";
const std::string CATEGORY_SMARTALBUMMAP_DB_CATEGORY_ID = "category_id";
const std::string CATEGORY_SMARTALBUMMAP_DB_CATEGORY_NAME = "category_name";
const std::string CATEGORY_SMARTALBUMMAP_DB_ALBUM_ID = "album_id";
const std::string CATEGORY_SMARTALBUMMAP_DB_SELF_ID = "self_id";
const std::string CREATE_CATEGORY_SMARTALBUMMAP_TABLE = "CREATE TABLE IF NOT EXISTS " +
                                            CATEGORY_SMARTALBUM_MAP_TABLE + " (" +
                                            CATEGORY_SMARTALBUMMAP_DB_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
                                            CATEGORY_SMARTALBUMMAP_DB_CATEGORY_ID + " INT, " +
                                            CATEGORY_SMARTALBUMMAP_DB_CATEGORY_NAME + " TEXT, " +
                                            CATEGORY_SMARTALBUMMAP_DB_ALBUM_ID + " INT, " +
                                            SMARTALBUMMAP_DB_SELF_ID + " TEXT) ";
const std::string DIRECTORY_DB_DIRECTORY_TYPE = "directory_type";
const std::string DIRECTORY_DB_MEDIA_TYPE = "media_type";
const std::string DIRECTORY_DB_DIRECTORY = "directory";
const std::string DIRECTORY_DB_EXTENSION = "extension";
const std::string CREATE_MEDIATYPE_DIRECTORY_TABLE = "CREATE TABLE IF NOT EXISTS " +
                                            MEDIATYPE_DIRECTORY_TABLE + " (" +
                                            DIRECTORY_DB_DIRECTORY_TYPE + " INTEGER PRIMARY KEY, " +
                                            DIRECTORY_DB_MEDIA_TYPE + " TEXT, " +
                                            DIRECTORY_DB_DIRECTORY + " TEXT, " +
                                            DIRECTORY_DB_EXTENSION + " TEXT) ";

const std::string DEVICE_DB_ID = "id";
const std::string DEVICE_DB_UDID = "device_udid";
const std::string DEVICE_DB_NETWORK_ID = "network_id";
const std::string DEVICE_DB_NAME = "device_name";
const std::string DEVICE_DB_IP = "device_ip";
const std::string DEVICE_DB_SYNC_STATUS = "sync_status";
const std::string DEVICE_DB_PHOTO_SYNC_STATUS = "photo_table_sync_status";
const std::string DEVICE_DB_SELF_ID = "self_id";
const std::string DEVICE_DB_TYPE = "device_type";
const std::string DEVICE_DB_PREPATH = "pre_path";
const std::string DEVICE_DB_DATE_ADDED = "date_added";
const std::string DEVICE_DB_DATE_MODIFIED = "date_modified";
const std::string CREATE_DEVICE_TABLE = "CREATE TABLE IF NOT EXISTS " + DEVICE_TABLE + " (" +
                                            DEVICE_DB_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
                                            DEVICE_DB_UDID + " TEXT, " +
                                            DEVICE_DB_NETWORK_ID + " TEXT, " +
                                            DEVICE_DB_NAME + " TEXT, " +
                                            DEVICE_DB_IP + " TEXT DEFAULT '', " +
                                            DEVICE_DB_SYNC_STATUS + " INT DEFAULT 0, " +
                                            DEVICE_DB_PHOTO_SYNC_STATUS + " INT DEFAULT 0, " +
                                            DEVICE_DB_SELF_ID + " TEXT, " +
                                            DEVICE_DB_TYPE + " INT, " +
                                            DEVICE_DB_PREPATH + " TEXT, " +
                                            DEVICE_DB_DATE_ADDED + " BIGINT DEFAULT 0, " +
                                            DEVICE_DB_DATE_MODIFIED + " BIGINT DEFAULT 0) ";
const std::string SMARTALBUM_TABLE_NAME = "smartalbum";
const std::string SMARTALBUMMAP_TABLE_NAME = "smartAlbumMap";
const std::string CATEGORY_SMARTALBUMMAP_TABLE_NAME = "categorySmartAlbumMap";
const std::string SMARTALBUMASSETS_VIEW_NAME = "SmartAsset";
const std::string SMARTALBUMASSETS_ALBUMCAPACITY = "size";
const std::string SMARTABLUMASSETS_PARENTID = "parentid";
const std::string CREATE_SMARTALBUMASSETS_VIEW = "CREATE VIEW IF NOT EXISTS " + SMARTALBUMASSETS_VIEW_NAME +
                        " AS SELECT COUNT(" +
                        MEDIALIBRARY_TABLE + "."+ MEDIA_DATA_DB_DATE_TRASHED + " = 0 OR (" +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_ID + " = " +
                        std::to_string(TRASH_ALBUM_ID_VALUES) + " AND " +
                        MEDIALIBRARY_TABLE + "."+ MEDIA_DATA_DB_IS_TRASH + "<> 0)" +
                        " OR NULL ) AS " + SMARTALBUMASSETS_ALBUMCAPACITY + ", " +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_ID + ", " +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_NAME + ", " +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_SELF_ID + ", " +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_DESCRIPTION + ", " +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_EXPIRED_TIME + ", " +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_COVER_URI + ", " +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_ALBUM_TYPE + ", " +
                        SMARTALBUM_MAP_TABLE + "." + SMARTALBUMMAP_DB_ID + ", " +
                        "a." + SMARTALBUMMAP_DB_ALBUM_ID + " AS " + SMARTABLUMASSETS_PARENTID +
                        " FROM " + SMARTALBUM_TABLE + " " + SMARTALBUM_TABLE_NAME +
                        " LEFT JOIN " + SMARTALBUM_MAP_TABLE +
                        " ON " + SMARTALBUM_MAP_TABLE + "." + SMARTALBUMMAP_DB_ALBUM_ID + " = " +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_ID +
                        " LEFT JOIN " + MEDIALIBRARY_TABLE +
                        " ON " + SMARTALBUM_MAP_TABLE + "." + SMARTALBUMMAP_DB_CHILD_ASSET_ID + " = " +
                        MEDIALIBRARY_TABLE + "." + MEDIA_DATA_DB_ID +
                        " LEFT JOIN " + SMARTALBUM_MAP_TABLE + " a " +
                        " ON a." + SMARTALBUMMAP_DB_CHILD_ALBUM_ID + " = " +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_ID +
                        " GROUP BY IFNULL( " + SMARTALBUM_MAP_TABLE + "." + SMARTALBUMMAP_DB_ALBUM_ID + ", " +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_ID + " ), " +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_SELF_ID;
const std::string ASSETMAP_VIEW_NAME = "AssetMap";
const std::string CREATE_ASSETMAP_VIEW = "CREATE VIEW IF NOT EXISTS " + ASSETMAP_VIEW_NAME +
                        " AS SELECT * FROM " +
                        MEDIALIBRARY_TABLE + " a " + ", " +
                        SMARTALBUM_MAP_TABLE + " b " +
                        " WHERE " +
                        "a." + MEDIA_DATA_DB_ID + " = " +
                        "b." + SMARTALBUMMAP_DB_CHILD_ASSET_ID;

const std::string QUERY_MEDIA_VOLUME = "SELECT sum(" + MEDIA_DATA_DB_SIZE + ") AS " +
                        MEDIA_DATA_DB_SIZE + "," +
                        MEDIA_DATA_DB_MEDIA_TYPE + " FROM " +
                        MEDIALIBRARY_TABLE + " WHERE " +
                        MEDIA_DATA_DB_MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_FILE) + " OR " +
                        MEDIA_DATA_DB_MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_IMAGE) + " OR " +
                        MEDIA_DATA_DB_MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_VIDEO) + " OR " +
                        MEDIA_DATA_DB_MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_ALBUM) + " OR " +
                        MEDIA_DATA_DB_MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_AUDIO) + " GROUP BY " +
                        MEDIA_DATA_DB_MEDIA_TYPE;

const std::string CREATE_FILES_DELETE_TRIGGER = "CREATE TRIGGER delete_trigger AFTER UPDATE ON " +
                        MEDIALIBRARY_TABLE + " FOR EACH ROW WHEN new.dirty = " +
                        std::to_string(static_cast<int32_t>(DirtyType::TYPE_DELETED)) +
                        " and OLD.cloud_id is NULL AND is_caller_self_func() = 'true'" +
                        " BEGIN " +
                        " DELETE FROM " + MEDIALIBRARY_TABLE + " WHERE file_id = old.file_id;" +
                        " END;";

const std::string CREATE_FILES_FDIRTY_TRIGGER = "CREATE TRIGGER fdirty_trigger AFTER UPDATE ON " +
                        MEDIALIBRARY_TABLE + " FOR EACH ROW WHEN OLD.cloud_id IS NOT NULL AND" +
                        " new.date_modified <> old.date_modified AND is_caller_self_func() = 'true'" +
                        " BEGIN " +
                        " UPDATE " + MEDIALIBRARY_TABLE + " SET dirty = " +
                        std::to_string(static_cast<int32_t>(DirtyType::TYPE_FDIRTY)) +
                        " WHERE file_id = old.file_id;" +
                        " SELECT cloud_sync_func(); " +
                        " END;";

const std::string CREATE_FILES_MDIRTY_TRIGGER = "CREATE TRIGGER mdirty_trigger AFTER UPDATE ON " +
                        MEDIALIBRARY_TABLE + " FOR EACH ROW WHEN OLD.cloud_id IS NOT NULL" +
                        " AND new.date_modified = old.date_modified AND old.dirty = " +
                        std::to_string(static_cast<int32_t>(DirtyType::TYPE_SYNCED)) +
                        " AND new.dirty = old.dirty AND is_caller_self_func() = 'true'" +
                        " BEGIN " +
                        " UPDATE " + MEDIALIBRARY_TABLE + " SET dirty = " +
                        std::to_string(static_cast<int32_t>(DirtyType::TYPE_MDIRTY)) +
                        " WHERE file_id = old.file_id;" +
                        " SELECT cloud_sync_func(); " +
                        " END;";

const std::string CREATE_INSERT_CLOUD_SYNC_TRIGGER =
                        " CREATE TRIGGER insert_cloud_sync_trigger AFTER INSERT ON " + MEDIALIBRARY_TABLE +
                        " BEGIN SELECT cloud_sync_func(); END;";

/*
 * Error Table
 */
const std::string MEDIALIBRARY_ERROR_TABLE = "Error";
const std::string MEDIA_DATA_ERROR = "err";
const std::string CREATE_MEDIALIBRARY_ERROR_TABLE = "CREATE TABLE IF NOT EXISTS " + MEDIALIBRARY_ERROR_TABLE + " ("
    + MEDIA_DATA_ERROR + " TEXT PRIMARY KEY)";

/*
 * Media Unique Number Table
 */
const std::string ASSET_UNIQUE_NUMBER_TABLE = "UniqueNumber";
const std::string ASSET_MEDIA_TYPE = "media_type";
const std::string UNIQUE_NUMBER = "unique_number";
const std::string CREATE_ASSET_UNIQUE_NUMBER_TABLE = "CREATE TABLE IF NOT EXISTS " + ASSET_UNIQUE_NUMBER_TABLE + " (" +
    ASSET_MEDIA_TYPE + " TEXT, " + UNIQUE_NUMBER + " INT DEFAULT 0) ";

const std::string IMAGE_ASSET_TYPE = "image";
const std::string VIDEO_ASSET_TYPE = "video";
const std::string AUDIO_ASSET_TYPE = "audio";

// fetch columns from fileAsset in medialibrary.d.ts
static const std::vector<std::string> FILE_ASSET_COLUMNS = {
    MEDIA_DATA_DB_ID, MEDIA_DATA_DB_FILE_PATH, MEDIA_DATA_DB_URI, MEDIA_DATA_DB_MIME_TYPE, MEDIA_DATA_DB_MEDIA_TYPE,
    MEDIA_DATA_DB_NAME, MEDIA_DATA_DB_TITLE, MEDIA_DATA_DB_RELATIVE_PATH, MEDIA_DATA_DB_PARENT_ID, MEDIA_DATA_DB_SIZE,
    MEDIA_DATA_DB_DATE_ADDED, MEDIA_DATA_DB_DATE_MODIFIED, MEDIA_DATA_DB_DATE_TAKEN, MEDIA_DATA_DB_ARTIST,
    MEDIA_DATA_DB_WIDTH, MEDIA_DATA_DB_HEIGHT, MEDIA_DATA_DB_ORIENTATION, MEDIA_DATA_DB_DURATION,
    MEDIA_DATA_DB_BUCKET_ID, MEDIA_DATA_DB_BUCKET_NAME, MEDIA_DATA_DB_IS_TRASH, MEDIA_DATA_DB_IS_FAV,
    MEDIA_DATA_DB_DATE_TRASHED
};

const std::string EMPTY_COLUMN_AS = "'' AS ";
const std::string DEFAULT_INT_COLUMN_AS = "0 AS ";
const std::string COMPAT_COLUMN_ARTIST = EMPTY_COLUMN_AS + MEDIA_DATA_DB_ARTIST;
const std::string COMPAT_COLUMN_AUDIO_ALBUM = EMPTY_COLUMN_AS + MEDIA_DATA_DB_AUDIO_ALBUM;
const std::string COMPAT_COLUMN_ORIENTATION = DEFAULT_INT_COLUMN_AS + MEDIA_DATA_DB_ORIENTATION;
const std::string COMPAT_COLUMN_BUCKET_ID = DEFAULT_INT_COLUMN_AS + MEDIA_DATA_DB_BUCKET_ID;
const std::string COMPAT_COLUMN_BUCKET_NAME = EMPTY_COLUMN_AS + MEDIA_DATA_DB_BUCKET_NAME;
const std::string COMPAT_COLUMN_IS_TRASH = MEDIA_DATA_DB_DATE_TRASHED + " AS " + MEDIA_DATA_DB_IS_TRASH;
const std::string COMPAT_COLUMN_WIDTH = DEFAULT_INT_COLUMN_AS + MEDIA_DATA_DB_WIDTH;
const std::string COMPAT_COLUMN_HEIGHT = DEFAULT_INT_COLUMN_AS + MEDIA_DATA_DB_HEIGHT;
const std::string COMPAT_COLUMN_URI = EMPTY_COLUMN_AS + MEDIA_DATA_DB_URI;

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

// delete_tool
const std::string DELETE_TOOL_ONLY_DATABASE = "only_db";

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
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_DATA_ABILITY_CONST_H_
