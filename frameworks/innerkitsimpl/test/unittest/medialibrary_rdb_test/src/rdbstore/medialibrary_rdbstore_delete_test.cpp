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
#define MLOG_TAG "MediaLibraryRdbStoreDeleteTest"

#include "medialibrary_rdbstore_delete_test.h"

#include <chrono>
#include <cstdint>
#include <thread>

#include "ability_context_impl.h"
#include "context.h"
#include "js_runtime.h"
#include "media_column.h"
#include "media_log.h"
#include "media_upgrade.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_db_const.h"
#include "medialibrary_db_const_sqls.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "rdb_predicates.h"
#include "rdb_table_strategy_manager.h"
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

static shared_ptr<NativeRdb::ResultSet> QueryForTestResult(const string& tableName,
    const string& idColumn, int64_t rowId, const vector<string>& columns)
{
    MEDIA_INFO_LOG("QueryForTestResult, tableName: %{public}s, rowId: %{public}" PRId64,
        tableName.c_str(), rowId);

    NativeRdb::AbsRdbPredicates predicates(tableName);
    predicates.EqualTo(idColumn, rowId);

    EXPECT_NE(rdbStorePtr, nullptr);
    return rdbStorePtr->Query(predicates, columns);
}

void MediaLibraryRdbStoreDeleteTest::SetUpTestCase(void)
{
    auto stageContext = std::make_shared<AbilityRuntime::ContextImpl>();
    auto abilityContextImpl = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    abilityContextImpl->SetStageContext(stageContext);
    rdbStorePtr = std::make_shared<MediaLibraryRdbStore>(abilityContextImpl);
    int32_t ret = rdbStorePtr->Init();
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MediaLibraryRdbStoreDeleteTest rdbstore start ret = %{public}d", ret);
}

void MediaLibraryRdbStoreDeleteTest::TearDownTestCase(void)
{
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void MediaLibraryRdbStoreDeleteTest::SetUp()
{
    CleanTestTables();
    SetTables();
}

void MediaLibraryRdbStoreDeleteTest::TearDown() {}

/**
 * @tc.name: [软删基础测试: rdbStore已停止删除] MediaLibraryRdbStore_DeleteCmd_001
 * @tc.desc: 测试Delete(MediaLibraryCommand &cmd, int32_t &deletedRows)方法在rdbStore停止时删除失败
 *           [1] 创建MediaLibraryCommand并设置删除条件
 *           [2] 调用rdbStorePtr->Stop()停止数据库
 *           [3] 调用Delete方法删除数据
 *           [4] 验证删除失败返回E_HAS_DB_ERROR
 */
HWTEST_F(MediaLibraryRdbStoreDeleteTest, MediaLibraryRdbStore_DeleteCmd_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_DeleteCmd_001");
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::DELETE);
    cmd.SetTableName(PhotoColumn::PHOTOS_TABLE);
    string selection = PhotoColumn::MEDIA_ID + " = ?";
    cmd.GetAbsRdbPredicates()->SetWhereClause(selection);
    vector<string> selectionArgs;
    selectionArgs.push_back("1");
    cmd.GetAbsRdbPredicates()->SetWhereArgs(selectionArgs);

    rdbStorePtr->Stop();
    int32_t deletedRows = 0;
    int32_t ret = rdbStorePtr->Delete(cmd, deletedRows);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    rdbStorePtr->Init();
}

/**
 * @tc.name: [基础测试: 使用AbsRdbPredicates硬删数据] MediaLibraryRdbStore_Delete_001
 * @tc.desc: 测试Delete方法使用AbsRdbPredicates硬删数据
 *           [1] 先插入一条数据
 *           [2] 创建AbsRdbPredicates并设置删除条件
 *           [3] 调用Delete方法硬删数据
 *           [4] 验证删除成功且数据已被删除
 */
