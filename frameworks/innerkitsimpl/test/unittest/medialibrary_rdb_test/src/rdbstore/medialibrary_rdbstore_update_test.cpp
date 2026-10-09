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
#define MLOG_TAG "MediaLibraryRdbStoreUpdateTest"

#include "medialibrary_rdbstore_update_test.h"

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
    PhotoColumn::PHOTO_META_DATE_MODIFIED,
    PhotoColumn::PHOTO_LAST_VISIT_TIME,
};

static void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
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

static shared_ptr<ResultSet> QueryForTestResult(const string& tableName,
    const string& idColumn, int64_t rowId, const vector<string>& columns)
{
    MEDIA_INFO_LOG("QueryForTestResult, tableName: %{public}s, rowId: %{public}" PRId64,
        tableName.c_str(), rowId);

    NativeRdb::AbsRdbPredicates predicates(tableName);
    predicates.EqualTo(idColumn, rowId);

    EXPECT_NE(rdbStorePtr, nullptr);
    return rdbStorePtr->Query(predicates, columns);
}

void MediaLibraryRdbStoreUpdateTest::SetUpTestCase(void)
{
    auto stageContext = std::make_shared<AbilityRuntime::ContextImpl>();
    auto abilityContextImpl = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    abilityContextImpl->SetStageContext(stageContext);
    rdbStorePtr = std::make_shared<MediaLibraryRdbStore>(abilityContextImpl);
    int32_t ret = rdbStorePtr->Init();
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MediaLibraryRdbStoreUpdateTest rdbstore start ret = %{public}d", ret);
}

void MediaLibraryRdbStoreUpdateTest::TearDownTestCase(void)
{
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void MediaLibraryRdbStoreUpdateTest::SetUp()
{
    CleanTestTables();
    SetTables();
}

void MediaLibraryRdbStoreUpdateTest::TearDown() {}

/**
 * @tc.name: [策略测试: Photos表更新] MediaLibraryRdbStore_UpdateCmd_001
 * @tc.desc: 测试Update(MediaLibraryCommand &cmd, int32_t &changedRows)方法更新Photos表数据
 *           [1] 插入Photos表数据并验证初始PHOTO_LAST_VISIT_TIME为0
 *           [2] 创建MediaLibraryCommand并设置更新条件
 *           [3] 调用Update方法更新数据
 *           [4] 验证更新成功且PHOTO_META_DATE_MODIFIED、PHOTO_LAST_VISIT_TIME被更新
 */
HWTEST_F(MediaLibraryRdbStoreUpdateTest, MediaLibraryRdbStore_UpdateCmd_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_UpdateCmd_001");
    // 先插入一条数据
    ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_NAME, "test_update.jpg");
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 获取初始时间戳
    auto resultSet = QueryForTestResult(PhotoColumn::PHOTOS_TABLE, PhotoColumn::MEDIA_ID, rowId, PHOTOS_QUERY_COLUMNS);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    int64_t initialDateModified = GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet);
    int64_t initialLastVisitTime = GetInt64Val(PhotoColumn::PHOTO_LAST_VISIT_TIME, resultSet);
    resultSet->Close();
    EXPECT_EQ(initialLastVisitTime, 0);

    // 使用MediaLibraryCommand更新数据
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    cmd.SetTableName(PhotoColumn::PHOTOS_TABLE);
    ValuesBucket updateValues;
    updateValues.Put(PhotoColumn::MEDIA_NAME, "test_update_new.jpg");
    cmd.GetValueBucket() = updateValues;
    string selection = PhotoColumn::MEDIA_ID + " = ?";
    cmd.GetAbsRdbPredicates()->SetWhereClause(selection);
    vector<string> selectionArgs;
    selectionArgs.push_back(to_string(rowId));
    cmd.GetAbsRdbPredicates()->SetWhereArgs(selectionArgs);

    int32_t changedRows = 0;
    ret = rdbStorePtr->Update(cmd, changedRows);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(changedRows, 1);

    // 查询验证策略字段
    resultSet = QueryForTestResult(PhotoColumn::PHOTOS_TABLE, PhotoColumn::MEDIA_ID, rowId, PHOTOS_QUERY_COLUMNS);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    string name = GetStringVal(PhotoColumn::MEDIA_NAME, resultSet);
    int64_t dateModified = GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet);
    int64_t lastVisitTime = GetInt64Val(PhotoColumn::PHOTO_LAST_VISIT_TIME, resultSet);
    resultSet->Close();
    EXPECT_EQ(name, "test_update_new.jpg");
    EXPECT_EQ(dateModified > initialDateModified, true);
    EXPECT_EQ(lastVisitTime > 0, true);
}

