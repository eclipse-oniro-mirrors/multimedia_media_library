/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef UPGRADE_OTHER_TABLE_SQLS_H
#define UPGRADE_OTHER_TABLE_SQLS_H
// table name need to be added here
#define TABLE_ANALYSIS_ALBUM "AnalysisAlbum"
#define TABLE_DOWNLOAD_RESOURCES_TASK_RECORDS "download_resources_task_records"
#define PHOTOS_MAP_CODE_TABLE "tab_map_photo_map"
// column name should be added here
#define COLUMN_EXTRA_INFO "extra_info"
#define COLUMN_MAPCODE_LEVEL_20 "cell_20int"
#define COLUMN_MAPCODE_LEVEL_5_INDEX "map_cell_5_index"
#define COLUMN_MAPCODE_LEVEL_20_INDEX "map_cell_20_index"
#define COLUMN_FRIEND_ID "friend_id"
#define COLUMN_CONTACT_INFO "contact_info"
#define COLUMN_IS_SHARED "is_shared"

// sqls only execute in upgrade progress should be added here
#define SQL_CREATE_MAP_CODE_TABLE \
    "CREATE TABLE IF NOT EXISTS tab_map_photo_map (" \
    "file_id INTEGER PRIMARY KEY, " \
    "cell_20int BIGINT DEFAULT 0)"

#define SQL_CREATE_MAPCODE_LEVEL_5_INDEX \
    "CREATE INDEX IF NOT EXISTS map_cell_5_index ON tab_map_photo_map (cell_20int/1073741824 DESC)"
#define SQL_CREATE_MAPCODE_LEVEL_20_INDEX \
    "CREATE INDEX IF NOT EXISTS map_cell_20_index ON tab_map_photo_map (cell_20int DESC)"

#define SQL_CREATE_MAP_CODE_INSERT_TRIGGER \
    "CREATE TRIGGER IF NOT EXISTS insert_map_code_trigger AFTER INSERT" \
    " ON Photos FOR EACH ROW " \
    " WHEN NEW.file_id IS NOT NULL AND NEW.latitude IS NOT NULL AND NEW.longitude IS NOT NULL" \
    " BEGIN " \
    " INSERT INTO tab_map_photo_map" \
    " (file_id, cell_20int)" \
    " VALUES ( NEW.file_id, photo_map_code_func(NEW.latitude, NEW.longitude, 'insert') );" \
    " END;"

#define SQL_CREATE_MAP_CODE_UPDATE_TRIGGER \
    "CREATE TRIGGER IF NOT EXISTS update_map_code_trigger AFTER UPDATE" \
    " ON Photos FOR EACH ROW " \
    " WHEN (NEW.latitude IS NOT NULL AND NEW.longitude IS NOT NULL) AND " \
    " ( NEW.latitude <> OLD.latitude OR NEW.longitude <> OLD.longitude )" \
    " BEGIN " \
    " INSERT OR REPLACE INTO tab_map_photo_map" \
    " (file_id, cell_20int)" \
    " VALUES ( NEW.file_id, photo_map_code_func(NEW.latitude, NEW.longitude, 'update') );" \
    " END;"

#define SQL_CREATE_MAP_CODE_CLEAR_TRIGGER \
    "CREATE TRIGGER IF NOT EXISTS clear_map_code_trigger AFTER UPDATE" \
    " ON Photos FOR EACH ROW " \
    " WHEN (NEW.latitude IS NULL OR NEW.longitude IS NULL) " \
    " BEGIN " \
    " DELETE FROM tab_map_photo_map" \
    " WHERE file_id = OLD.file_id;" \
    " END;"

#define SQL_CREATE_MAP_CODE_DELETE_TRIGGER \
    "CREATE TRIGGER IF NOT EXISTS delete_map_code_trigger AFTER DELETE" \
    " ON Photos FOR EACH ROW " \
    " BEGIN " \
    " DELETE FROM tab_map_photo_map" \
    " WHERE file_id = OLD.file_id;" \
    " END;"

#endif // UPGRADE_OTHER_TABLE_SQLS_H