HWTEST_F(MediaLibraryRdbStoreDeleteTest, MediaLibraryRdbStore_Delete_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Delete_001");
    // 先插入一条数据
    ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_NAME, "test_delete_predicates.jpg");
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 使用AbsRdbPredicates硬删数据
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, rowId);

    int deletedRows = 0;
    ret = rdbStorePtr->Delete(deletedRows, predicates);
    EXPECT_EQ(ret, E_OK);
    EXPECT_GT(deletedRows, 0);

    // 查询验证数据已被删除
    auto resultSet = QueryForTestResult(PhotoColumn::PHOTOS_TABLE, PhotoColumn::MEDIA_ID, rowId, PHOTOS_QUERY_COLUMNS);
    ASSERT_NE(resultSet, nullptr);
    int rowCount = 0;
    resultSet->GetRowCount(rowCount);
    resultSet->Close();
    EXPECT_EQ(rowCount, 0);
}

/**
 * @tc.name: [基础测试: 使用whereClause硬删数据] MediaLibraryRdbStore_Delete_002
 * @tc.desc: 测试Delete方法使用whereClause硬删数据
 *           [1] 先插入一条数据
 *           [2] 调用Delete方法使用whereClause删除数据
 *           [3] 验证删除成功且数据已被删除
 */
HWTEST_F(MediaLibraryRdbStoreDeleteTest, MediaLibraryRdbStore_Delete_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Delete_002");
    // 先插入一条数据
    ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_NAME, "test_delete_where.jpg");
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 使用whereClause删除数据
    string whereClause = MediaColumn::MEDIA_ID + " = ?";
    vector<string> args = { to_string(rowId) };
    int deletedRows = 0;
    ret = rdbStorePtr->Delete(deletedRows, PhotoColumn::PHOTOS_TABLE, whereClause, args);
    EXPECT_EQ(ret, E_OK);
    EXPECT_GT(deletedRows, 0);

    // 查询验证数据已被删除
    auto resultSet = QueryForTestResult(PhotoColumn::PHOTOS_TABLE, PhotoColumn::MEDIA_ID, rowId, PHOTOS_QUERY_COLUMNS);
    ASSERT_NE(resultSet, nullptr);
    int rowCount = 0;
    resultSet->GetRowCount(rowCount);
    resultSet->Close();
    EXPECT_EQ(rowCount, 0);
}

/**
 * @tc.name: [基础测试: 使用DeleteWithReturn硬删数据] MediaLibraryRdbStore_Delete_003
 * @tc.desc: 测试DeleteWithReturn方法使用AbsRdbPredicates硬删数据并返回结果
 *           [1] 先插入一条数据
 *           [2] 创建AbsRdbPredicates并设置删除条件
 *           [3] 调用DeleteWithReturn方法硬删数据
 *           [4] 验证删除成功且数据已被删除
 */
HWTEST_F(MediaLibraryRdbStoreDeleteTest, MediaLibraryRdbStore_Delete_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Delete_003");
    // 先插入一条数据
    ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_NAME, "test_delete_return.jpg");
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 使用DeleteWithReturn硬删数据
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, rowId);

    auto result = rdbStorePtr->DeleteWithReturn(predicates, PhotoColumn::MEDIA_ID);
    EXPECT_EQ(result.first, E_OK);

    // 查询验证数据已被删除
    auto resultSet = QueryForTestResult(PhotoColumn::PHOTOS_TABLE, PhotoColumn::MEDIA_ID, rowId, PHOTOS_QUERY_COLUMNS);
    ASSERT_NE(resultSet, nullptr);
    int rowCount = 0;
    resultSet->GetRowCount(rowCount);
    resultSet->Close();
    EXPECT_EQ(rowCount, 0);
}

/**
 * @tc.name: [策略测试: Photos表软删除] MediaLibraryRdbStore_Delete_PhotoTable_001
 * @tc.desc: 测试PhotosTableStrategy的ExtendDeleteValues正确设置脏标记和同步状态
 *           [1] 插入Photos表数据
 *           [2] 调用Delete方法软删除数据
 *           [3] 验证PHOTO_DIRTY=TYPE_DELETED, PHOTO_SYNC_STATUS=TYPE_UPLOAD, PHOTO_META_DATE_MODIFIED被更新
 */