/**
 * @tc.name: [基础测试: rdbStore已停止更新] MediaLibraryRdbStore_UpdateCmd_002
 * @tc.desc: 测试Update(MediaLibraryCommand &cmd, int32_t &changedRows)方法在rdbStore停止时更新失败
 *           [1] 创建MediaLibraryCommand并设置更新条件
 *           [2] 调用rdbStorePtr->Stop()停止数据库
 *           [3] 调用Update方法更新数据
 *           [4] 验证更新失败返回E_HAS_DB_ERROR
 */
HWTEST_F(MediaLibraryRdbStoreUpdateTest, MediaLibraryRdbStore_UpdateCmd_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_UpdateCmd_002");
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    cmd.SetTableName(PhotoColumn::PHOTOS_TABLE);
    ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_NAME, "test_update.jpg");
    cmd.GetValueBucket() = values;
    string selection = PhotoColumn::MEDIA_ID + " = ?";
    cmd.GetAbsRdbPredicates()->SetWhereClause(selection);
    vector<string> selectionArgs;
    selectionArgs.push_back("1");
    cmd.GetAbsRdbPredicates()->SetWhereArgs(selectionArgs);

    rdbStorePtr->Stop();
    int32_t changedRows = 0;
    int32_t ret = rdbStorePtr->Update(cmd, changedRows);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    rdbStorePtr->Init();
}

/**
 * @tc.name: [基础测试: 使用whereClause更新数据] MediaLibraryRdbStore_Update_001
 * @tc.desc: 测试Update方法使用whereClause更新数据（不经过策略，时间字段不更新）
 *           [1] 先插入一条数据
 *           [2] 调用Update方法使用whereClause更新数据
 *           [3] 验证更新成功且数据已被更新
 *           [4] 验证PHOTO_META_DATE_MODIFIED、PHOTO_LAST_VISIT_TIME未变化
 */
HWTEST_F(MediaLibraryRdbStoreUpdateTest, MediaLibraryRdbStore_Update_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Update_001");
    // 先插入一条数据
    ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_NAME, "test_update_where.jpg");
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 获取初始时间戳
    auto resultSet = QueryForTestResult(PhotoColumn::PHOTOS_TABLE, PhotoColumn::MEDIA_ID, rowId, PHOTOS_QUERY_COLUMNS);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), E_OK);
    int64_t initialDateModified = GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet);
    int64_t initialLastVisitTime = GetInt64Val(PhotoColumn::PHOTO_LAST_VISIT_TIME, resultSet);
    resultSet->Close();

    // 使用whereClause更新数据
    ValuesBucket updateValues;
    updateValues.Put(PhotoColumn::MEDIA_NAME, "test_update_where_new.jpg");
    string whereClause = PhotoColumn::MEDIA_ID + " = ?";
    vector<string> args = { to_string(rowId) };
    int changedRows = 0;
    ret = rdbStorePtr->Update(changedRows, PhotoColumn::PHOTOS_TABLE, updateValues, whereClause, args);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(changedRows, 1);

    // 查询验证数据已被更新且时间未变化
    resultSet = QueryForTestResult(PhotoColumn::PHOTOS_TABLE, PhotoColumn::MEDIA_ID, rowId, PHOTOS_QUERY_COLUMNS);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), E_OK);
    string name = GetStringVal(PhotoColumn::MEDIA_NAME, resultSet);
    int64_t dateModified = GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet);
    int64_t lastVisitTime = GetInt64Val(PhotoColumn::PHOTO_LAST_VISIT_TIME, resultSet);
    resultSet->Close();
    EXPECT_EQ(name, "test_update_where_new.jpg");
    EXPECT_EQ(dateModified, initialDateModified);
    EXPECT_EQ(lastVisitTime, initialLastVisitTime);
}

