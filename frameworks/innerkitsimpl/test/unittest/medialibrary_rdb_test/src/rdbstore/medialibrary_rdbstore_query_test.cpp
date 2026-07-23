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
#define MLOG_TAG "MediaLibraryRdbStoreQueryTest"

#include "medialibrary_rdbstore_query_test.h"

#include <cstdint>

#include "ability_context_impl.h"
#include "context.h"
#include "js_runtime.h"
#include "media_column.h"
#include "media_log.h"
#include "media_upgrade.h"
#include "medialibrary_command.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_db_const.h"
#include "medialibrary_db_const_sqls.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "rdb_predicates.h"
#include "result_set_utils.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace NativeRdb;

namespace OHOS {
namespace Media {

static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
static shared_ptr<MediaLibraryRdbStore> rdbStorePtr = nullptr;

// 查询列定义 - Photos表
static const vector<string> PHOTOS_QUERY_COLUMNS = {
    PhotoColumn::MEDIA_ID,
    PhotoColumn::MEDIA_NAME,
    PhotoColumn::PHOTO_DIRTY,
    PhotoColumn::PHOTO_SYNC_STATUS,
    PhotoColumn::PHOTO_META_DATE_MODIFIED,
};

// 查询列定义 - PhotoAlbum表
static const vector<string> PHOTO_ALBUM_QUERY_COLUMNS = {
    PhotoAlbumColumns::ALBUM_ID,
    PhotoAlbumColumns::ALBUM_NAME,
    PhotoAlbumColumns::ALBUM_DIRTY,
};

// 查询列定义 - Files表
static const vector<string> FILES_QUERY_COLUMNS = {
    CONST_MEDIA_DATA_DB_ID,
    CONST_MEDIA_DATA_DB_DIRTY,
    CONST_MEDIA_DATA_DB_SYNC_STATUS,
    CONST_MEDIA_DATA_DB_NAME,
    PhotoColumn::PHOTO_META_DATE_MODIFIED,
};

// 查询列定义 - PhotoMap表
static const vector<string> PHOTO_MAP_QUERY_COLUMNS = {
    PhotoMap::ALBUM_ID,
    PhotoMap::ASSET_ID,
    PhotoMap::DIRTY,
};

static void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        PhotoAlbumColumns::TABLE,
        CONST_MEDIALIBRARY_TABLE,
        PhotoMap::TABLE,
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

static void SetTables()
{
    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        PhotoAlbumColumns::CREATE_TABLE,
        CREATE_MEDIA_TABLE,
        PhotoMap::CREATE_TABLE,
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

void MediaLibraryRdbStoreQueryTest::SetUpTestCase(void)
{
    auto stageContext = std::make_shared<AbilityRuntime::ContextImpl>();
    auto abilityContextImpl = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    abilityContextImpl->SetStageContext(stageContext);
    rdbStorePtr = std::make_shared<MediaLibraryRdbStore>(abilityContextImpl);
    int32_t ret = rdbStorePtr->Init();
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MediaLibraryRdbStoreQueryTest rdbstore start ret = %{public}d", ret);
}

void MediaLibraryRdbStoreQueryTest::TearDownTestCase(void)
{
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void MediaLibraryRdbStoreQueryTest::SetUp()
{
    CleanTestTables();
    SetTables();
}

void MediaLibraryRdbStoreQueryTest::TearDown() {}

// 检查查询结果是否为空
static bool CheckQueryResultEmpty(shared_ptr<ResultSet> resultSet)
{
    if (resultSet == nullptr) {
        MEDIA_INFO_LOG("resultSet is nullptr.");
        return true;
    }
    int rowCount = 0;
    resultSet->GetRowCount(rowCount);
    resultSet->Close();
    MEDIA_INFO_LOG("rowCount: %{public}d.", rowCount);
    return rowCount == 0;
}

// 检查查询结果是否包含指定数据
static bool CheckQueryResultHasData(shared_ptr<ResultSet> resultSet, const string& columnName,
    const string& expectedValue)
{
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet is nullptr.");
        return false;
    }
    if (resultSet->GoToFirstRow() != E_OK) {
        MEDIA_ERR_LOG("failed to GoToFirstRow.");
        resultSet->Close();
        return false;
    }
    string value = GetStringVal(columnName, resultSet);
    resultSet->Close();
    MEDIA_INFO_LOG("value: %{public}s, expectedValue: %{public}s.", value.c_str(), expectedValue.c_str());
    return value == expectedValue;
}

/**
 * @tc.name: [策略测试: Photos表查询过滤] MediaLibraryRdbStore_Query_PhotoTable_001
 * @tc.desc: 测试不同查询接口在PHOTO_SYNC_STATUS!=0时的过滤行为
 *           [1] 插入Photos表数据，PHOTO_SYNC_STATUS=1
 *           [2] Query(MediaLibraryCommand) - 被策略过滤，查不到数据
 *           [3] QueryWithFilter - 被策略过滤，查不到数据
 *           [4] QueryByStepWithoutCount - 被策略过滤，查不到数据
 *           [5] Query(AbsRdbPredicates) - 不过滤，可以查询到数据
 *           [6] QueryByStep - 不过滤，可以查询到数据
 */
HWTEST_F(MediaLibraryRdbStoreQueryTest, MediaLibraryRdbStore_Query_PhotoTable_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Query_PhotoTable_001");
    // 插入Photos表数据，PHOTO_SYNC_STATUS=1
    ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_NAME, "test_query_sync_status.jpg");
    values.Put(PhotoColumn::PHOTO_SYNC_STATUS, 1);
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 1. Query(MediaLibraryCommand) - 被策略过滤，查不到数据
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    cmd.SetTableName(PhotoColumn::PHOTOS_TABLE);
    string selection = PhotoColumn::MEDIA_ID + " = ?";
    cmd.GetAbsRdbPredicates()->SetWhereClause(selection);
    vector<string> selectionArgs;
    selectionArgs.push_back(to_string(rowId));
    cmd.GetAbsRdbPredicates()->SetWhereArgs(selectionArgs);

    auto resultSet = rdbStorePtr->Query(cmd, PHOTOS_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultEmpty(resultSet));

    // 2. QueryWithFilter - 被策略过滤，查不到数据
    AbsRdbPredicates predicates1(PhotoColumn::PHOTOS_TABLE);
    predicates1.EqualTo(PhotoColumn::MEDIA_ID, rowId);
    resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates1, PHOTOS_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultEmpty(resultSet));

