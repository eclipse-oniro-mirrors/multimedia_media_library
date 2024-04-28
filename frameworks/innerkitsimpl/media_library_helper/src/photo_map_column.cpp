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

#include "photo_map_column.h"

#include "medialibrary_db_const.h"
#include "photo_album_column.h"

using namespace std;

namespace OHOS::Media {
// PhotoMap table
const string PhotoMap::TABLE = "PhotoMap";
const string PhotoMap::ALBUM_ID = "map_album";
const string PhotoMap::ASSET_ID = "map_asset";
const string PhotoMap::DIRTY = "dirty";

const string PhotoMap::CREATE_TABLE = CreateTable() + TABLE +
    " (" +
    ALBUM_ID + " INT, " +
    ASSET_ID + " INT, " +
    DIRTY + " INT DEFAULT " + to_string(static_cast<int32_t>(DirtyTypes::TYPE_NEW)) + ", " +
    "PRIMARY KEY (" + ALBUM_ID + "," + ASSET_ID + ")" +
    ")";

const string PhotoMap::CREATE_NEW_TRIGGER =
    " CREATE TRIGGER IF NOT EXISTS album_map_insert_cloud_sync_trigger AFTER INSERT ON " + TABLE +
    " FOR EACH ROW WHEN new." + DIRTY + " = " +
    to_string(static_cast<int32_t>(DirtyTypes::TYPE_NEW)) + " AND is_caller_self_func() = 'true'" +
    " BEGIN UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_DIRTY + " = " +
    to_string(static_cast<int32_t>(DirtyTypes::TYPE_MDIRTY)) + " WHERE " + MediaColumn::MEDIA_ID + " = " +
    "new." + ASSET_ID + " AND " + PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_DIRTY + " = " +
    to_string(static_cast<int32_t>(DirtyTypes::TYPE_SYNCED)) + "; SELECT cloud_sync_func(); END;";

const string PhotoMap::CREATE_DELETE_TRIGGER =
    "CREATE TRIGGER IF NOT EXISTS album_map_delete_trigger AFTER UPDATE ON " + TABLE +
    " FOR EACH ROW WHEN new." + DIRTY + " = " +
    std::to_string(static_cast<int32_t>(DirtyTypes::TYPE_DELETED)) +
    " AND is_caller_self_func() = 'true' BEGIN DELETE FROM " + TABLE +
    " WHERE " + ALBUM_ID + " = old." + ALBUM_ID + " AND " + ASSET_ID + " = old." + ASSET_ID +
    " AND old." + DIRTY + " = " + std::to_string(static_cast<int32_t>(DirtyTypes::TYPE_NEW)) +
    "; UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_DIRTY + " = " +
    to_string(static_cast<int32_t>(DirtyTypes::TYPE_MDIRTY)) + " WHERE " + MediaColumn::MEDIA_ID + " = " +
    "new." + ASSET_ID + " AND " + PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_DIRTY + " = " +
    to_string(static_cast<int32_t>(DirtyTypes::TYPE_SYNCED)) + " AND " + "old." +
    PhotoMap::DIRTY + " = " + std::to_string(static_cast<int32_t>(DirtyTypes::TYPE_SYNCED)) +
    "; SELECT cloud_sync_func(); END;";
} // namespace OHOS::Media