/**
 * @tc.name: [基础测试: 使用AbsRdbPredicates更新数据] MediaLibraryRdbStore_Update_002
 * @tc.desc: 测试Update方法使用AbsRdbPredicates更新数据（不经过策略，时间字段不更新）
 *           [1] 先插入一条数据
 *           [2] 创建AbsRdbPredicates并设置更新条件
 *           [3] 调用Update方法更新数据
 *           [4] 验证更新成功且数据已被更新
 *           [5] 验证PHOTO_META_DATE_MODIFIED、PHOTO_LAST_VISIT_TIME未变化
 */
HWTEST_F(MediaLibraryRdbStoreUpdateTest, MediaLibraryRdbStore_Update_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Update_002");
    // 先插入一条数据
    ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_NAME, "test_update_predicates.jpg");
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 获取初始时间戳
    auto resultSet = QueryForTestResult(PhotoColumn::PHOTOS_TABLE, PhotoColumn::MEDIA_ID, rowId, PHOTOS_QUERY_COLUMNS);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    int64_t initialDateModified = GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet);
    int64_t initialLastVisitTime = GetInt64Val(PhotoColumn::PHOTO_LAST_VISIT_TIME, resultSet);
    resultSet->Close();

    // 使用AbsRdbPredicates更新数据
    ValuesBucket updateValues;
    updateValues.Put(PhotoColumn::MEDIA_NAME, "test_update_predicates_new.jpg");
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, rowId);

    int changedRows = 0;
    ret = rdbStorePtr->Update(changedRows, updateValues, predicates);
    EXPECT_EQ(ret, E_OK);
    EXPECT_GT(changedRows, 0);

    // 查询验证数据已被更新且时间未变化
    resultSet = QueryForTestResult(PhotoColumn::PHOTOS_TABLE, PhotoColumn::MEDIA_ID, rowId, PHOTOS_QUERY_COLUMNS);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    string name = GetStringVal(PhotoColumn::MEDIA_NAME, resultSet);
    int64_t dateModified = GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet);
    int64_t lastVisitTime = GetInt64Val(PhotoColumn::PHOTO_LAST_VISIT_TIME, resultSet);
    resultSet->Close();
    EXPECT_EQ(name, "test_update_predicates_new.jpg");
    EXPECT_EQ(dateModified, initialDateModified);
    EXPECT_EQ(lastVisitTime, initialLastVisitTime);
}

/**
 * @tc.name: [基础测试: 使用UpdateWithReturn更新数据] MediaLibraryRdbStore_Update_003
 * @tc.desc: 测试UpdateWithReturn方法使用AbsRdbPredicates更新数据并返回指定字段
 *           [1] 先插入一条数据
 *           [2] 创建AbsRdbPredicates并设置更新条件
 *           [3] 调用UpdateWithReturn方法更新数据并返回MEDIA_ID
 *           [4] 验证更新成功且返回的MEDIA_ID正确
 */
HWTEST_F(MediaLibraryRdbStoreUpdateTest, MediaLibraryRdbStore_Update_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Update_003");
    // 先插入一条数据
    ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_NAME, "test_update_return.jpg");
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 使用UpdateWithReturn更新数据
    ValuesBucket updateValues;
    updateValues.Put(PhotoColumn::MEDIA_NAME, "test_update_return_new.jpg");
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, rowId);

    auto result = rdbStorePtr->UpdateWithReturn(updateValues, predicates, PhotoColumn::MEDIA_ID);
    EXPECT_EQ(result.first, E_OK);

    // 验证返回的MEDIA_ID正确
    auto resultSet = result.second.results;
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    int64_t returnedId = GetInt64Val(PhotoColumn::MEDIA_ID, resultSet);
    EXPECT_EQ(returnedId, rowId);

    // 查询验证数据已被更新
    auto resultSetPtr = QueryForTestResult(
        PhotoColumn::PHOTOS_TABLE, PhotoColumn::MEDIA_ID, rowId, PHOTOS_QUERY_COLUMNS);
    ASSERT_NE(resultSetPtr, nullptr);
    ASSERT_EQ(resultSetPtr->GoToFirstRow(), E_OK);
    string name = GetStringVal(PhotoColumn::MEDIA_NAME, resultSetPtr);
    resultSetPtr->Close();
    EXPECT_EQ(name, "test_update_return_new.jpg");
}