HWTEST_F(MediaLibraryRdbStoreDeleteTest, MediaLibraryRdbStore_Delete_PhotoTable_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Delete_PhotoTable_001");
    // 插入Photos表数据
    ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_NAME, "test_delete_strategy.jpg");
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 软删除数据
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, rowId);
    ret = MediaLibraryRdbStore::Delete(predicates);
    EXPECT_EQ(ret, 1);

    // 查询验证策略字段
    auto resultSet = QueryForTestResult(PhotoColumn::PHOTOS_TABLE, PhotoColumn::MEDIA_ID, rowId, PHOTOS_QUERY_COLUMNS);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    int32_t dirty = GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet);
    int32_t syncStatus = GetInt32Val(PhotoColumn::PHOTO_SYNC_STATUS, resultSet);
    int64_t dateModified = GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet);
    resultSet->Close();
    EXPECT_EQ(dirty, static_cast<int32_t>(DirtyType::TYPE_DELETED));
    EXPECT_EQ(syncStatus, static_cast<int32_t>(SyncStatusType::TYPE_UPLOAD));
    EXPECT_GT(dateModified, 0);
}

/**
 * @tc.name: [策略测试: PhotoAlbum表软删除] MediaLibraryRdbStore_Delete_PhotoAlbumTable_001
 * @tc.desc: 测试PhotoAlbumTableStrategy的ExtendDeleteValues正确设置脏标记
 *           [1] 插入PhotoAlbum表数据
 *           [2] 调用Delete方法软删除数据
 *           [3] 验证ALBUM_DIRTY=TYPE_DELETED被更新
 */
HWTEST_F(MediaLibraryRdbStoreDeleteTest, MediaLibraryRdbStore_Delete_PhotoAlbumTable_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Delete_PhotoAlbumTable_001");
    // 插入PhotoAlbum表数据
    ValuesBucket values;
    values.Put(PhotoAlbumColumns::ALBUM_NAME, "test_delete_album");
    values.Put(PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumType::USER);
    values.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::USER_GENERIC);
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoAlbumColumns::TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 软删除数据
    AbsRdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, rowId);
    ret = MediaLibraryRdbStore::Delete(predicates);
    EXPECT_EQ(ret, 1);

    // 查询验证策略字段
    auto resultSet = QueryForTestResult(
        PhotoAlbumColumns::TABLE, PhotoAlbumColumns::ALBUM_ID, rowId, PHOTO_ALBUM_QUERY_COLUMNS);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    int32_t dirty = GetInt32Val(PhotoAlbumColumns::ALBUM_DIRTY, resultSet);
    resultSet->Close();
    EXPECT_EQ(dirty, static_cast<int32_t>(DirtyType::TYPE_DELETED));
}

/**
 * @tc.name: [策略测试: Files表软删除] MediaLibraryRdbStore_Delete_FilesTable_001
 * @tc.desc: 测试FilesTableStrategy的ExtendDeleteValues正确设置脏标记和同步状态
 *           [1] 插入Files表数据
 *           [2] 调用Delete方法软删除数据
 *           [3] 验证DIRTY=TYPE_DELETED, SYNC_STATUS=TYPE_UPLOAD, PHOTO_META_DATE_MODIFIED被更新
 */
HWTEST_F(MediaLibraryRdbStoreDeleteTest, MediaLibraryRdbStore_Delete_FilesTable_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Delete_FilesTable_001");
    // 插入Files表数据
    ValuesBucket values;
    values.Put(CONST_MEDIA_DATA_DB_FILE_PATH, "/test/path/file_delete.jpg");
    values.Put(MEDIA_DATA_DB_NAME, "file_delete.jpg");
    values.Put(CONST_MEDIA_DATA_DB_MEDIA_TYPE, 1);
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, CONST_MEDIALIBRARY_TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 软删除数据
    AbsRdbPredicates predicates(CONST_MEDIALIBRARY_TABLE);
    predicates.EqualTo(CONST_MEDIA_DATA_DB_ID, rowId);
    ret = MediaLibraryRdbStore::Delete(predicates);
    EXPECT_EQ(ret, 1);

    // 查询验证策略字段
    auto resultSet = QueryForTestResult(CONST_MEDIALIBRARY_TABLE, CONST_MEDIA_DATA_DB_ID, rowId, FILES_QUERY_COLUMNS);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    int32_t dirty = GetInt32Val(CONST_MEDIA_DATA_DB_DIRTY, resultSet);
    int32_t syncStatus = GetInt32Val(CONST_MEDIA_DATA_DB_SYNC_STATUS, resultSet);
    int64_t dateModified = GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet);
    resultSet->Close();
    EXPECT_EQ(dirty, static_cast<int32_t>(DirtyType::TYPE_DELETED));
    EXPECT_EQ(syncStatus, static_cast<int32_t>(SyncStatusType::TYPE_UPLOAD));
    EXPECT_GT(dateModified, 0);
}

