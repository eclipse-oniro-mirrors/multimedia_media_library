/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef MEDIALIBRARY_OPERATION_RECORD_H
#define MEDIALIBRARY_OPERATION_RECORD_H

#include "media_column.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {

#define SQL_CREATE_TAB_ASSET_ALBUM_OPERATION \
    "CREATE TABLE IF NOT EXISTS tab_asset_and_album_operation (" \
    "id INTEGER PRIMARY KEY AUTOINCREMENT, " \
    "file_id INTEGER, " \
    "data TEXT, " \
    "opt_type INTEGER, " \
    "type INTEGER, " \
    "is_sent INTEGER, " \
    "UNIQUE (file_id, type, opt_type, is_sent) " \
    ") "

// trigger
// Listening of Assets: insert
#define OPERATION_ASSET_INSERT_TRIGGER "operation_asset_insert_trigger"
#define SQL_CREATE_OPERATION_ASSET_INSERT_TRIGGER \
    "CREATE TRIGGER IF NOT EXISTS operation_asset_insert_trigger AFTER INSERT ON Photos" \
    " FOR EACH ROW " \
    " WHEN (NEW.media_type = 1 OR NEW.media_type = 2)" \
    " AND NEW.POSITION <> 2" \
    " AND (NEW.storage_path IS NULL OR NEW.storage_path = '')" \
    " AND NEW.file_source_type <> 4 AND NEW.file_source_type <> 1" \
    " BEGIN " \
    " INSERT OR IGNORE INTO tab_asset_and_album_operation" \
    " (file_id, data, opt_type, type, is_sent ) VALUES ( NEW.file_id, NEW.data, 1, 1, 0);" \
    " END;"

// Listening of Assets: delete
#define OPERATION_ASSET_DELETE_TRIGGER "operation_asset_delete_trigger"
#define SQL_CREATE_OPERATION_ASSET_DELETE_TRIGGER \
    "CREATE TRIGGER IF NOT EXISTS operation_asset_delete_trigger AFTER DELETE ON Photos" \
    " FOR EACH ROW " \
    " WHEN (OLD.media_type = 1 OR OLD.media_type = 2)" \
    " AND OLD.POSITION <> 2" \
    " AND (OLD.storage_path IS NULL OR OLD.storage_path = '')" \
    " AND OLD.file_source_type <> 4 AND OLD.file_source_type <> 1" \
    " BEGIN " \
    " INSERT OR IGNORE INTO tab_asset_and_album_operation" \
    " (file_id, data, opt_type, type, is_sent) VALUES ( OLD.file_id, OLD.data, 2, 1, 0);" \
    " END;"

// Listening of Assets: update (title,date_modified,latitude,longitude
// date_day, date_month, date_year, shooting_mode, date_taken, hidden, date_trashed)
#define OPERATION_ASSET_UPDATE_TRIGGER "operation_asset_update_trigger"
#define SQL_CREATE_OPERATION_ASSET_UPDATE_TRIGGER \
    "CREATE TRIGGER IF NOT EXISTS operation_asset_update_trigger AFTER UPDATE ON Photos" \
    " FOR EACH ROW " \
    " WHEN (" \
    " (OLD.POSITION = 2 AND NEW.POSITION = 3)" \
    " OR (OLD.POSITION = 3 AND NEW.POSITION = 2)" \
    " OR (NEW.POSITION <> 2" \
    " AND (NEW.data <> OLD.data" \
    " OR NEW.size <> OLD.size" \
    " OR NEW.media_type <> OLD.media_type" \
    " OR NEW.mime_type <> OLD.mime_type" \
    " OR NEW.owner_album_id <> OLD.owner_album_id" \
    " OR NEW.date_modified <> OLD.date_modified" \
    " OR NEW.date_trashed <> OLD.date_trashed" \
    " OR NEW.date_taken <> OLD.date_taken" \
    " OR NEW.height <> OLD.height" \
    " OR NEW.width <> OLD.width" \
    " OR NEW.latitude <> OLD.latitude" \
    " OR NEW.longitude <> OLD.longitude" \
    " OR NEW.duration <> OLD.duration" \
    " OR NEW.orientation <> OLD.orientation" \
    " OR NEW.display_name <> OLD.display_name" \
    " OR NEW.is_favorite <> OLD.is_favorite" \
    " OR NEW.hidden <> OLD.hidden" \
    " OR NEW.user_comment <> OLD.user_comment" \
    " OR NEW.is_temp <> OLD.is_temp" \
    " OR NEW.time_pending <> OLD.time_pending" \
    " OR NEW.moving_photo_effect_mode <> OLD.moving_photo_effect_mode)" \
    " )" \
    " AND (NEW.storage_path IS NULL OR NEW.storage_path = '')" \
    " ) AND OLD.file_source_type <> 4 AND OLD.file_source_type <> 1" \
    " BEGIN " \
    " INSERT OR IGNORE INTO tab_asset_and_album_operation" \
    " (file_id, data, opt_type, type, is_sent) VALUES ( OLD.file_id, OLD.data, 3, 1, 0);" \
    " END;"

