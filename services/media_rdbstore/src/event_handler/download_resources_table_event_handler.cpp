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
#define MLOG_TAG "Media_Upgrade"

#include <string>
#include <vector>

#include "download_resources_table_event_handler.h"

#include "rdb_store.h"
#include "rdb_errno.h"
#include "result_set_utils.h"
#include "media_log.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_rdbstore.h"
#include "media_file_utils.h"
#include "medialibrary_db_const.h"

namespace OHOS::Media {
/**
 * @brief execute sql while database created
 * @param store rdb store
 */
int32_t DownloadResourcesTableEventHandler::OnCreate(std::shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_INFO_LOG("DownloadResourcesTableEventHandler OnCreate, in");
    int32_t ret = this->CreateTable(store);
    if (ret != NativeRdb::E_OK) {
        return NativeRdb::E_ERROR;
    }
    MEDIA_INFO_LOG("OnCreate end create download_resources_task_records table");
    return NativeRdb::E_OK;
}

int32_t DownloadResourcesTableEventHandler::CreateTable(std::shared_ptr<MediaLibraryRdbStore> &store)
{
    int32_t ret = store->ExecuteSql(this->CREATE_TABLE_SQL);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Media_Library: CreateTable failed, ret=%{public}d, sql=%{public}s", ret,
            this->CREATE_TABLE_SQL.c_str());
        return NativeRdb::E_ERROR;
    }
    return NativeRdb::E_OK;
}

/**
 * @brief execute sql while database upgraded
 * @param store rdb store
 */
int32_t DownloadResourcesTableEventHandler::OnUpgrade(
    std::shared_ptr<MediaLibraryRdbStore> store, int32_t oldVersion, int32_t newVersion)
{
    bool isOldVersion = oldVersion < VERSION_UPDATE_PHOTO_ALBUM_DATEMODIFIED_TIGGER;
    if (isOldVersion) {
        MEDIA_INFO_LOG("OnUpgrade OnCreate create download_resources_task_records table");
        this->OnCreate(store);
    }
    MEDIA_INFO_LOG("OnUpgrade oldVersion %{public}d newVersion %{public}d", oldVersion, newVersion);
    CHECK_AND_RETURN_RET(isOldVersion, NativeRdb::E_OK);
    return NativeRdb::E_OK;
}
}  // namespace OHOS::Media