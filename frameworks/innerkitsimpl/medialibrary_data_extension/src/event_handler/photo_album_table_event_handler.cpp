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

#include "photo_album_table_event_handler.h"

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
int32_t PhotoAlbumTableEventHandler::OnCreate(std::shared_ptr<MediaLibraryRdbStore> store)
{
    return NativeRdb::E_OK;
}

/**
 * @brief execute sql while database upgraded
 * @param store rdb store
 */
int32_t PhotoAlbumTableEventHandler::OnUpgrade(
    std::shared_ptr<MediaLibraryRdbStore> store, int32_t oldVersion, int32_t newVersion)
{
    int32_t ret = NativeRdb::E_OK;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    for (auto &it : this->upgradeHandles_) {
        ret = (this->*it)(store, oldVersion, newVersion);
    }
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    MEDIA_INFO_LOG("OnUpgrade end upgrade %{public}s table, costTime: %{public}" PRId64 "ms.",
        PhotoAlbumColumns::TABLE.c_str(),
        costTime);
    return ret;
}

int32_t PhotoAlbumTableEventHandler::AddWriterCodeColumn(
    std::shared_ptr<MediaLibraryRdbStore> store, int32_t oldVersion, int32_t newVersion)
{
    bool conn = oldVersion < VERSION_ADD_WRITER_CODE_COLUMN;
    CHECK_AND_RETURN_RET(conn, NativeRdb::E_OK);
    CHECK_AND_RETURN_RET_LOG(store != nullptr, -1, "store is null");
    conn = !this->dbUpgradeUtils_.IsColumnExists(store, PhotoAlbumColumns::TABLE, PhotoAlbumColumns::WRITER_CODE);
    CHECK_AND_RETURN_RET_WARN_LOG(conn,
        NativeRdb::E_OK,
        "column %{public}s already exists in table %{public}s",
        PhotoAlbumColumns::WRITER_CODE.c_str(),
        PhotoAlbumColumns::TABLE.c_str());
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t ret = store->ExecuteSql(this->SQL_WRITER_CODE_COLUMN_ADD);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "add column failed");
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    MEDIA_INFO_LOG("added %{public}s column in %{public}s table, ret: %{public}d, costTime: %{public}" PRId64 "ms.",
        PhotoAlbumColumns::WRITER_CODE.c_str(),
        PhotoAlbumColumns::TABLE.c_str(),
        ret,
        costTime);
    return ret;
}

int32_t PhotoAlbumTableEventHandler::UpgradeTriggerForWriterCode(
    std::shared_ptr<MediaLibraryRdbStore> store, int32_t oldVersion, int32_t newVersion)
{
    bool conn = oldVersion < VERSION_ADD_WRITER_CODE_COLUMN;
    CHECK_AND_RETURN_RET(conn, NativeRdb::E_OK);
    CHECK_AND_RETURN_RET_LOG(store != nullptr, -1, "store is null");
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    const std::vector<std::string> sqls = {
        this->SQL_DROP_ALBUM_DELETE_TRIGGER,
        this->SQL_CREATE_ALBUM_DELETE_TRIGGER,
        this->SQL_DROP_ALBUM_MODIFY_TRIGGER,
        this->SQL_CREATE_ALBUM_MODIFY_TRIGGER,
    };
    int32_t ret = NativeRdb::E_OK;
    for (auto &sql : sqls) {
        ret = store->ExecuteSql(sql);
        CHECK_AND_CONTINUE_ERR_LOG(ret == NativeRdb::E_OK, "execute sql failed");
    }
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    MEDIA_INFO_LOG(
        "upgrade trigger for wirter_code column in %{public}s table, ret: %{public}d, costTime: %{public}" PRId64 "ms.",
        PhotoAlbumColumns::TABLE.c_str(),
        ret,
        costTime);
    return ret;
}
}  // namespace OHOS::Media