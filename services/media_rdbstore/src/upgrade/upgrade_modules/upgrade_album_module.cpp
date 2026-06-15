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

#define MLOG_TAG "Media_Upgrade"

#include "media_library_upgrade_macros.h"
#include "media_library_upgrade_helper.h"
#include "upgrade_album_sqls.h"
#include "media_log.h"
#include "rdb_store.h"
#include "medialibrary_db_const.h"
#include "medialibrary_operation_record.h"
#include "album_plugin_table_event_handler.h"

namespace OHOS {
namespace Media {
using namespace std;
#define ALBUM_MODULE_NAME "Album"

// ADD Album Table Upgrade Code HERE
static vector<pair<int32_t, int32_t>> VersionFilterTabAssetAlbumOperation(NativeRdb::RdbStore &store)
{
    SqlBuilder builder;
    auto commands = builder.AddRawSql(SQL_CREATE_TAB_ASSET_ALBUM_OPERATION)
                           .DropTrigger(OPERATION_ASSET_INSERT_TRIGGER)
                           .AddRawSql(SQL_CREATE_OPERATION_ASSET_INSERT_TRIGGER)
                           .DropTrigger(OPERATION_ASSET_DELETE_TRIGGER)
                           .AddRawSql(SQL_CREATE_OPERATION_ASSET_DELETE_TRIGGER)
                           .DropTrigger(OPERATION_ASSET_UPDATE_TRIGGER)
                           .AddRawSql(SQL_CREATE_OPERATION_ASSET_UPDATE_TRIGGER)
                           .DropTrigger(OPERATION_ALBUM_INSERT_TRIGGER)
                           .AddRawSql(SQL_CREATE_OPERATION_ALBUM_INSERT_TRIGGER)
                           .DropTrigger(OPERATION_ALBUM_DELETE_TRIGGER)
                           .AddRawSql(SQL_CREATE_OPERATION_ALBUM_DELETE_TRIGGER)
                           .DropTrigger(OPERATION_ALBUM_UPDATE_TRIGGER)
                           .AddRawSql(SQL_CREATE_OPERATION_ALBUM_UPDATE_TRIGGER)
                           .DropTable(TABLE_ALBUM_PLUGIN)
                           .Build();
    auto result =  UpgradeHelper::ExecuteCommands(commands, store);
    AlbumPluginTableEventHandler().OnCreate(store);
    return result;
}
REGISTER_SYNC_UPGRADE_MODULE_TASK(VERSION_FILTER_TAB_ASSET_ALBUM_OPERATION,
    ALBUM_MODULE_NAME, VersionFilterTabAssetAlbumOperation);

static vector<pair<int32_t, int32_t>> AddCoverOrderColumns(NativeRdb::RdbStore &store)
{
    SqlBuilder builder;
    auto commands = builder.AddColumn(TABLE_PHOTO_ALBUM, COLUMN_COVER_ORDER_KEY, "TEXT DEFAULT NULL")
                           .AddColumn(TABLE_PHOTO_ALBUM, COLUMN_COVER_ORDER_SUBKEY, "TEXT DEFAULT NULL")
                           .AddColumn(TABLE_PHOTO_ALBUM, COLUMN_COVER_ORDER_TYPE, "INT NOT NULL DEFAULT 0")
                           .AddColumn(TABLE_PHOTO_ALBUM, COLUMN_HIDDEN_COVER_ORDER_KEY, "TEXT DEFAULT NULL")
                           .AddColumn(TABLE_PHOTO_ALBUM, COLUMN_HIDDEN_COVER_ORDER_SUBKEY, "TEXT DEFAULT NULL")
                           .AddColumn(TABLE_PHOTO_ALBUM, COLUMN_HIDDEN_COVER_ORDER_TYPE, "INT NOT NULL DEFAULT 0")
                           .AddRawSql(SQL_UPGRADE_CREATE_TAB_COVER_RECORD)
                           .Build();
    return UpgradeHelper::ExecuteCommands(commands, store);
}
REGISTER_SYNC_UPGRADE_MODULE_TASK(VERSION_ADD_TAB_COVER_RECORD_AND_INDEX, ALBUM_MODULE_NAME, AddCoverOrderColumns)
}
}