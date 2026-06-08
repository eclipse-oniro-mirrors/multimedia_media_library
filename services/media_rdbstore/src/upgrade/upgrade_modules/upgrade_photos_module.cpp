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
#include "upgrade_photos_sqls.h"
#include "media_log.h"
#include "rdb_store.h"
#include "medialibrary_db_const.h"

namespace OHOS {
namespace Media {
using namespace std;
#define PHOTOS_MODULE_NAME "Photos"
// ==================== ADD Photos Table Upgrade Code HERE  ====================

static vector<pair<int32_t, int32_t>> AddPhotoRiskStatusColumnsAndDeleteCritical(NativeRdb::RdbStore &store)
{
    SqlBuilder builder;
    auto commands = builder.AddColumn(TABLE_PHOTOS, COLUMN_PHOTO_RISK_STATUS, "INT DEFAULT 0 NOT NULL")
                           .DropColumn(TABLE_PHOTOS, COLUMN_CRITICAL_TYPE)
                           .Build();
    return UpgradeHelper::ExecuteCommands(commands, store, true);
}
REGISTER_SYNC_UPGRADE_MODULE_TASK(VERSION_ADD_PHOTO_RISK_STATUS,
    "Photos", AddPhotoRiskStatusColumnsAndDeleteCritical);

static vector<pair<int32_t, int32_t>> AddPhotoNeedThumbnailColumn(NativeRdb::RdbStore &store)
{
    SqlBuilder builder;
    auto commands = builder.AddColumn(TABLE_PHOTOS, COLUMN_PHOTO_NEED_THUMBNAIL, "INT DEFAULT 1 NOT NULL")
                           .Build();
    return UpgradeHelper::ExecuteCommands(commands, store, true);
}
REGISTER_SYNC_UPGRADE_MODULE_TASK(VERSION_ADD_NEED_THUMBNAIL, PHOTOS_MODULE_NAME, AddPhotoNeedThumbnailColumn);

static vector<pair<int32_t, int32_t>> AddAttachmentSizeColumn(NativeRdb::RdbStore &store)
{
    SqlBuilder builder;
    auto commands = builder.AddColumn(TABLE_PHOTOS, COLUMN_ATTACHMENT_SIZE, "BIGINT DEFAULT -1 NOT NULL")
                           .Build();
    return UpgradeHelper::ExecuteCommands(commands, store, true);
}
REGISTER_SYNC_UPGRADE_MODULE_TASK(VERSION_ADD_ATTACHMENT_SIZE_COLUMN, PHOTOS_MODULE_NAME, AddAttachmentSizeColumn);

static vector<pair<int32_t, int32_t>> UpdateIndexsOnPhotos(NativeRdb::RdbStore &store)
{
    SqlBuilder builder;
    auto commands = builder.DropIndex(INDEX_PHOTO_SORT_IN_ALBUM_DATE_ADDED_INDEX)
                           .AddRawSql(SQL_UPGRADE_CREATE_PHOTO_SORT_IN_ALBUM_DATE_ADDED_INDEX)
                           .DropIndex(INDEX_PHOTO_SORT_IN_ALBUM_DATE_TAKEN_INDEX)
                           .AddRawSql(SQL_UPGRADE_CREATE_PHOTO_SORT_IN_ALBUM_DATE_TAKEN_INDEX)
                           .DropIndex(INDEX_PHOTO_SORT_IN_ALBUM_DISPLAY_NAME_INDEX)
                           .AddRawSql(SQL_UPGRADE_CREATE_PHOTO_SORT_IN_ALBUM_DISPLAY_NAME_INDEX)
                           .DropIndex(INDEX_PHOTO_SORT_IN_ALBUM_SIZE_INDEX)
                           .AddRawSql(SQL_UPGRADE_CREATE_PHOTO_SORT_IN_ALBUM_SIZE_INDEX)
                           .DropIndex(INDEX_PHOTO_SORT_MEDIA_TYPE_DATE_ADDED_INDEX)
                           .AddRawSql(SQL_UPGRADE_CREATE_PHOTO_SORT_MEDIA_TYPE_DATE_ADDED_INDEX)
                           .DropIndex(INDEX_PHOTO_SORT_MEDIA_TYPE_DATE_TAKEN_INDEX)
                           .AddRawSql(SQL_UPGRADE_CREATE_PHOTO_SORT_MEDIA_TYPE_DATE_TAKEN_INDEX)
                           .DropIndex(INDEX_PHOTO_SORT_MEDIA_TYPE_DISPLAY_NAME_INDEX)
                           .AddRawSql(SQL_UPGRADE_CREATE_PHOTO_SORT_MEDIA_TYPE_DISPLAY_NAME_INDEX)
                           .DropIndex(INDEX_PHOTO_SORT_MEDIA_TYPE_SIZE_INDEX)
                           .AddRawSql(SQL_UPGRADE_CREATE_PHOTO_SORT_MEDIA_TYPE_SIZE_INDEX)
                           .DropIndex(INDEX_PHOTO_FAVORITE_INDEX)
                           .AddRawSql(SQL_UPGRADE_CREATE_PHOTO_FAVORITE_INDEX)
                           .DropIndex(INDEX_PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX)
                           .AddRawSql(SQL_UPGRADE_CREATE_PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX)
                           .DropIndex(INDEX_PHOTO_MOVING_PHOTO_ALBUM_INDEX)
                           .AddRawSql(SQL_UPGRADE_CREATE_PHOTO_MOVING_PHOTO_ALBUM_INDEX)
                           .DropIndex(INDEX_PHOTO_RAW_IMAGE_ALBUM_INDEX)
                           .AddRawSql(SQL_UPGRADE_CREATE_PHOTO_RAW_IMAGE_ALBUM_INDEX)
                           .DropIndex(INDEX_PHOTO_SCHPT_PHOTO_DATEADDED_INDEX)
                           .AddRawSql(SQL_UPGRADE_INDEX_SCTHP_PHOTO_DATEADDED)
                           .DropIndex(INDEX_PHOTO_SCHPT_HIDDEN_TIME_INDEX)
                           .AddRawSql(SQL_UPGRADE_CREATE_SCHPT_HIDDEN_TIME_INDEX)
                           .AddRawSql(SQL_UPGRADE_CREATE_LPATH_INDEX)
                           .Build();

    return UpgradeHelper::ExecuteCommands(commands, store, true);
}
REGISTER_ASYNC_UPGRADE_MODULE_TASK(VERSION_ADD_TAB_COVER_RECORD_AND_INDEX, PHOTOS_MODULE_NAME, UpdateIndexsOnPhotos)
} // namespace Media
} // namespace OHOS