/**
 * @tc.name: [策略测试: Photos表更新策略] MediaLibraryRdbStore_Update_PhotoTable_001
 * @tc.desc: 测试MediaLibraryRdbStore::UpdateWithDateTime正确设置时间戳
 *           [1] 插入Photos表数据
 *           [2] 调用UpdateWithDateTime方法更新数据
 *           [3] 验证PHOTO_META_DATE_MODIFIED、PHOTO_LAST_VISIT_TIME被更新
 */
HWTEST_F(MediaLibraryRdbStoreUpdateTest, MediaLibraryRdbStore_Update_PhotoTable_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Update_PhotoTable_001");
    // 插入Photos表数据
    ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_NAME, "test_update_strategy.jpg");
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 获取初始时间戳
    auto resultSet = QueryForTestResult(PhotoColumn::PHOTOS_TABLE, PhotoColumn::MEDIA_ID, rowId, PHOTOS_QUERY_COLUMNS);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    int64_t initialDateModified = GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet);
    int64_t initialLastVisitTime = GetInt64Val(PhotoColumn::PHOTO_LAST_VISIT_TIME, resultSet);
    resultSet->Close();
    EXPECT_EQ(initialLastVisitTime, 0);

    // 使用UpdateWithDateTime更新数据
    ValuesBucket updateValues;
    updateValues.Put(PhotoColumn::MEDIA_NAME, "test_update_strategy_new.jpg");
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, rowId);
    ret = MediaLibraryRdbStore::UpdateWithDateTime(updateValues, predicates);
    EXPECT_EQ(ret, 1);

    // 查询验证策略字段
    resultSet = QueryForTestResult(PhotoColumn::PHOTOS_TABLE, PhotoColumn::MEDIA_ID, rowId, PHOTOS_QUERY_COLUMNS);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    string name = GetStringVal(PhotoColumn::MEDIA_NAME, resultSet);
    int64_t dateModified = GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet);
    int64_t lastVisitTime = GetInt64Val(PhotoColumn::PHOTO_LAST_VISIT_TIME, resultSet);
    resultSet->Close();
    EXPECT_EQ(name, "test_update_strategy_new.jpg");
    EXPECT_GT(dateModified, initialDateModified);
    EXPECT_GT(lastVisitTime, 0);
}

/**
 * @tc.name: [策略测试: 无策略表更新] MediaLibraryRdbStore_Update_NoStrategyTable_001
 * @tc.desc: 测试MediaLibraryRdbStore::UpdateWithDateTime在无策略表上的更新操作
 *           [1] 创建测试表但不注册策略
 *           [2] 插入测试数据
 *           [3] 调用UpdateWithDateTime方法更新数据
 *           [4] 验证更新成功且数据已被更新
 */
HWTEST_F(MediaLibraryRdbStoreUpdateTest, MediaLibraryRdbStore_Update_NoStrategyTable_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Update_NoStrategyTable_001");
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

    // 使用UpdateWithDateTime更新数据 - 没有注册策略，应该直接更新
    ValuesBucket updateValues;
    updateValues.Put("name", "test_no_strategy_updated");
    updateValues.Put("value", 888);
    AbsRdbPredicates predicates("test_no_strategy_table");
    predicates.EqualTo("id", rowId);
    ret = MediaLibraryRdbStore::UpdateWithDateTime(updateValues, predicates);
    EXPECT_EQ(ret, 1);

    // 查询验证数据已被更新
    auto resultSet = QueryForTestResult("test_no_strategy_table", "id", rowId, {"name", "value"});
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    string name = GetStringVal("name", resultSet);
    int value = GetInt32Val("value", resultSet);
    resultSet->Close();
    EXPECT_EQ(name, "test_no_strategy_updated");
    EXPECT_EQ(value, 888);

    // 清理测试表
    string dropTestTableSql = "DROP TABLE IF EXISTS test_no_strategy_table";
    ret = rdbStorePtr->ExecuteSql(dropTestTableSql);
    EXPECT_EQ(ret, NativeRdb::E_OK);
}

} // namespace Media
} // namespace OHOS