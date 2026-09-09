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

#include "photo_map_code_column.h"
#include "base_column.h"
#include "media_column.h"

namespace OHOS {
namespace Media {
const std::string PhotoMapCodeColumn::MAPCODE_LEVEL_SCALE_NUMBER = "1073741824";
const std::string PhotoMapCodeColumn::PHOTOS_MAP_CODE_TABLE = "tab_map_photo_map";

const std::string PhotoMapCodeColumn::MAPCODE_FILE_ID = "file_id";

const std::string PhotoMapCodeColumn::MAPCODE_LEVEL_20 = "cell_20int";

const std::string PhotoMapCodeColumn::MAPCODE_LEVEL_5_INDEX = "map_cell_5_index";
const std::string PhotoMapCodeColumn::MAPCODE_LEVEL_20_INDEX = "map_cell_20_index";

const std::string PhotoMapCodeColumn::CREATE_MAP_CODE_TABLE =
    "CREATE TABLE IF NOT EXISTS " + PhotoMapCodeColumn::PHOTOS_MAP_CODE_TABLE + " (" +
    PhotoMapCodeColumn::MAPCODE_FILE_ID + " INTEGER PRIMARY KEY, " +
    PhotoMapCodeColumn::MAPCODE_LEVEL_20 + " BIGINT DEFAULT 0 NOT NULL" +
    ") ";

const std::string PhotoMapCodeColumn::CREATE_MAPCODE_LEVEL_5_INDEX =
    BaseColumn::CreateIndex() + PhotoMapCodeColumn::MAPCODE_LEVEL_5_INDEX + " ON " +
    PhotoMapCodeColumn::PHOTOS_MAP_CODE_TABLE + " (" +
    PhotoMapCodeColumn::MAPCODE_LEVEL_20 + "/" + PhotoMapCodeColumn::MAPCODE_LEVEL_SCALE_NUMBER + " DESC)";

const std::string PhotoMapCodeColumn::CREATE_MAPCODE_LEVEL_20_INDEX =
    BaseColumn::CreateIndex() + PhotoMapCodeColumn::MAPCODE_LEVEL_20_INDEX + " ON " +
    PhotoMapCodeColumn::PHOTOS_MAP_CODE_TABLE + " (" +
    PhotoMapCodeColumn::MAPCODE_LEVEL_20 + " DESC)";

const std::string PhotoMapCodeColumn::DROP_MAPCODE_LEVEL_5_INDEX =
    BaseColumn::DropIndex() + PhotoMapCodeColumn::MAPCODE_LEVEL_5_INDEX;
const std::string PhotoMapCodeColumn::DROP_MAPCODE_LEVEL_20_INDEX =
    BaseColumn::DropIndex() + PhotoMapCodeColumn::MAPCODE_LEVEL_20_INDEX;

const std::string PhotoMapCodeColumn::INSERT_MAP_CODE_TRIGGER = "insert_map_code_trigger";
const std::string PhotoMapCodeColumn::CREATE_MAP_CODE_INSERT_TRIGGER =
    std::string("CREATE TRIGGER IF NOT EXISTS insert_map_code_trigger AFTER INSERT ON ") +
    PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW " +
    " WHEN NEW.latitude IS NOT NULL AND NEW.longitude IS NOT NULL" +
    " BEGIN " +
    " INSERT INTO " + PhotoMapCodeColumn::PHOTOS_MAP_CODE_TABLE +
    " (" + PhotoMapCodeColumn::MAPCODE_FILE_ID + ", " + PhotoMapCodeColumn::MAPCODE_LEVEL_20 + " )" +
    " VALUES ( NEW.file_id, photo_map_code_func(NEW.latitude, NEW.longitude, 'insert') );" +
    " END;";

const std::string PhotoMapCodeColumn::UPDATE_MAP_CODE_TRIGGER = "update_map_code_trigger";
const std::string PhotoMapCodeColumn::CREATE_MAP_CODE_UPDATE_TRIGGER =
    std::string("CREATE TRIGGER IF NOT EXISTS update_map_code_trigger AFTER UPDATE") +
    " ON " + PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW " +
    " WHEN (NEW.latitude IS NOT NULL AND NEW.longitude IS NOT NULL) AND " +
    " ( NEW.latitude <> OLD.latitude OR NEW.longitude <> OLD.longitude )"
    " BEGIN " +
    " INSERT OR REPLACE INTO " + PhotoMapCodeColumn::PHOTOS_MAP_CODE_TABLE +
    " (" + PhotoMapCodeColumn::MAPCODE_FILE_ID + ", " + PhotoMapCodeColumn::MAPCODE_LEVEL_20 + " )" +
    " VALUES ( NEW.file_id, photo_map_code_func(NEW.latitude, NEW.longitude, 'update') );" +
    " END;";

const std::string PhotoMapCodeColumn::CLEAR_MAP_CODE_TRIGGER = "clear_map_code_trigger";
const std::string PhotoMapCodeColumn::CREATE_MAP_CODE_CLEAR_TRIGGER =
    std::string("CREATE TRIGGER IF NOT EXISTS clear_map_code_trigger AFTER UPDATE") +
    " ON " + PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW " +
    " WHEN (NEW.latitude IS NULL OR NEW.longitude IS NULL) "
    " BEGIN " +
    " DELETE FROM " + PhotoMapCodeColumn::PHOTOS_MAP_CODE_TABLE +
    " WHERE " + PhotoMapCodeColumn::MAPCODE_FILE_ID + " = OLD.file_id;" +
    " END;";

const std::string PhotoMapCodeColumn::DELETE_MAP_CODE_TRIGGER = "delete_map_code_trigger";
const std::string PhotoMapCodeColumn::CREATE_MAP_CODE_DELETE_TRIGGER =
    std::string("CREATE TRIGGER IF NOT EXISTS delete_map_code_trigger AFTER DELETE ON ") +
    PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW " +
    " BEGIN " +
    " DELETE FROM " + PhotoMapCodeColumn::PHOTOS_MAP_CODE_TABLE +
    " WHERE " + PhotoMapCodeColumn::MAPCODE_FILE_ID + " = OLD.file_id;" +
    " END;";
} // namespace Media
} // namespace OHOS
