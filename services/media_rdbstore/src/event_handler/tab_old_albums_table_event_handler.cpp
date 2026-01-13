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

#define MLOG_TAG "TabOldAlbumEventHandler"

#include "tab_old_albums_table_event_handler.h"
#include "media_log.h"
#include "rdb_errno.h"

#include <string>

namespace OHOS::Media {
int32_t TabOldAlbumTableEventHandler::OnCreate(NativeRdb::RdbStore &store)
{
    MEDIA_INFO_LOG("OnCreate begin create %{public}s table", this->TABLE_NAME.c_str());
    int32_t ret = this->CreateTable(store);
    if (ret != NativeRdb::E_OK) {
        return ret;
    }
    MEDIA_INFO_LOG("OnCreate end create %{public}s table", this->TABLE_NAME.c_str());
    return NativeRdb::E_OK;
}

int32_t TabOldAlbumTableEventHandler::OnUpgrade(NativeRdb::RdbStore &store, int oldVersion, int newVersion)
{
    // Defined for abstract class
    return NativeRdb::E_OK;
}

int32_t TabOldAlbumTableEventHandler::CreateTable(NativeRdb::RdbStore &store)
{
    int32_t ret = store.ExecuteSql(this->CREATE_TABLE_SQL);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Media_Library: CreateTable failed, ret=%{public}d, sql=%{public}s", ret,
            this->CREATE_TABLE_SQL.c_str());
        return NativeRdb::E_ERROR;
    }
    return NativeRdb::E_OK;
}
} // namespace OHOS::Media