    // 3. QueryByStepWithoutCount - 被策略过滤，查不到数据
    AbsRdbPredicates predicates2(PhotoColumn::PHOTOS_TABLE);
    predicates2.EqualTo(PhotoColumn::MEDIA_ID, rowId);
    resultSet = MediaLibraryRdbStore::QueryByStepWithoutCount(predicates2, PHOTOS_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultEmpty(resultSet));

    // 4. Query(AbsRdbPredicates) - 不过滤，可以查询到数据
    AbsRdbPredicates predicates3(PhotoColumn::PHOTOS_TABLE);
    predicates3.EqualTo(PhotoColumn::MEDIA_ID, rowId);
    resultSet = MediaLibraryRdbStore::Query(predicates3, PHOTOS_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultHasData(resultSet, PhotoColumn::MEDIA_NAME, "test_query_sync_status.jpg"));

    // 5. QueryByStep - 不过滤，可以查询到数据
    AbsRdbPredicates predicates4(PhotoColumn::PHOTOS_TABLE);
    predicates4.EqualTo(PhotoColumn::MEDIA_ID, rowId);
    resultSet = rdbStorePtr->QueryByStep(predicates4, PHOTOS_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultHasData(resultSet, PhotoColumn::MEDIA_NAME, "test_query_sync_status.jpg"));
}

/**
 * @tc.name: [策略测试: Photos表查询过滤] MediaLibraryRdbStore_Query_PhotoTable_002
 * @tc.desc: 测试不同查询接口在PHOTO_CLEAN_FLAG!=0时的过滤行为
 *           [1] 插入Photos表数据，PHOTO_CLEAN_FLAG=1
 *           [2] Query(MediaLibraryCommand) - 被策略过滤，查不到数据
 *           [3] QueryWithFilter - 被策略过滤，查不到数据
 *           [4] QueryByStepWithoutCount - 被策略过滤，查不到数据
 *           [5] Query(AbsRdbPredicates) - 不过滤，可以查询到数据
 *           [6] QueryByStep - 不过滤，可以查询到数据
 */
HWTEST_F(MediaLibraryRdbStoreQueryTest, MediaLibraryRdbStore_Query_PhotoTable_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Query_PhotoTable_002");
    // 插入Photos表数据，PHOTO_CLEAN_FLAG=1
    ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_NAME, "test_query_clean_flag.jpg");
    values.Put(PhotoColumn::PHOTO_CLEAN_FLAG, 1);
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 1. Query(MediaLibraryCommand) - 被策略过滤，查不到数据
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    cmd.SetTableName(PhotoColumn::PHOTOS_TABLE);
    string selection = PhotoColumn::MEDIA_ID + " = ?";
    cmd.GetAbsRdbPredicates()->SetWhereClause(selection);
    vector<string> selectionArgs;
    selectionArgs.push_back(to_string(rowId));
    cmd.GetAbsRdbPredicates()->SetWhereArgs(selectionArgs);

