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

#include "custom_restore_source_test.h"

#include "custom_restore_const.h"
#include "media_file_utils.h"
#include "media_pure_file_utils.h"
#include "media_path_utils.h"
#include "media_log.h"
#include "rdb_helper.h"

using namespace std;

namespace OHOS {
namespace Media {
const std::string SHARE_RESTORE_MEDIA_INFO_ID = "id";

const std::string CREATE_MEDIA_INFO_TABLE =
    "CREATE TABLE IF NOT EXISTS " + SHARE_RESTORE_TABLE_NAME + " (" + SHARE_RESTORE_MEDIA_INFO_ID +
    " INTEGER PRIMARY KEY AUTOINCREMENT, " + SHARE_RESTORE_MEDIA_INFO_FILE_NAME + " TEXT, " +
    SHARE_RESTORE_MEDIA_INFO_DATE_ADDED + " BIGINT NOT NULL DEFAULT 0, " + SHARE_RESTORE_MEDIA_INFO_DATE_TAKEN +
    " BIGINT NOT NULL DEFAULT 0, " + SHARE_RESTORE_MEDIA_INFO_DETAIL_TIME + " TEXT" + ") ";

int32_t CustomRestoreRdbCallbackTest::OnCreate(NativeRdb::RdbStore &store)
{
    int32_t errCode = store.ExecuteSql(CREATE_MEDIA_INFO_TABLE);
    if (errCode != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute %{public}s failed: %{public}d", CREATE_MEDIA_INFO_TABLE.c_str(), errCode);
        return errCode;
    }
    return NativeRdb::E_OK;
}

int32_t CustomRestoreRdbCallbackTest::OnUpgrade(NativeRdb::RdbStore &store, int oldVersion, int newVersion)
{
    return NativeRdb::E_OK;
}

shared_ptr<NativeRdb::RdbStore> CustomRestoreSourceTest::Init(const string &dbPath)
{
    std::string dbDir = MediaPathUtils::GetParentPath(dbPath);
    if (!MediaFileUtils::IsDirExists(dbDir) && !MediaPureFileUtils::CreateDirectory(dbDir)) {
        MEDIA_ERR_LOG("CreateDirectory failed. dbDir: %{public}s", dbDir.c_str());
        return nullptr;
    }
    const std::string dbName = MediaFileUtils::GetFileName(dbPath);
    NativeRdb::RdbStoreConfig config(dbName);
    config.SetPath(dbPath);
    config.SetSecurityLevel(NativeRdb::SecurityLevel::S2);
    config.SetHaMode(NativeRdb::HAMode::MANUAL_TRIGGER);
    config.SetAllowRebuild(true);
    config.SetWalLimitSize(SHARE_RESTORE_DB_WAL_LIMIT_SIZE);

    CustomRestoreRdbCallbackTest callback;
    int32_t errCode = 0;
    shared_ptr<NativeRdb::RdbStore> rdbStore =
        NativeRdb::RdbHelper::GetRdbStore(config, SHARE_RESTORE_DB_VERSION, callback, errCode);
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("GetRdbStore failed, dbPath: %{public}s, err: %{public}d", dbPath.c_str(), errCode);
        return nullptr;
    }
    return rdbStore;
}

}  // namespace Media
}  // namespace OHOS