// Listening of album: insert
#define OPERATION_ALBUM_INSERT_TRIGGER "operation_album_insert_trigger"
#define SQL_CREATE_OPERATION_ALBUM_INSERT_TRIGGER \
    "CREATE TRIGGER IF NOT EXISTS operation_album_insert_trigger AFTER INSERT ON PhotoAlbum" \
    " WHEN NEW.album_subtype <> 2050" \
    " BEGIN " \
    " INSERT OR IGNORE INTO tab_asset_and_album_operation" \
    " (file_id, data, opt_type, type, is_sent) VALUES ( NEW.album_id, NEW.lpath, 1, 2, 0);" \
    " END;"

// Listening of album: delete
#define OPERATION_ALBUM_DELETE_TRIGGER "operation_album_delete_trigger"
#define SQL_CREATE_OPERATION_ALBUM_DELETE_TRIGGER \
    "CREATE TRIGGER IF NOT EXISTS operation_album_delete_trigger AFTER DELETE ON PhotoAlbum" \
    " WHEN OLD.album_subtype <> 2050" \
    " BEGIN " \
    " INSERT OR IGNORE INTO tab_asset_and_album_operation" \
    " (file_id, data, opt_type, type, is_sent) VALUES ( OLD.album_id, OLD.lpath, 2, 2, 0);" \
    " END;"

// Listening of album: update
#define OPERATION_ALBUM_UPDATE_TRIGGER "operation_album_update_trigger"
#define SQL_CREATE_OPERATION_ALBUM_UPDATE_TRIGGER \
    "CREATE TRIGGER IF NOT EXISTS operation_album_update_trigger AFTER UPDATE ON PhotoAlbum" \
    " FOR EACH ROW " \
    " WHEN (" \
    " NEW.album_name <> OLD.album_name" \
    " OR NEW.lpath <> OLD.lpath)" \
    " AND OLD.album_subtype <> 2050" \
    " AND OLD.share_type <> 2" \
    " BEGIN " \
    " INSERT OR IGNORE INTO tab_asset_and_album_operation" \
    " (file_id, data, opt_type, type, is_sent" \
    " ) VALUES ( OLD.album_id, OLD.lpath, 3, 2, 0);" \
    " END;"

// 共享相册资产/共享相册不支持同步到东湖，新增带共享过滤的 operation trigger

// Listening of Assets: insert (排除共享资产 is_shared = 0)
#define SQL_CREATE_OPERATION_ASSET_INSERT_TRIGGER_WITH_SHARED_FILTER \
    "CREATE TRIGGER IF NOT EXISTS operation_asset_insert_trigger AFTER INSERT ON Photos" \
    " FOR EACH ROW " \
    " WHEN (NEW.media_type = 1 OR NEW.media_type = 2)" \
    " AND NEW.POSITION <> 2" \
    " AND (NEW.storage_path IS NULL OR NEW.storage_path = '')" \
    " AND NEW.file_source_type <> 4 AND NEW.file_source_type <> 1" \
    " AND NEW.is_shared = 0" \
    " BEGIN " \
    " INSERT OR IGNORE INTO tab_asset_and_album_operation" \
    " (file_id, data, opt_type, type, is_sent ) VALUES ( NEW.file_id, NEW.data, 1, 1, 0);" \
    " END;"

// Listening of Assets: delete (排除共享资产 is_shared = 0)
#define SQL_CREATE_OPERATION_ASSET_DELETE_TRIGGER_WITH_SHARED_FILTER \
    "CREATE TRIGGER IF NOT EXISTS operation_asset_delete_trigger AFTER DELETE ON Photos" \
    " FOR EACH ROW " \
    " WHEN (OLD.media_type = 1 OR OLD.media_type = 2)" \
    " AND OLD.POSITION <> 2" \
    " AND (OLD.storage_path IS NULL OR OLD.storage_path = '')" \
    " AND OLD.file_source_type <> 4 AND OLD.file_source_type <> 1" \
    " AND OLD.is_shared = 0" \
    " BEGIN " \
    " INSERT OR IGNORE INTO tab_asset_and_album_operation" \
    " (file_id, data, opt_type, type, is_sent) VALUES ( OLD.file_id, OLD.data, 2, 1, 0);" \
    " END;"