    auto resultSet = rdbStorePtr->Query(cmd, PHOTOS_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultEmpty(resultSet));

    // 2. QueryWithFilter - 被策略过滤，查不到数据
    AbsRdbPredicates predicates1(PhotoColumn::PHOTOS_TABLE);
    predicates1.EqualTo(PhotoColumn::MEDIA_ID, rowId);
    resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates1, PHOTOS_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultEmpty(resultSet));

    // 3. QueryByStepWithoutCount - 被策略过滤，查不到数据
    AbsRdbPredicates predicates2(PhotoColumn::PHOTOS_TABLE);
    predicates2.EqualTo(PhotoColumn::MEDIA_ID, rowId);
    resultSet = MediaLibraryRdbStore::QueryByStepWithoutCount(predicates2, PHOTOS_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultEmpty(resultSet));

    // 4. Query(AbsRdbPredicates) - 不过滤，可以查询到数据
    AbsRdbPredicates predicates3(PhotoColumn::PHOTOS_TABLE);
    predicates3.EqualTo(PhotoColumn::MEDIA_ID, rowId);
    resultSet = MediaLibraryRdbStore::Query(predicates3, PHOTOS_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultHasData(resultSet, PhotoColumn::MEDIA_NAME, "test_query_clean_flag.jpg"));

    // 5. QueryByStep - 不过滤，可以查询到数据
    AbsRdbPredicates predicates4(PhotoColumn::PHOTOS_TABLE);
    predicates4.EqualTo(PhotoColumn::MEDIA_ID, rowId);
    resultSet = rdbStorePtr->QueryByStep(predicates4, PHOTOS_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultHasData(resultSet, PhotoColumn::MEDIA_NAME, "test_query_clean_flag.jpg"));
}

/**
 * @tc.name: [策略测试: PhotoAlbum表查询过滤] MediaLibraryRdbStore_Query_PhotoAlbumTable_001
 * @tc.desc: 测试不同查询接口在ALBUM_DIRTY=TYPE_DELETED时的过滤行为
 *           [1] 插入PhotoAlbum表数据，ALBUM_DIRTY=TYPE_DELETED
 *           [2] Query(MediaLibraryCommand) - 被策略过滤，查不到数据
 *           [3] QueryWithFilter - 被策略过滤，查不到数据
 *           [4] QueryByStepWithoutCount - 被策略过滤，查不到数据
 *           [5] Query(AbsRdbPredicates) - 不过滤，可以查询到数据
 *           [6] QueryByStep - 不过滤，可以查询到数据
 */
