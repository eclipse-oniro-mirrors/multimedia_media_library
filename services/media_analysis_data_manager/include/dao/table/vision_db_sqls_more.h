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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_DB_SQLS_MORE_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_DB_SQLS_MORE_H

#include "vision_db_sqls.h"

namespace OHOS {
namespace Media {
const std::string ADD_POSE_STATUS_COLUMN = "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " + POSE + " INT";
const std::string UPDATE_POSE_TOTAL_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " + STATUS + " = 0, " + POSE +
    " = 0 WHERE " + FILE_ID + " IN (SELECT " + FILE_ID + " FROM " + PhotoColumn::PHOTOS_TABLE +
    " WHERE media_type = 1)";
const std::string UPDATE_POSE_NOT_SUPPORT_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " + POSE + " = -1 WHERE " +
    POSE + " IS NULL";
const std::string POSE_INDEX = "pose_index";
const std::string CREATE_POSE_INDEX = "CREATE UNIQUE INDEX IF NOT EXISTS " + POSE_INDEX + " ON " + VISION_POSE_TABLE +
    " (" + FILE_ID + "," + POSE_ID + ")";

const std::string ALTER_WIDTH_COLUMN = "ALTER TABLE tab_analysis_ocr ADD COLUMN width INT;";
const std::string ALTER_HEIGHT_COLUMN = "ALTER TABLE tab_analysis_ocr ADD COLUMN height INT;";
const std::string DROP_TABLE_ANALYSISALBUM = "DROP TABLE AnalysisAlbum;";
const std::string DROP_TABLE_ANALYSISPHOTOMAP = "DROP TABLE AnalysisPhotoMap;";

const std::string ADD_TAG_ID_COLUMN_FOR_ALBUM = "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " +
    TAG_ID + " TEXT";
const std::string ADD_USER_OPERATION_COLUMN_FOR_ALBUM = "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " +
    USER_OPERATION + " INT";
const std::string ADD_GROUP_TAG_COLUMN_FOR_ALBUM = "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " +
    GROUP_TAG + " TEXT";
const std::string ADD_USER_DISPLAY_LEVEL_COLUMN_FOR_ALBUM = "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " +
    USER_DISPLAY_LEVEL + " INT";
const std::string ADD_IS_ME_COLUMN_FOR_ALBUM = "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " +
    IS_ME + " INT";
const std::string ADD_IS_REMOVED_COLUMN_FOR_ALBUM = "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " +
    IS_REMOVED + " INT";
const std::string ADD_RENAME_OPERATION_COLUMN_FOR_ALBUM = "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " +
    RENAME_OPERATION + " INT";
const std::string ADD_IS_LOCAL_COLUMN_FOR_ALBUM = "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " +
    IS_LOCAL + " INT";
const std::string ADD_IS_COVER_SATISFIED_FOR_ALBUM = "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " +
    IS_COVER_SATISFIED + " INT";
const std::string CREATE_ANALYSIS_ALBUM_FOR_ONCREATE = "CREATE TABLE IF NOT EXISTS " + ANALYSIS_ALBUM_TABLE + " (" +
    ALBUM_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    ALBUM_TYPE + " INT, " +
    ALBUM_SUBTYPE + " INT, " +
    ALBUM_NAME + " TEXT, " +
    COVER_URI + " TEXT, " +
    COUNT + " INT, " +
    DATE_MODIFIED + " BIGINT, " +
    RANK + " INT, " +
    TAG_ID + " TEXT, " +
    USER_OPERATION + " INT, " +
    GROUP_TAG + " TEXT, " +
    USER_DISPLAY_LEVEL + " INT, " +
    IS_ME + " INT, " +
    IS_REMOVED + " INT, " +
    RENAME_OPERATION + " INT, " +
    IS_LOCAL + " INT, " +
    IS_COVER_SATISFIED + " INT, " +
    ALBUM_RELATIONSHIP + " TEXT(64), " +
    ANALYSIS_STATUS + " INT, " +
    EDIT_OPERATION + " INT ) ";
const std::string ANALYSIS_ALBUM_UPDATE_SEARCH_TRIGGER = "analysis_album_update_search_trigger";
const std::string CREATE_ANALYSIS_ALBUM_UPDATE_SEARCH_TRIGGER = "CREATE TRIGGER IF NOT EXISTS " +
    ANALYSIS_ALBUM_UPDATE_SEARCH_TRIGGER +
    " AFTER UPDATE OF " + ALBUM_NAME + ", "  + IS_ME + ", " + ALBUM_RELATIONSHIP +
    " ON " + ANALYSIS_ALBUM_TABLE +
    " FOR EACH ROW WHEN (NEW." + ALBUM_SUBTYPE + " = " + std::to_string(PORTRAIT) + ")" +
    " BEGIN " +
    " UPDATE " + "tab_analysis_search_index" +
    " SET " + "cv_status" + " = 0 " +
    " WHERE (" + FILE_ID + " IN( " +
    " SELECT " + MAP_ASSET +
    " FROM " + ANALYSIS_PHOTO_MAP_TABLE +
    " WHERE (old." + ALBUM_ID + " = " + ANALYSIS_PHOTO_MAP_TABLE + "." + MAP_ALBUM + ")" +
    ")); END;";

const std::string IDX_FILEID_FOR_ANALYSIS_PHOTO_MAP = "idx_fileid_for_analysis_photo_map";
const std::string CREATE_IDX_FILEID_FOR_ANALYSIS_PHOTO_MAP = "CREATE INDEX IF NOT EXISTS " +
    IDX_FILEID_FOR_ANALYSIS_PHOTO_MAP + " ON " + ANALYSIS_PHOTO_MAP_TABLE + " ( map_asset );";

const std::string IDX_FILEID_FOR_ANALYSIS_TOTAL = "idx_fileid_for_analysis_total";
const std::string CREATE_IDX_FILEID_FOR_ANALYSIS_TOTAL = "CREATE INDEX IF NOT EXISTS " +
    IDX_FILEID_FOR_ANALYSIS_TOTAL + " ON " + VISION_TOTAL_TABLE + " ( file_id );";

const std::string IDX_GROUP_TAG_FOR_ANALYSIS_ALBUM = "idx_group_tag";
const std::string CREATE_ANALYSIS_ALBUM_GROUP_TAG_INDEX = "CREATE INDEX IF NOT EXISTS " +
    IDX_GROUP_TAG_FOR_ANALYSIS_ALBUM + " ON " + ANALYSIS_ALBUM_TABLE + " ( " + GROUP_TAG + " );";

const std::string IDX_ALBUM_SUBTYPE_NAME = "idx_album_subtype_name";
const std::string CREATE_ANALYSIS_ALBUM_SUBTYPE_NAME_INDEX = "CREATE INDEX IF NOT EXISTS " +
    IDX_ALBUM_SUBTYPE_NAME + " ON " + ANALYSIS_ALBUM_TABLE + " ( " + ALBUM_SUBTYPE + "," + ALBUM_NAME +
    " ) WHERE " + ALBUM_NAME + " is not null;";

const std::string IDX_ALBUM_TAG_ID = "idx_album_tag_id";
const std::string CREATE_ANALYSIS_ALBUM_TAG_ID_INDEX = "CREATE INDEX IF NOT EXISTS " +
    IDX_ALBUM_TAG_ID + " ON " + ANALYSIS_ALBUM_TABLE + " ( " + TAG_ID + " ) WHERE " + TAG_ID + " is not null;";
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_DB_SQLS_MORE_H