// Listening of Assets: update (排除共享资产 is_shared = 0)
#define SQL_CREATE_OPERATION_ASSET_UPDATE_TRIGGER_WITH_SHARED_FILTER \
    "CREATE TRIGGER IF NOT EXISTS operation_asset_update_trigger AFTER UPDATE ON Photos" \
    " FOR EACH ROW " \
    " WHEN (" \
    " (OLD.POSITION = 2 AND NEW.POSITION = 3)" \
    " OR (OLD.POSITION = 3 AND NEW.POSITION = 2)" \
    " OR (NEW.POSITION <> 2" \
    " AND (NEW.data <> OLD.data" \
    " OR NEW.size <> OLD.size" \
    " OR NEW.media_type <> OLD.media_type" \
    " OR NEW.mime_type <> OLD.mime_type" \
    " OR NEW.owner_album_id <> OLD.owner_album_id" \
    " OR NEW.date_modified <> OLD.date_modified" \
    " OR NEW.date_trashed <> OLD.date_trashed" \
    " OR NEW.date_taken <> OLD.date_taken" \
    " OR NEW.height <> OLD.height" \
    " OR NEW.width <> OLD.width" \
    " OR NEW.latitude <> OLD.latitude" \
    " OR NEW.longitude <> OLD.longitude" \
    " OR NEW.duration <> OLD.duration" \
    " OR NEW.orientation <> OLD.orientation" \
    " OR NEW.display_name <> OLD.display_name" \
    " OR NEW.is_favorite <> OLD.is_favorite" \
    " OR NEW.hidden <> OLD.hidden" \
    " OR NEW.user_comment <> OLD.user_comment" \
    " OR NEW.is_temp <> OLD.is_temp" \
    " OR NEW.time_pending <> OLD.time_pending" \
    " OR NEW.moving_photo_effect_mode <> OLD.moving_photo_effect_mode)" \
    " )" \
    " AND (NEW.storage_path IS NULL OR NEW.storage_path = '')" \
    " ) AND OLD.file_source_type <> 4 AND OLD.file_source_type <> 1" \
    " AND OLD.is_shared = 0" \
    " BEGIN " \
    " INSERT OR IGNORE INTO tab_asset_and_album_operation" \
    " (file_id, data, opt_type, type, is_sent) VALUES ( OLD.file_id, OLD.data, 3, 1, 0);" \
    " END;"

// Listening of album: insert (排除共享相册 share_type <> 2)
#define SQL_CREATE_OPERATION_ALBUM_INSERT_TRIGGER_WITH_SHARED_FILTER \
    "CREATE TRIGGER IF NOT EXISTS operation_album_insert_trigger AFTER INSERT ON PhotoAlbum" \
    " WHEN NEW.album_subtype <> 2050 AND NEW.share_type <> 2" \
    " BEGIN " \
    " INSERT OR IGNORE INTO tab_asset_and_album_operation" \
    " (file_id, data, opt_type, type, is_sent) VALUES ( NEW.album_id, NEW.lpath, 1, 2, 0);" \
    " END;"

// Listening of album: delete (排除共享相册 share_type <> 2)
#define SQL_CREATE_OPERATION_ALBUM_DELETE_TRIGGER_WITH_SHARED_FILTER \
    "CREATE TRIGGER IF NOT EXISTS operation_album_delete_trigger AFTER DELETE ON PhotoAlbum" \
    " WHEN OLD.album_subtype <> 2050 AND OLD.share_type <> 2" \
    " BEGIN " \
    " INSERT OR IGNORE INTO tab_asset_and_album_operation" \
    " (file_id, data, opt_type, type, is_sent) VALUES ( OLD.album_id, OLD.lpath, 2, 2, 0);" \
    " END;"

// Listening of album: update (排除共享相册 share_type <> 2)
#define SQL_CREATE_OPERATION_ALBUM_UPDATE_TRIGGER_WITH_SHARED_FILTER \
    "CREATE TRIGGER IF NOT EXISTS operation_album_update_trigger AFTER UPDATE ON PhotoAlbum" \
    " FOR EACH ROW " \
    " WHEN (" \
    " NEW.album_name <> OLD.album_name" \
    " OR NEW.lpath <> OLD.lpath)" \
    " AND OLD.album_subtype <> 2050" \
    " AND OLD.share_type <> 2" \
    " BEGIN " \
    " INSERT OR IGNORE INTO tab_asset_and_album_operation" \
    " (file_id, data, opt_type, type, is_sent" \
    " ) VALUES ( OLD.album_id, OLD.lpath, 3, 2, 0);" \
    " END;"
} // namespace Media
} // namespace OHOS
#endif // MEDIALIBRARY_OPERATION_RECORD_H