HWTEST_F(MediaLibraryRdbStoreQueryTest, MediaLibraryRdbStore_Query_PhotoAlbumTable_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Query_PhotoAlbumTable_001");
    // 插入PhotoAlbum表数据，ALBUM_DIRTY=TYPE_DELETED
    ValuesBucket values;
    values.Put(PhotoAlbumColumns::ALBUM_NAME, "test_query_album");
    values.Put(PhotoAlbumColumns::ALBUM_DIRTY, static_cast<int32_t>(DirtyType::TYPE_DELETED));
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoAlbumColumns::TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 1. Query(MediaLibraryCommand) - 被策略过滤，查不到数据
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ALBUM, OperationType::QUERY);
    cmd.SetTableName(PhotoAlbumColumns::TABLE);
    string selection = PhotoAlbumColumns::ALBUM_ID + " = ?";
    cmd.GetAbsRdbPredicates()->SetWhereClause(selection);
    vector<string> selectionArgs;
    selectionArgs.push_back(to_string(rowId));
    cmd.GetAbsRdbPredicates()->SetWhereArgs(selectionArgs);

    auto resultSet = rdbStorePtr->Query(cmd, PHOTO_ALBUM_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultEmpty(resultSet));

    // 2. QueryWithFilter - 被策略过滤，查不到数据
    AbsRdbPredicates predicates1(PhotoAlbumColumns::TABLE);
    predicates1.EqualTo(PhotoAlbumColumns::ALBUM_ID, rowId);
    resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates1, PHOTO_ALBUM_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultEmpty(resultSet));

    // 3. QueryByStepWithoutCount - 被策略过滤，查不到数据
    AbsRdbPredicates predicates2(PhotoAlbumColumns::TABLE);
    predicates2.EqualTo(PhotoAlbumColumns::ALBUM_ID, rowId);
    resultSet = MediaLibraryRdbStore::QueryByStepWithoutCount(predicates2, PHOTO_ALBUM_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultEmpty(resultSet));

    // 4. Query(AbsRdbPredicates) - 不过滤，可以查询到数据
    AbsRdbPredicates predicates3(PhotoAlbumColumns::TABLE);
    predicates3.EqualTo(PhotoAlbumColumns::ALBUM_ID, rowId);
    resultSet = MediaLibraryRdbStore::Query(predicates3, PHOTO_ALBUM_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultHasData(resultSet, PhotoAlbumColumns::ALBUM_NAME, "test_query_album"));

    // 5. QueryByStep - 不过滤，可以查询到数据
    AbsRdbPredicates predicates4(PhotoAlbumColumns::TABLE);
    predicates4.EqualTo(PhotoAlbumColumns::ALBUM_ID, rowId);
    resultSet = rdbStorePtr->QueryByStep(predicates4, PHOTO_ALBUM_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultHasData(resultSet, PhotoAlbumColumns::ALBUM_NAME, "test_query_album"));
}

/**
 * @tc.name: [策略测试: Files表查询过滤] MediaLibraryRdbStore_Query_FilesTable_001
 * @tc.desc: 测试不同查询接口在sync_status!=TYPE_VISIBLE时的过滤行为
 *           [1] 插入Files表数据，sync_status=1
 *           [2] Query(MediaLibraryCommand) - 被策略过滤，查不到数据
 *           [3] QueryWithFilter - 被策略过滤，查不到数据
 *           [4] QueryByStepWithoutCount - 被策略过滤，查不到数据
 *           [5] Query(AbsRdbPredicates) - 不过滤，可以查询到数据
 *           [6] QueryByStep - 不过滤，可以查询到数据
 */
HWTEST_F(MediaLibraryRdbStoreQueryTest, MediaLibraryRdbStore_Query_FilesTable_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Query_FilesTable_001");
    // 插入Files表数据，sync_status=1
    ValuesBucket values;
    values.Put(CONST_MEDIA_DATA_DB_NAME, "test_query_files.jpg");
    values.Put(CONST_MEDIA_DATA_DB_SYNC_STATUS, 1);
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, CONST_MEDIALIBRARY_TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 1. Query(MediaLibraryCommand) - 被策略过滤，查不到数据
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::QUERY);
    cmd.SetTableName(CONST_MEDIALIBRARY_TABLE);
    string selection = string(CONST_MEDIA_DATA_DB_ID) + " = ?";
    cmd.GetAbsRdbPredicates()->SetWhereClause(selection);
    vector<string> selectionArgs;
    selectionArgs.push_back(to_string(rowId));
    cmd.GetAbsRdbPredicates()->SetWhereArgs(selectionArgs);

    auto resultSet = rdbStorePtr->Query(cmd, FILES_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultEmpty(resultSet));

    // 2. QueryWithFilter - 被策略过滤，查不到数据
    AbsRdbPredicates predicates1(CONST_MEDIALIBRARY_TABLE);
    predicates1.EqualTo(CONST_MEDIA_DATA_DB_ID, rowId);
    resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates1, FILES_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultEmpty(resultSet));

    // 3. QueryByStepWithoutCount - 被策略过滤，查不到数据
    AbsRdbPredicates predicates2(CONST_MEDIALIBRARY_TABLE);
    predicates2.EqualTo(CONST_MEDIA_DATA_DB_ID, rowId);
    resultSet = MediaLibraryRdbStore::QueryByStepWithoutCount(predicates2, FILES_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultEmpty(resultSet));

    // 4. Query(AbsRdbPredicates) - 不过滤，可以查询到数据
    AbsRdbPredicates predicates3(CONST_MEDIALIBRARY_TABLE);
    predicates3.EqualTo(CONST_MEDIA_DATA_DB_ID, rowId);
    resultSet = MediaLibraryRdbStore::Query(predicates3, FILES_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultHasData(resultSet, CONST_MEDIA_DATA_DB_NAME, "test_query_files.jpg"));

    // 5. QueryByStep - 不过滤，可以查询到数据
    AbsRdbPredicates predicates4(CONST_MEDIALIBRARY_TABLE);
    predicates4.EqualTo(CONST_MEDIA_DATA_DB_ID, rowId);
    resultSet = rdbStorePtr->QueryByStep(predicates4, FILES_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultHasData(resultSet, CONST_MEDIA_DATA_DB_NAME, "test_query_files.jpg"));
}

