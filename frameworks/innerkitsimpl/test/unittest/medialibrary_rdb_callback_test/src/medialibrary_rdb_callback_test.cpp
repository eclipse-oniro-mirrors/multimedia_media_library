/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define MLOG_TAG "FileExtUnitTest"

#include <chrono>
#include <thread>
#include "context.h"
#include "ability_context_impl.h"
#include "js_runtime.h"
#include "photo_album_column.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_db_const.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_rdb_callback_test.h"
#include "rdbstore_mock.h"
#define private public
#include "medialibrary_object_utils.h"
#include "medialibrary_rdbstore.h"
#undef private
#include "media_upgrade.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {
const std::string QUERY_MEDIA_VOLUME = "SELECT sum(" + MEDIA_DATA_DB_SIZE + ") AS " +
    MEDIA_DATA_DB_SIZE + "," +
    MEDIA_DATA_DB_MEDIA_TYPE + " FROM " +
    MEDIALIBRARY_TABLE + " WHERE " +
    MEDIA_DATA_DB_MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_FILE) + " OR " +
    MEDIA_DATA_DB_MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_IMAGE) + " OR " +
    MEDIA_DATA_DB_MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_VIDEO) + " OR " +
    MEDIA_DATA_DB_MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_ALBUM) + " OR " +
    MEDIA_DATA_DB_MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_AUDIO) + " GROUP BY " +
    MEDIA_DATA_DB_MEDIA_TYPE;

shared_ptr <MediaLibraryRdbStore> rdbStorePtr = nullptr;

void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        MEDIALIBRARY_TABLE,
    };
    for (auto &dropTable : dropTableList) {
        string dropSql = "DROP TABLE " + dropTable + ";";
        int32_t ret = rdbStorePtr->ExecuteSql(dropSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Drop %{public}s table failed", dropTable.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Drop %{public}s table success", dropTable.c_str());
    }
}

void SetTables()
{
    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        CREATE_MEDIA_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        if (rdbStorePtr == nullptr) {
            MEDIA_ERR_LOG("can not get rdbStorePtr");
            return;
        }
        int32_t ret = rdbStorePtr->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}

void MediaLibraryRdbCallbackTest::SetUpTestCase(void)
{
    auto stageContext = std::make_shared<AbilityRuntime::ContextImpl>();
    auto abilityContextImpl = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    abilityContextImpl->SetStageContext(stageContext);
    rdbStorePtr = std::make_shared<MediaLibraryRdbStore>(abilityContextImpl);
    int32_t ret = rdbStorePtr->Init();
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MediaLibraryRdbCallbackTest rdbstore start ret = %{public}d", ret);
}

void MediaLibraryRdbCallbackTest::TearDownTestCase(void) {}

// SetUp:Execute before each test case
void MediaLibraryRdbCallbackTest::SetUp() {}

void MediaLibraryRdbCallbackTest::TearDown(void) {}

HWTEST_F(MediaLibraryRdbCallbackTest, MediaLibraryDataCallback_OnUpgrade_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryDataCallback_OnUpgrade_001 begin");
    MediaLibraryDataCallBack callback;
    RdbStoreMock store;
    auto res = callback.OnUpgrade(store, 0, MEDIA_RDB_VERSION);
    EXPECT_EQ(res, NativeRdb::E_OK);
    MEDIA_INFO_LOG("MediaLibraryDataCallback_OnUpgrade_001 end");
}

HWTEST_F(MediaLibraryRdbCallbackTest, MediaLibraryDataCallback_OnUpgrade_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryDataCallback_OnUpgrade_002 begin");
    MediaLibraryDataCallBack callback;
    RdbStoreMock store;
    auto res = callback.OnUpgrade(store, MEDIA_RDB_VERSION, 0);
    EXPECT_EQ(res, NativeRdb::E_OK);
    MEDIA_INFO_LOG("MediaLibraryDataCallback_OnUpgrade_002 end");
}
} // namespace Media
} // namespace OHOS