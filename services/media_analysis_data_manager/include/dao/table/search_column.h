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

#ifndef MEDIALIBRARY_SEARCH_COLUMN_H
#define MEDIALIBRARY_SEARCH_COLUMN_H

#include "media_column.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "vision_column.h"
#include "userfilemgr_uri.h"

namespace OHOS {
namespace Media {
// uri
const std::string URI_SEARCH_INDEX = MEDIALIBRARY_DATA_URI + "/" + SEARCH_TOTAL_TABLE;
const std::string UPDATE_INDEX = "update_index";

// create search table
const std::string TBL_SEARCH_ID = "id";
const std::string TBL_SEARCH_FILE_ID = "file_id";
const std::string TBL_SEARCH_DATA = "data";
const std::string TBL_SEARCH_DISPLAYNAME = "display_name";
const std::string TBL_SEARCH_LATITUDE = "latitude";
const std::string TBL_SEARCH_LONGITUDE = "longitude";
const std::string TBL_SEARCH_DATE_MODIFIED = "date_modified";
const std::string TBL_SEARCH_PHOTO_STATUS = "photo_status";
const std::string TBL_SEARCH_CV_STATUS = "cv_status";
const std::string TBL_SEARCH_GEO_STATUS = "geo_status";
const std::string TBL_SEARCH_VERSION = "version";
const std::string TBL_SEARCH_SYSTEM_LANGUAGE = "system_language";

// Number of completed and total progress in image and video indexing construction
const std::string PHOTO_COMPLETE_NUM = "finishedImageCount";
const std::string PHOTO_TOTAL_NUM = "totalImageCount";
const std::string VIDEO_COMPLETE_NUM = "finishedVideoCount";
const std::string VIDEO_TOTAL_NUM = "totalVideoCount";

// field status enum
enum TblSearchPhotoStatus {
    INSERT_FAIL = -3,
    DELETE_FAIL = -2,
    NEED_DELETE = -1,
    NO_INDEX = 0,
    INDEXED = 1,
    NEED_UPDATE = 2,
    INDEXED_NEW = 3,
};

const std::string CREATE_SEARCH_TOTAL_TABLE = "CREATE TABLE IF NOT EXISTS " + SEARCH_TOTAL_TABLE + " (" +
    TBL_SEARCH_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    TBL_SEARCH_FILE_ID + " INT UNIQUE, " +
    TBL_SEARCH_DATA + " TEXT, " +
    TBL_SEARCH_DISPLAYNAME + " TEXT, " +
    TBL_SEARCH_LATITUDE + " DOUBLE, " +
    TBL_SEARCH_LONGITUDE + " DOUBLE, " +
    TBL_SEARCH_DATE_MODIFIED + " BIGINT DEFAULT 0, " +
    TBL_SEARCH_PHOTO_STATUS + " INT DEFAULT 0, " +
    TBL_SEARCH_CV_STATUS + " INT DEFAULT 0, " +
    TBL_SEARCH_GEO_STATUS + " INT DEFAULT 0, " +
    TBL_SEARCH_VERSION + " INT DEFAULT 0, " +
    TBL_SEARCH_SYSTEM_LANGUAGE + " TEXT) ";

// trigger
// Listening of Photos: basic information
const std::string INSERT_SEARCH_TRIGGER = "insert_search_trigger";
const std::string CREATE_SEARCH_INSERT_TRIGGER =
    std::string("CREATE TRIGGER IF NOT EXISTS insert_search_trigger AFTER INSERT ON ") +
    PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW " +
    " WHEN (NEW.media_type = 1 OR NEW.media_type = 2)" +
    " AND NEW." + PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " <> " +
    to_string(static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) +
    " BEGIN " +
    " INSERT INTO " + SEARCH_TOTAL_TABLE +
    " (" + TBL_SEARCH_FILE_ID + ", " + TBL_SEARCH_DATA + ", " + TBL_SEARCH_DATE_MODIFIED + ", " +
    TBL_SEARCH_DISPLAYNAME + ", " + TBL_SEARCH_LATITUDE + ", " + TBL_SEARCH_LONGITUDE + " )" +
    " VALUES ( NEW.file_id, NEW.data, NEW.date_modified, NEW.display_name, NEW.latitude, NEW.longitude );" +
    " END;";

// Listening of Photos: update(date_modified, latitude, longitude)
const std::string UPDATE_SEARCH_TRIGGER = "update_search_trigger";
const std::string CREATE_SEARCH_UPDATE_TRIGGER =
    std::string("CREATE TRIGGER IF NOT EXISTS update_search_trigger AFTER UPDATE") +
    " OF data, date_modified, latitude, longitude " +
    " ON " + PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW " +
    " BEGIN " +
    " UPDATE " + SEARCH_TOTAL_TABLE +
    " SET " + " ( data, date_modified, latitude, longitude ) = " +
    " ( NEW.data, NEW.date_modified, NEW.latitude, NEW.longitude ) " +
    " WHERE " + TBL_SEARCH_FILE_ID + " = OLD.file_id;" +
    " END;";

// Listening of Photos: update (title,date_modified,latitude,longitude
// date_day, date_month, date_year, shooting_mode, date_taken, hidden, date_trashed)
const std::string UPDATE_SEARCH_STATUS_TRIGGER = "update_search_status_trigger";
const std::string CREATE_SEARCH_UPDATE_STATUS_TRIGGER =
    std::string("CREATE TRIGGER IF NOT EXISTS update_search_status_trigger AFTER UPDATE") +
    " OF title, date_modified, latitude, longitude, date_day, date_month, date_year, " +
    " shooting_mode, date_taken, hidden, date_trashed, user_comment, clean_flag, owner_album_id, is_favorite " +
    " ON " + PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW " +
    " BEGIN " +
    " UPDATE " + SEARCH_TOTAL_TABLE +
    " SET " + TBL_SEARCH_PHOTO_STATUS + " = " + std::to_string(TblSearchPhotoStatus::NEED_UPDATE) +
    " WHERE " + " (" + TBL_SEARCH_FILE_ID + " = OLD.file_id" +
    " AND (" + TBL_SEARCH_PHOTO_STATUS + " = " + std::to_string(TblSearchPhotoStatus::INDEXED) +
    " OR " + TBL_SEARCH_PHOTO_STATUS + " = " + std::to_string(TblSearchPhotoStatus::INDEXED_NEW) + "));" +
    " END;";

// Listening of Photos: delete
const std::string DELETE_SEARCH_TRIGGER = "delete_search_trigger";
const std::string CREATE_SEARCH_DELETE_TRIGGER =
    std::string("CREATE TRIGGER IF NOT EXISTS delete_search_trigger AFTER DELETE ON ") +
    PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW " +
    " BEGIN " +
    " UPDATE " + SEARCH_TOTAL_TABLE +
    " SET " + TBL_SEARCH_PHOTO_STATUS + " = " + std::to_string(TblSearchPhotoStatus::NEED_DELETE) +
    " WHERE " + TBL_SEARCH_FILE_ID + " = OLD.file_id;" +
    " END;";

// Listening of photoMap: insert
const std::string ALBUM_MAP_INSERT_SEARCH_TRIGGER = "album_map_insert_search_trigger";

// Listening of photoMap: delete
const std::string ALBUM_MAP_DELETE_SEARCH_TRIGGER = "album_map_delete_search_trigger";

// Listening of photoAlbum: update of(album_name)
const std::string ALBUM_UPDATE_SEARCH_TRIGGER = "album_update_search_trigger";
const std::string CREATE_ALBUM_UPDATE_SEARCH_TRIGGER =
    std::string("CREATE TRIGGER IF NOT EXISTS album_update_search_trigger AFTER UPDATE ") +
    " OF album_name " +
    " ON " + PhotoAlbumColumns::TABLE + " FOR EACH ROW " +
    " BEGIN " +
    " UPDATE " + SEARCH_TOTAL_TABLE +
    " SET " + TBL_SEARCH_PHOTO_STATUS + " = " + std::to_string(TblSearchPhotoStatus::NEED_UPDATE) +
    " WHERE " + " (" +
    TBL_SEARCH_FILE_ID + " IN" + " (" +
    "SELECT " + PhotoMap::ASSET_ID + " FROM " + PhotoMap::TABLE +
    " WHERE " + " ( old.album_id = PhotoMap.map_album ) " +
    " ) " +
    " );" +
    " END;";

// Listening of cv tab_analysis_total: update of(status,ocr,label,face), update cv_status
const std::string ANALYSIS_UPDATE_SEARCH_TRIGGER = "analysis_update_search_trigger";
const std::string CREATE_ANALYSIS_UPDATE_SEARCH_TRIGGER =
    std::string("CREATE TRIGGER IF NOT EXISTS analysis_update_search_trigger AFTER UPDATE ") +
    " OF status, ocr, label, face" +
    " ON " + VISION_TOTAL_TABLE + " FOR EACH ROW " +
    " WHEN (NEW.status = 1" +
    " OR NEW.ocr = 1 OR NEW.label = 1 OR NEW.face > 2)" +
    " BEGIN " +
    " UPDATE " + SEARCH_TOTAL_TABLE +
    " SET " + TBL_SEARCH_CV_STATUS + " = " +
    " CASE WHEN (NEW.status = 1) THEN " + std::to_string(TblSearchPhotoStatus::NO_INDEX) +
    " ELSE " + std::to_string(TblSearchPhotoStatus::NEED_UPDATE) + " END" +
    " WHERE " + " (" + TBL_SEARCH_FILE_ID + " = old.file_id " +
    " AND " + TBL_SEARCH_CV_STATUS + " = " + std::to_string(TblSearchPhotoStatus::INDEXED) +
    " AND EXISTS (SELECT 1 FROM Photos WHERE file_id = old.file_id AND media_type != 2));" +
    " END;";

// Listening of cv tab_analysis_video_total: update of(status,label,face), update cv_status
const std::string ANALYSIS_UPDATE_VIDEO_SEARCH_TRIGGER = "analysis_update_video_search_trigger";
const std::string CREATE_ANALYSIS_UPDATE_VIDEO_SEARCH_TRIGGER =
    std::string("CREATE TRIGGER IF NOT EXISTS analysis_update_video_search_trigger AFTER UPDATE ") +
    " OF status, label, face" +
    " ON " + VISION_VIDEO_TOTAL_TABLE + " FOR EACH ROW " +
    " WHEN (NEW.status = 1" +
    " OR NEW.label = 1 OR NEW.face > 2)" +
    " BEGIN " +
    " UPDATE " + SEARCH_TOTAL_TABLE +
    " SET " + TBL_SEARCH_CV_STATUS + " = " +
    " CASE WHEN (NEW.status = 1) THEN " + std::to_string(TblSearchPhotoStatus::NO_INDEX) +
    " ELSE " + std::to_string(TblSearchPhotoStatus::NEED_UPDATE) + " END" +
    " WHERE " + " (" + TBL_SEARCH_FILE_ID + " = old.file_id " +
    " AND " + TBL_SEARCH_CV_STATUS + " = " + std::to_string(TblSearchPhotoStatus::INDEXED) + ");" +
    " END;";

const std::string IDX_FILEID_FOR_SEARCH_INDEX = "idx_fileid_for_search_index";
const std::string CREATE_IDX_FILEID_FOR_SEARCH_INDEX = "CREATE INDEX IF NOT EXISTS " +
    IDX_FILEID_FOR_SEARCH_INDEX + " ON " + SEARCH_TOTAL_TABLE + " ( file_id );";
} // namespace Media
} // namespace OHOS
#endif // MEDIALIBRARY_SEARCH_COLUMN_H