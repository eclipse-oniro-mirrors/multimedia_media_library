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
#include "upgrade_vision_sqls.h"
#include "media_log.h"
#include "rdb_store.h"
#include "medialibrary_db_const.h"

namespace OHOS {
namespace Media {
using namespace std;
#define VISION_MODULE_NAME "Vision"
// ADD Vision Table Upgrade Code HERE

// VERSION_ADD_VISION_TABLE: 添加分析相关表
static vector<pair<int32_t, int32_t>> VersionAddVisionTable(NativeRdb::RdbStore& store)
{
    SqlBuilder builder;
    auto commands = builder.DropTable(TABLE_TAB_ANALYSIS_LABEL)
                           .AddRawSql(SQL_CREATE_TAB_ANALYSIS_OCR)
                           .AddRawSql(SQL_CREATE_TAB_ANALYSIS_LABEL)
                           .AddRawSql(SQL_CREATE_TAB_ANALYSIS_AESTHETICS)
                           .AddRawSql(SQL_CREATE_TAB_ANALYSIS_TOTAL)
                           .AddRawSql(SQL_CREATE_VISION_UPDATE_TRIGGER)
                           .AddRawSql(SQL_CREATE_VISION_DELETE_TRIGGER)
                           .AddRawSql(SQL_CREATE_VISION_INSERT_TRIGGER)
                           .AddRawSql(SQL_INIT_TAB_ANALYSIS_TOTAL)
                           .Build();

    return UpgradeHelper::ExecuteCommands(commands, store);
}
REGISTER_SYNC_UPGRADE_MODULE_TASK(VERSION_ADD_VISION_TABLE, VISION_MODULE_NAME, VersionAddVisionTable)

static vector<pair<int32_t, int32_t>> VersionAddCaptionTable(NativeRdb::RdbStore& store)
{
    SqlBuilder builder;
    auto commands = builder.AddRawSql(SQL_UPGRADE_CREATE_TAB_ANALYSIS_CAPTION)
                           .AddColumn(TABLE_TAB_ANALYSIS_TOTAL, COLUMN_ANALYSIS_CAPTION, "INT NOT NULL DEFAULT 0")
                           .DropTrigger(TRIGGER_ANALYSIS_UPDATE_SEARCH_TRIGGER)
                           .AddRawSql(SQL_UPGRADE_CREATE_ANALYSIS_UPDATE_SEARCH_TRIGGER)
                           .Build();

    return UpgradeHelper::ExecuteCommands(commands, store);
}
REGISTER_SYNC_UPGRADE_MODULE_TASK(VERSION_ADD_ANALYSIS_CAPTION_TABLE, VISION_MODULE_NAME, VersionAddCaptionTable)
}
}