/**
 * @tc.name: [策略测试: PhotoMap表查询过滤] MediaLibraryRdbStore_Query_PhotoMapTable_001
 * @tc.desc: 测试不同查询接口在DIRTY=TYPE_DELETED时的过滤行为
 *           [1] 插入PhotoMap表数据，DIRTY=TYPE_DELETED
 *           [2] Query(MediaLibraryCommand) - 被策略过滤，查不到数据
 *           [3] QueryWithFilter - 被策略过滤，查不到数据
 *           [4] QueryByStepWithoutCount - 被策略过滤，查不到数据
 *           [5] Query(AbsRdbPredicates) - 不过滤，可以查询到数据
 *           [6] QueryByStep - 不过滤，可以查询到数据
 */
HWTEST_F(MediaLibraryRdbStoreQueryTest, MediaLibraryRdbStore_Query_PhotoMapTable_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Query_PhotoMapTable_001");
    // 插入PhotoMap表数据，DIRTY=TYPE_DELETED
    ValuesBucket values;
    values.Put(PhotoMap::ALBUM_ID, 100);
    values.Put(PhotoMap::ASSET_ID, 200);
    values.Put(PhotoMap::DIRTY, static_cast<int32_t>(DirtyType::TYPE_DELETED));
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoMap::TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 1. Query(MediaLibraryCommand) - 被策略过滤，查不到数据
    MediaLibraryCommand cmd(OperationObject::PHOTO_MAP, OperationType::QUERY);
    cmd.SetTableName(PhotoMap::TABLE);
    string selection = PhotoMap::ALBUM_ID + " = ?";
    cmd.GetAbsRdbPredicates()->SetWhereClause(selection);
    vector<string> selectionArgs;
    selectionArgs.push_back("100");
    cmd.GetAbsRdbPredicates()->SetWhereArgs(selectionArgs);

    auto resultSet = rdbStorePtr->Query(cmd, PHOTO_MAP_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultEmpty(resultSet));

    // 2. QueryWithFilter - 被策略过滤，查不到数据
    AbsRdbPredicates predicates1(PhotoMap::TABLE);
    predicates1.EqualTo(PhotoMap::ALBUM_ID, 100);
    resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates1, PHOTO_MAP_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultEmpty(resultSet));

    // 3. QueryByStepWithoutCount - 被策略过滤，查不到数据
    AbsRdbPredicates predicates2(PhotoMap::TABLE);
    predicates2.EqualTo(PhotoMap::ALBUM_ID, 100);
    resultSet = MediaLibraryRdbStore::QueryByStepWithoutCount(predicates2, PHOTO_MAP_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultEmpty(resultSet));

    // 4. Query(AbsRdbPredicates) - 不过滤，可以查询到数据
    AbsRdbPredicates predicates3(PhotoMap::TABLE);
    predicates3.EqualTo(PhotoMap::ALBUM_ID, 100);
    resultSet = MediaLibraryRdbStore::Query(predicates3, PHOTO_MAP_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultHasData(resultSet, PhotoMap::ALBUM_ID, "100"));

    // 5. QueryByStep - 不过滤，可以查询到数据
    AbsRdbPredicates predicates4(PhotoMap::TABLE);
    predicates4.EqualTo(PhotoMap::ALBUM_ID, 100);
    resultSet = rdbStorePtr->QueryByStep(predicates4, PHOTO_MAP_QUERY_COLUMNS);
    EXPECT_TRUE(CheckQueryResultHasData(resultSet, PhotoMap::ALBUM_ID, "100"));
}

} // namespace Media
} // namespace OHOS