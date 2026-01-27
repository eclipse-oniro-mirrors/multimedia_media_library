/*
 * Copyright (C) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MEDIALIBRARY_DB_CONST_SQLS_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MEDIALIBRARY_DB_CONST_SQLS_H

#include "location_db_sqls.h"
#include "media_category_smart_album_map_column.h"
#include "media_device_column.h"
#include "media_directory_type_column.h"
#include "media_error_column.h"
#include "media_refresh_album_column.h"
#include "media_remote_thumbnail_column.h"
#include "media_smart_album_column.h"
#include "media_smart_map_column.h"
#include "media_unique_number_column.h"
#include "medialibrary_db_const.h"
#include "vision_db_sqls_more.h"
#include "smart_album_column.h"
#include "media_album_order_back.h"
#include "media_lake_album.h"

namespace OHOS {
namespace Media {

const std::string FILE_TABLE = "file";

const std::string CREATE_MEDIA_TABLE = std::string("CREATE TABLE IF NOT EXISTS ") + CONST_MEDIALIBRARY_TABLE + " (" +
                                       CONST_MEDIA_DATA_DB_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
                                       CONST_MEDIA_DATA_DB_FILE_PATH + " TEXT, " +
                                       CONST_MEDIA_DATA_DB_SIZE + " BIGINT, " +
                                       CONST_MEDIA_DATA_DB_PARENT_ID + " INT DEFAULT 0, " +
                                       CONST_MEDIA_DATA_DB_DATE_ADDED + " BIGINT, " +
                                       CONST_MEDIA_DATA_DB_DATE_MODIFIED + " BIGINT, " +
                                       CONST_MEDIA_DATA_DB_MIME_TYPE + " TEXT, " +
                                       CONST_MEDIA_DATA_DB_TITLE + " TEXT, " +
                                       CONST_MEDIA_DATA_DB_DESCRIPTION + " TEXT, " +
                                       CONST_MEDIA_DATA_DB_NAME + " TEXT, " +
                                       CONST_MEDIA_DATA_DB_ORIENTATION + " INT DEFAULT 0, " +
                                       CONST_MEDIA_DATA_DB_LATITUDE + " DOUBLE DEFAULT 0, " +
                                       CONST_MEDIA_DATA_DB_LONGITUDE + " DOUBLE DEFAULT 0, " +
                                       CONST_MEDIA_DATA_DB_DATE_TAKEN + " BIGINT DEFAULT 0, " +
                                       CONST_MEDIA_DATA_DB_THUMBNAIL + " TEXT, " +
                                       CONST_MEDIA_DATA_DB_LCD + " TEXT, " +
                                       CONST_MEDIA_DATA_DB_BUCKET_ID + " INT DEFAULT 0, " +
                                       CONST_MEDIA_DATA_DB_BUCKET_NAME + " TEXT, " +
                                       CONST_MEDIA_DATA_DB_DURATION + " INT, " +
                                       CONST_MEDIA_DATA_DB_ARTIST + " TEXT, " +
                                       CONST_MEDIA_DATA_DB_AUDIO_ALBUM + " TEXT, " +
                                       CONST_MEDIA_DATA_DB_MEDIA_TYPE + " INT, " +
                                       CONST_MEDIA_DATA_DB_HEIGHT + " INT, " +
                                       CONST_MEDIA_DATA_DB_WIDTH + " INT, " +
                                       CONST_MEDIA_DATA_DB_IS_TRASH + " INT DEFAULT 0, " +
                                       CONST_MEDIA_DATA_DB_RECYCLE_PATH + " TEXT, " +
                                       CONST_MEDIA_DATA_DB_IS_FAV + " BOOL DEFAULT 0, " +
                                       CONST_MEDIA_DATA_DB_OWNER_PACKAGE + " TEXT, " +
                                       CONST_MEDIA_DATA_DB_OWNER_APPID + " TEXT, " +
                                       CONST_MEDIA_DATA_DB_PACKAGE_NAME + " TEXT, " +
                                       CONST_MEDIA_DATA_DB_DEVICE_NAME + " TEXT, " +
                                       CONST_MEDIA_DATA_DB_IS_PENDING + " BOOL DEFAULT 0, " +
                                       CONST_MEDIA_DATA_DB_TIME_PENDING + " BIGINT DEFAULT 0, " +
                                       CONST_MEDIA_DATA_DB_DATE_TRASHED + " BIGINT DEFAULT 0, " +
                                       CONST_MEDIA_DATA_DB_RELATIVE_PATH + " TEXT, " +
                                       CONST_MEDIA_DATA_DB_VOLUME_NAME + " TEXT, " +
                                       CONST_MEDIA_DATA_DB_SELF_ID + " TEXT DEFAULT '1', " +
                                       CONST_MEDIA_DATA_DB_ALBUM_NAME + " TEXT, " +
                                       CONST_MEDIA_DATA_DB_URI + " TEXT, " +
                                       CONST_MEDIA_DATA_DB_ALBUM + " TEXT, " +
                                       CONST_MEDIA_DATA_DB_CLOUD_ID + " TEXT, " +
                                       CONST_MEDIA_DATA_DB_DIRTY + " INT DEFAULT 1, " +
                                       CONST_MEDIA_DATA_DB_POSITION + " INT DEFAULT 1, " +
                                       CONST_MEDIA_DATA_DB_META_DATE_MODIFIED + "  BIGINT DEFAULT 0, " +
                                       CONST_MEDIA_DATA_DB_SYNC_STATUS + " INT DEFAULT 0)";

const std::string CREATE_BUNDLE_PREMISSION_TABLE = std::string("CREATE TABLE IF NOT EXISTS ") +
                                      CONST_BUNDLE_PERMISSION_TABLE + " (" +
                                      CONST_PERMISSION_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
                                      CONST_PERMISSION_BUNDLE_NAME + " TEXT NOT NULL, " +
                                      CONST_PERMISSION_FILE_ID + " INT NOT NULL, " +
                                      CONST_PERMISSION_MODE + " TEXT NOT NULL, " +
                                      CONST_PERMISSION_TABLE_TYPE + " INT )";

const std::string CREATE_IMAGE_VIEW = std::string("CREATE VIEW IF NOT EXISTS Image AS SELECT ") +
                                      CONST_MEDIA_DATA_DB_ID + ", " +
                                      CONST_MEDIA_DATA_DB_FILE_PATH + ", " +
                                      CONST_MEDIA_DATA_DB_SIZE + ", " +
                                      CONST_MEDIA_DATA_DB_NAME + ", " +
                                      CONST_MEDIA_DATA_DB_TITLE + ", " +
                                      CONST_MEDIA_DATA_DB_DESCRIPTION + ", " +
                                      CONST_MEDIA_DATA_DB_DATE_ADDED + ", " +
                                      CONST_MEDIA_DATA_DB_DATE_MODIFIED + ", " +
                                      CONST_MEDIA_DATA_DB_DATE_TAKEN + ", " +
                                      CONST_MEDIA_DATA_DB_MIME_TYPE + ", " +
                                      CONST_MEDIA_DATA_DB_LATITUDE + ", " +
                                      CONST_MEDIA_DATA_DB_LONGITUDE + ", " +
                                      CONST_MEDIA_DATA_DB_ORIENTATION + ", " +
                                      CONST_MEDIA_DATA_DB_WIDTH + ", " +
                                      CONST_MEDIA_DATA_DB_HEIGHT + ", " +
                                      CONST_MEDIA_DATA_DB_BUCKET_ID + ", " +
                                      CONST_MEDIA_DATA_DB_BUCKET_NAME + ", " +
                                      CONST_MEDIA_DATA_DB_THUMBNAIL + ", " +
                                      CONST_MEDIA_DATA_DB_LCD + ", " +
                                      CONST_MEDIA_DATA_DB_SELF_ID + " " +
                                      "FROM " + CONST_MEDIALIBRARY_TABLE + " WHERE " +
                                      CONST_MEDIA_DATA_DB_MEDIA_TYPE + " = 3";

const std::string CREATE_VIDEO_VIEW = std::string("CREATE VIEW IF NOT EXISTS Video AS SELECT ") +
                                      CONST_MEDIA_DATA_DB_ID + ", " +
                                      CONST_MEDIA_DATA_DB_FILE_PATH + ", " +
                                      CONST_MEDIA_DATA_DB_SIZE + ", " +
                                      CONST_MEDIA_DATA_DB_NAME + ", " +
                                      CONST_MEDIA_DATA_DB_TITLE + ", " +
                                      CONST_MEDIA_DATA_DB_DESCRIPTION + ", " +
                                      CONST_MEDIA_DATA_DB_DATE_ADDED + ", " +
                                      CONST_MEDIA_DATA_DB_DATE_MODIFIED + ", " +
                                      CONST_MEDIA_DATA_DB_MIME_TYPE + ", " +
                                      CONST_MEDIA_DATA_DB_ORIENTATION + ", " +
                                      CONST_MEDIA_DATA_DB_WIDTH + ", " +
                                      CONST_MEDIA_DATA_DB_HEIGHT + ", " +
                                      CONST_MEDIA_DATA_DB_DURATION + ", " +
                                      CONST_MEDIA_DATA_DB_BUCKET_ID + ", " +
                                      CONST_MEDIA_DATA_DB_BUCKET_NAME + ", " +
                                      CONST_MEDIA_DATA_DB_THUMBNAIL + ", " +
                                      CONST_MEDIA_DATA_DB_SELF_ID + " " +
                                      "FROM " + CONST_MEDIALIBRARY_TABLE + " WHERE " +
                                      CONST_MEDIA_DATA_DB_MEDIA_TYPE + " = 4";

const std::string CREATE_AUDIO_VIEW = std::string("CREATE VIEW IF NOT EXISTS Audio AS SELECT ") +
                                      CONST_MEDIA_DATA_DB_ID + ", " +
                                      CONST_MEDIA_DATA_DB_FILE_PATH + ", " +
                                      CONST_MEDIA_DATA_DB_SIZE + ", " +
                                      CONST_MEDIA_DATA_DB_NAME + ", " +
                                      CONST_MEDIA_DATA_DB_TITLE + ", " +
                                      CONST_MEDIA_DATA_DB_DESCRIPTION + ", " +
                                      CONST_MEDIA_DATA_DB_DATE_ADDED + ", " +
                                      CONST_MEDIA_DATA_DB_DATE_MODIFIED + ", " +
                                      CONST_MEDIA_DATA_DB_MIME_TYPE + ", " +
                                      CONST_MEDIA_DATA_DB_ARTIST + ", " +
                                      CONST_MEDIA_DATA_DB_DURATION + ", " +
                                      CONST_MEDIA_DATA_DB_BUCKET_ID + ", " +
                                      CONST_MEDIA_DATA_DB_BUCKET_NAME + ", " +
                                      CONST_MEDIA_DATA_DB_THUMBNAIL + ", " +
                                      CONST_MEDIA_DATA_DB_SELF_ID + " " +
                                      "FROM " + CONST_MEDIALIBRARY_TABLE + " WHERE " +
                                      CONST_MEDIA_DATA_DB_MEDIA_TYPE + " = 5";

const std::string CREATE_REMOTE_THUMBNAIL_TABLE = "CREATE TABLE IF NOT EXISTS " + REMOTE_THUMBNAIL_TABLE + " (" +
                                            REMOTE_THUMBNAIL_DB_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
                                            REMOTE_THUMBNAIL_DB_FILE_ID + " INT, " +
                                            REMOTE_THUMBNAIL_DB_UDID + " TEXT, " +
                                            CONST_MEDIA_DATA_DB_THUMBNAIL + " TEXT, " +
                                            CONST_MEDIA_DATA_DB_LCD + " TEXT) ";

const std::string DISTRIBUTED_ALBUM_COLUMNS = "SELECT count( " + FILE_TABLE + "." +
                                              CONST_MEDIA_DATA_DB_DATE_TRASHED + " = 0 OR NULL) AS " +
                                              CONST_MEDIA_DATA_DB_COUNT + ", " +
                                              CONST_ALBUM_TABLE + "." + CONST_MEDIA_DATA_DB_RELATIVE_PATH + ", " +
                                              CONST_ALBUM_TABLE + "." + CONST_MEDIA_DATA_DB_ID + " AS " +
                                              CONST_MEDIA_DATA_DB_BUCKET_ID + ", " +
                                              CONST_ALBUM_TABLE + "." + CONST_MEDIA_DATA_DB_FILE_PATH + ", " +
                                              CONST_ALBUM_TABLE + "." + CONST_MEDIA_DATA_DB_TITLE + " AS " +
                                              CONST_MEDIA_DATA_DB_BUCKET_NAME + ", " +
                                              CONST_ALBUM_TABLE + "." + CONST_MEDIA_DATA_DB_TITLE + ", " +
                                              CONST_ALBUM_TABLE + "." + CONST_MEDIA_DATA_DB_DESCRIPTION + ", " +
                                              CONST_ALBUM_TABLE + "." + CONST_MEDIA_DATA_DB_DATE_ADDED + ", " +
                                              CONST_ALBUM_TABLE + "." + CONST_MEDIA_DATA_DB_DATE_MODIFIED + ", " +
                                              CONST_ALBUM_TABLE + "." + CONST_MEDIA_DATA_DB_THUMBNAIL + ", " +
                                              FILE_TABLE + "." + CONST_MEDIA_DATA_DB_MEDIA_TYPE + ", " +
                                              CONST_ALBUM_TABLE + "." + CONST_MEDIA_DATA_DB_SELF_ID + ", " +
                                              CONST_ALBUM_TABLE + "." + CONST_MEDIA_DATA_DB_IS_TRASH;

const std::string DISTRIBUTED_ALBUM_WHERE_AND_GROUPBY = " WHERE " +
                                                        FILE_TABLE + "." + CONST_MEDIA_DATA_DB_BUCKET_ID + " = " +
                                                        CONST_ALBUM_TABLE + "." + CONST_MEDIA_DATA_DB_ID + " AND " +
                                                        FILE_TABLE + "." + CONST_MEDIA_DATA_DB_MEDIA_TYPE + " <> " +
                                                        std::to_string(MEDIA_TYPE_ALBUM) + " AND " +
                                                        FILE_TABLE + "." + CONST_MEDIA_DATA_DB_MEDIA_TYPE + " <> " +
                                                        std::to_string(MEDIA_TYPE_FILE) +
                                                        " GROUP BY " +
                                                        FILE_TABLE + "." + CONST_MEDIA_DATA_DB_BUCKET_ID +", " +
                                                        FILE_TABLE + "." + CONST_MEDIA_DATA_DB_BUCKET_NAME + ", " +
                                                        FILE_TABLE + "." + CONST_MEDIA_DATA_DB_MEDIA_TYPE + ", " +
                                                        CONST_ALBUM_TABLE + "." + CONST_MEDIA_DATA_DB_SELF_ID;

const std::string CREATE_ALBUM_VIEW = std::string("CREATE VIEW IF NOT EXISTS ") + CONST_ALBUM_VIEW_NAME +
                                      " AS " + DISTRIBUTED_ALBUM_COLUMNS +
                                      " FROM " + CONST_MEDIALIBRARY_TABLE + " " + FILE_TABLE + ", " +
                                      CONST_MEDIALIBRARY_TABLE + " " + CONST_ALBUM_TABLE +
                                      DISTRIBUTED_ALBUM_WHERE_AND_GROUPBY;

const std::string CREATE_SMARTALBUM_TABLE = std::string("CREATE TABLE IF NOT EXISTS ") + CONST_SMARTALBUM_TABLE + " (" +
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

const std::string CREATE_SMARTALBUMMAP_TABLE = std::string("CREATE TABLE IF NOT EXISTS ") + CONST_SMARTALBUM_MAP_TABLE +
                                            " (" + SMARTALBUMMAP_DB_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
                                            SMARTALBUMMAP_DB_ALBUM_ID + " INT, " +
                                            SMARTALBUMMAP_DB_CHILD_ALBUM_ID + " INT, " +
                                            SMARTALBUMMAP_DB_CHILD_ASSET_ID + " INT, " +
                                            SMARTALBUMMAP_DB_SELF_ID + " TEXT) ";

const std::string CREATE_CATEGORY_SMARTALBUMMAP_TABLE = std::string("CREATE TABLE IF NOT EXISTS ") +
                                            CONST_CATEGORY_SMARTALBUM_MAP_TABLE + " (" +
                                            CATEGORY_SMARTALBUMMAP_DB_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
                                            CATEGORY_SMARTALBUMMAP_DB_CATEGORY_ID + " INT, " +
                                            CATEGORY_SMARTALBUMMAP_DB_CATEGORY_NAME + " TEXT, " +
                                            CATEGORY_SMARTALBUMMAP_DB_ALBUM_ID + " INT, " +
                                            SMARTALBUMMAP_DB_SELF_ID + " TEXT) ";

const std::string CREATE_MEDIATYPE_DIRECTORY_TABLE = std::string("CREATE TABLE IF NOT EXISTS ") +
                                            CONST_MEDIATYPE_DIRECTORY_TABLE + " (" +
                                            DIRECTORY_DB_DIRECTORY_TYPE + " INTEGER PRIMARY KEY, " +
                                            DIRECTORY_DB_MEDIA_TYPE + " TEXT, " +
                                            DIRECTORY_DB_DIRECTORY + " TEXT, " +
                                            DIRECTORY_DB_EXTENSION + " TEXT) ";

const std::string CREATE_DEVICE_TABLE = std::string("CREATE TABLE IF NOT EXISTS ") + CONST_DEVICE_TABLE + " (" +
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

const std::string CREATE_SMARTALBUMASSETS_VIEW = "CREATE VIEW IF NOT EXISTS " + SMARTALBUMASSETS_VIEW_NAME +
                        " AS SELECT COUNT(" +
                        CONST_MEDIALIBRARY_TABLE + "."+ CONST_MEDIA_DATA_DB_DATE_TRASHED + " = 0 OR (" +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_ID + " = " +
                        std::to_string(TRASH_ALBUM_ID_VALUES) + " AND " +
                        CONST_MEDIALIBRARY_TABLE + "."+ CONST_MEDIA_DATA_DB_IS_TRASH + "<> 0)" +
                        " OR NULL ) AS " + SMARTALBUMASSETS_ALBUMCAPACITY + ", " +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_ID + ", " +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_NAME + ", " +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_SELF_ID + ", " +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_DESCRIPTION + ", " +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_EXPIRED_TIME + ", " +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_COVER_URI + ", " +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_ALBUM_TYPE + ", " +
                        CONST_SMARTALBUM_MAP_TABLE + "." + SMARTALBUMMAP_DB_ID + ", " +
                        "a." + SMARTALBUMMAP_DB_ALBUM_ID + " AS " + SMARTABLUMASSETS_PARENTID +
                        " FROM " + CONST_SMARTALBUM_TABLE + " " + SMARTALBUM_TABLE_NAME +
                        " LEFT JOIN " + CONST_SMARTALBUM_MAP_TABLE +
                        " ON " + CONST_SMARTALBUM_MAP_TABLE + "." + SMARTALBUMMAP_DB_ALBUM_ID + " = " +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_ID +
                        " LEFT JOIN " + CONST_MEDIALIBRARY_TABLE +
                        " ON " + CONST_SMARTALBUM_MAP_TABLE + "." + SMARTALBUMMAP_DB_CHILD_ASSET_ID + " = " +
                        CONST_MEDIALIBRARY_TABLE + "." + CONST_MEDIA_DATA_DB_ID +
                        " LEFT JOIN " + CONST_SMARTALBUM_MAP_TABLE + " a " +
                        " ON a." + SMARTALBUMMAP_DB_CHILD_ALBUM_ID + " = " +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_ID +
                        " GROUP BY IFNULL( " + CONST_SMARTALBUM_MAP_TABLE + "." + SMARTALBUMMAP_DB_ALBUM_ID + ", " +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_ID + " ), " +
                        SMARTALBUM_TABLE_NAME + "." + SMARTALBUM_DB_SELF_ID;

const std::string CREATE_ASSETMAP_VIEW = std::string("CREATE VIEW IF NOT EXISTS ") + CONST_ASSETMAP_VIEW_NAME +
                        " AS SELECT * FROM " +
                        CONST_MEDIALIBRARY_TABLE + " a " + ", " +
                        CONST_SMARTALBUM_MAP_TABLE + " b " +
                        " WHERE " +
                        "a." + CONST_MEDIA_DATA_DB_ID + " = " +
                        "b." + SMARTALBUMMAP_DB_CHILD_ASSET_ID;

const std::string CREATE_FILES_DELETE_TRIGGER =
                        std::string("CREATE TRIGGER IF NOT EXISTS delete_trigger AFTER UPDATE ON ") +
                        CONST_MEDIALIBRARY_TABLE + " FOR EACH ROW WHEN new.dirty = " +
                        std::to_string(static_cast<int32_t>(DirtyType::TYPE_DELETED)) +
                        " and OLD.position = 1 AND is_caller_self_func() = 'true'" +
                        " BEGIN " +
                        " DELETE FROM " + CONST_MEDIALIBRARY_TABLE + " WHERE file_id = old.file_id;" +
                        " END;";

const std::string CREATE_FILES_FDIRTY_TRIGGER =
                        std::string("CREATE TRIGGER IF NOT EXISTS fdirty_trigger AFTER UPDATE ON ") +
                        CONST_MEDIALIBRARY_TABLE + " FOR EACH ROW WHEN OLD.position <> 1 AND" +
                        " new.date_modified <> old.date_modified AND is_caller_self_func() = 'true'" +
                        " BEGIN " +
                        " UPDATE " + CONST_MEDIALIBRARY_TABLE + " SET dirty = " +
                        std::to_string(static_cast<int32_t>(DirtyType::TYPE_FDIRTY)) +
                        " WHERE file_id = old.file_id;" +
                        " SELECT cloud_sync_func(); " +
                        " END;";

const std::string CREATE_FILES_MDIRTY_TRIGGER =
                        std::string("CREATE TRIGGER IF NOT EXISTS mdirty_trigger AFTER UPDATE ON ") +
                        CONST_MEDIALIBRARY_TABLE + " FOR EACH ROW WHEN OLD.position <> 1" +
                        " AND new.date_modified = old.date_modified AND old.dirty = " +
                        std::to_string(static_cast<int32_t>(DirtyType::TYPE_SYNCED)) +
                        " AND new.dirty = old.dirty AND is_caller_self_func() = 'true'" +
                        " BEGIN " +
                        " UPDATE " + CONST_MEDIALIBRARY_TABLE + " SET dirty = " +
                        std::to_string(static_cast<int32_t>(DirtyType::TYPE_MDIRTY)) +
                        " WHERE file_id = old.file_id;" +
                        " SELECT cloud_sync_func(); " +
                        " END;";

const std::string CREATE_INSERT_CLOUD_SYNC_TRIGGER =
                        std::string(" CREATE TRIGGER IF NOT EXISTS insert_cloud_sync_trigger AFTER INSERT ON ") +
                        CONST_MEDIALIBRARY_TABLE +
                        " BEGIN SELECT cloud_sync_func(); END;";

const std::string CREATE_MEDIALIBRARY_ERROR_TABLE = "CREATE TABLE IF NOT EXISTS " + MEDIALIBRARY_ERROR_TABLE + " ("
    + MEDIA_DATA_ERROR + " TEXT PRIMARY KEY)";

const std::string CREATE_ASSET_UNIQUE_NUMBER_TABLE = "CREATE TABLE IF NOT EXISTS " + ASSET_UNIQUE_NUMBER_TABLE + " (" +
    ASSET_MEDIA_TYPE + " TEXT, " + UNIQUE_NUMBER + " INT DEFAULT 0) ";

const std::string CREATE_ALBUM_REFRESH_TABLE = "CREATE TABLE IF NOT EXISTS " + ALBUM_REFRESH_TABLE + " ("
    + REFRESH_ALBUM_ID + " INT PRIMARY KEY, " + ALBUM_REFRESH_STATUS + " INT DEFAULT 0 NOT NULL)";

const std::string CREATE_ALBUM_ORDER_BACK_TABLE = "CREATE TABLE IF NOT EXISTS " + ALBUM_ORDER_BACK_TABLE + "("+
    LPATH + " TEXT PRIMARY KEY, " +
    ALBUMS_ORDER + " INT, " +
    ORDERS_TYPE + " INT, " +
    ORDERS_SECTION + " INT, " +
    STYLE2_ALBUMS_ORDER + " INT, " +
    STYLE2_ORDERS_TYPE + " INT, " +
    STYLE2_ORDERS_SECTION + " INT) ";

const std::string CREATE_LAKE_ALBUM_TABLE = "CREATE TABLE IF NOT EXISTS " + LAKE_ALBUM_TABLE + " (" +
    LAKE_ALBUM_ID + " INTEGER PRIMARY KEY, " +
    LAKE_ALBUM_LPATH + " TEXT, " +
    LAKE_FOLDER_MODIFIED + " BIGINT DEFAULT 0) ";
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MEDIALIBRARY_DB_CONST_SQLS_H