/**
 * @tc.name: [策略测试: PhotoMap表软删除] MediaLibraryRdbStore_Delete_PhotoMapTable_001
 * @tc.desc: 测试PhotoMapTableStrategy的ExtendDeleteValues正确设置脏标记
 *           [1] 插入PhotoMap表数据
 *           [2] 调用Delete方法软删除数据
 *           [3] 验证DIRTY=TYPE_DELETED被更新
 */
HWTEST_F(MediaLibraryRdbStoreDeleteTest, MediaLibraryRdbStore_Delete_PhotoMapTable_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Delete_PhotoMapTable_001");
    // 插入PhotoMap表数据
    ValuesBucket values;
    values.Put(PhotoMap::ALBUM_ID, 1);
    values.Put(PhotoMap::ASSET_ID, 100);
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoMap::TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 软删除数据 - PhotoMap使用复合主键
    AbsRdbPredicates predicates(PhotoMap::TABLE);
    predicates.EqualTo(PhotoMap::ALBUM_ID, 1);
    predicates.EqualTo(PhotoMap::ASSET_ID, 100);
    ret = MediaLibraryRdbStore::Delete(predicates);
    EXPECT_EQ(ret, 1);

    // 查询验证策略字段
    auto resultSet = QueryForTestResult(PhotoMap::TABLE, PhotoMap::ALBUM_ID, 1, PHOTO_MAP_QUERY_COLUMNS);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    int32_t dirty = GetInt32Val(PhotoMap::DIRTY, resultSet);
    resultSet->Close();
    EXPECT_EQ(dirty, static_cast<int32_t>(DirtyType::TYPE_DELETED));
}

/**
 * @tc.name: [策略测试: 无策略表删除] MediaLibraryRdbStore_Delete_NoStrategyTable_001
 * @tc.desc: 测试没有对应表策略时删除操作正常执行
 *           [1] 创建测试表但不注册策略
 *           [2] 插入测试数据
 *           [3] 调用Delete方法删除数据
 *           [4] 验证删除成功且数据已被删除
 */
HWTEST_F(MediaLibraryRdbStoreDeleteTest, MediaLibraryRdbStore_Delete_NoStrategyTable_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Delete_NoStrategyTable_001");
    // 创建测试表
    string createTestTableSql = "CREATE TABLE IF NOT EXISTS test_no_strategy_table ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT, "
        "value INTEGER)";
    int32_t ret = rdbStorePtr->ExecuteSql(createTestTableSql);
    EXPECT_EQ(ret, NativeRdb::E_OK);

    // 插入测试数据
    ValuesBucket values;
    values.Put("name", "test_no_strategy");
    values.Put("value", 999);
    int64_t rowId = -1;
    ret = MediaLibraryRdbStore::Insert(rowId, "test_no_strategy_table", values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 删除数据 - 没有注册策略，应该直接删除
    AbsRdbPredicates predicates("test_no_strategy_table");
    predicates.EqualTo("id", rowId);
    ret = MediaLibraryRdbStore::Delete(predicates);
    EXPECT_EQ(ret, 1);

    // 查询验证数据已被删除
    auto resultSet = QueryForTestResult("test_no_strategy_table", "id", rowId, {"id", "name", "value"});
    ASSERT_NE(resultSet, nullptr);
    int rowCount = 0;
    resultSet->GetRowCount(rowCount);
    resultSet->Close();
    EXPECT_EQ(rowCount, 0);

    // 清理测试表
    string dropTestTableSql = "DROP TABLE IF EXISTS test_no_strategy_table";
    ret = rdbStorePtr->ExecuteSql(dropTestTableSql);
    EXPECT_EQ(ret, NativeRdb::E_OK);
}

} // namespace Media
} // namespace OHOS