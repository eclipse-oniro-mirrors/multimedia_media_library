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
#include "upgrade_other_table_sqls.h"
#include "media_log.h"
#include "rdb_store.h"
#include "medialibrary_db_const.h"

namespace OHOS {
namespace Media {
using namespace std;
#define OTHER_TABLE_MODULE_NAME "OtherTable"
// ADD Other Table Upgrade Code HERE

static vector<pair<int32_t, int32_t>> AddExtraInfoColumn(NativeRdb::RdbStore &store)
{
    SqlBuilder builder;
    auto commands = builder.AddColumn(TABLE_ANALYSIS_ALBUM, COLUMN_EXTRA_INFO, "TEXT")
                        .Build();
    return UpgradeHelper::ExecuteCommands(commands, store, true);
}
REGISTER_SYNC_UPGRADE_MODULE_TASK(VERSION_ADD_ANALYSIS_ALBUM_EXTRA_INFO,
    OTHER_TABLE_MODULE_NAME, AddExtraInfoColumn);

static vector<pair<int32_t, int32_t>> AddFriendIdColumn(NativeRdb::RdbStore &store)
{
    SqlBuilder builder;
    auto commands = builder.AddColumn(TABLE_ANALYSIS_ALBUM, COLUMN_FRIEND_ID, "TEXT")
                        .Build();
    return UpgradeHelper::ExecuteCommands(commands, store, true);
}
REGISTER_SYNC_UPGRADE_MODULE_TASK(VERSION_ADD_ANALYSIS_ALBUM_FRIEND_ID,
    OTHER_TABLE_MODULE_NAME, AddFriendIdColumn);
}
}