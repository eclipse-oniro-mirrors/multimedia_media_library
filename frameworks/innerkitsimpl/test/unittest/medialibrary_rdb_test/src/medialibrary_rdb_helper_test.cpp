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
#define MLOG_TAG "MediaLibraryRdbHelperTest"

#include "medialibrary_rdb_helper_test.h"

#include <chrono>
#include <cstdint>
#include <thread>
#include <sstream>
#include <variant>

#include "ability_context_impl.h"
#include "context.h"
#include "js_runtime.h"
#include "media_log.h"
#include "media_upgrade.h"
#include "medialibrary_rdb_helper.h"
#include "medialibrary_rdbstore.h"
#include "result_set_utils.h"
#include "shooting_mode_column.h"
#include "vision_column.h"
#include "vision_db_sqls.h"
#include "vision_db_sqls_more.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace NativeRdb;

namespace OHOS {
namespace Media {

static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
static constexpr int32_t SQLITE_LOCKED_MAX_RETRIES = 30;
static constexpr int32_t EXPECTED_RETRY_COUNT_ONCE = 1;
static constexpr int32_t EXPECTED_RETRY_COUNT_TWICE = 2;
static constexpr int32_t EXPECTED_RETRY_COUNT_THRICE = 3;
static shared_ptr<MediaLibraryRdbStore> rdbStorePtr = nullptr;

/**
 * 从ExecuteSqlWithReturn返回值中获取int64类型的值
 */
static vector<int64_t> GetInt64FromResults(const pair<int32_t, Results> &retWithResults, const string &columnName)
{
    vector<int64_t> values;
    auto resultSet = retWithResults.second.results;
    if (retWithResults.first != E_OK) {
        MEDIA_ERR_LOG("ret err: %{public}d", retWithResults.first);
        return values;
    }

    int32_t count = 0;
    auto ret = resultSet->GetRowCount(count);
    if (ret != E_OK || count <= 0) {
        MEDIA_ERR_LOG("Failed to get resultset row count, ret[%{public}d], count[%{public}d]", ret, count);
        return values;
    }

    do {
        int64_t value = get<int64_t>(ResultSetUtils::GetValFromColumn(columnName, resultSet, TYPE_INT64));
        values.push_back(value);
    } while (resultSet->GoToNextRow() == E_OK);
    return values;
}

static void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        ANALYSIS_ALBUM_TABLE,
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
        CREATE_ANALYSIS_ALBUM_FOR_ONCREATE,
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

void MediaLibraryRdbHelperTest::SetUpTestCase(void)
{
    auto stageContext = std::make_shared<AbilityRuntime::ContextImpl>();
    auto abilityContextImpl = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    abilityContextImpl->SetStageContext(stageContext);
    rdbStorePtr = std::make_shared<MediaLibraryRdbStore>(abilityContextImpl);
    int32_t ret = rdbStorePtr->Init();
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MediaLibraryRdbHelperTest rdbstore start ret = %{public}d", ret);
}

void MediaLibraryRdbHelperTest::TearDownTestCase(void)
{
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void MediaLibraryRdbHelperTest::SetUp()
{
    CleanTestTables();
    SetTables();
}

void MediaLibraryRdbHelperTest::TearDown(void) {}

/**
 * @tc.name: [正常场景: 执行sql成功] MediaLibraryRdbHelper_ExecSqlWithRetry_001
 * @tc.desc: 测试ExecSqlWithRetry在SQL执行成功的情况
 *           [1] 执行SQL返回E_OK
 *           [2] 验证返回值为E_OK
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_ExecSqlWithRetry_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_ExecSqlWithRetry_001");

    ASSERT_NE(rdbStorePtr, nullptr);
    // 使用RETURNING子句直接获取file_id
    string insertSql = "INSERT INTO " + string(PhotoColumn::PHOTOS_TABLE) + " (" +
        PhotoColumn::MEDIA_FILE_PATH + ", " + PhotoColumn::MEDIA_NAME + ", " +
        PhotoColumn::MEDIA_MIME_TYPE + ") VALUES (?, ?, ?) ";
    auto [ret, results] = rdbStorePtr->ExecuteSqlWithReturn(insertSql,
        {"/data/test/photo.jpg", "test_photo.jpg", "image/jpeg"}, PhotoColumn::MEDIA_ID);
    EXPECT_EQ(ret, E_OK);

    // 使用公共方法获取file_id
    vector<int64_t> fileIds = GetInt64FromResults({ret, results}, PhotoColumn::MEDIA_ID);
    EXPECT_GT(fileIds.size(), 0);
    EXPECT_GT(fileIds[0], 0);
    MEDIA_INFO_LOG("inserted file_id: %{public}" PRId64, fileIds[0]);
}

/**
 * @tc.name: [异常场景: SQL执行失败] MediaLibraryRdbHelper_ExecSqlWithRetry_002
 * @tc.desc: 测试ExecSqlWithRetry在SQL执行失败的情况
 *           [1] 执行SQL返回非E_OK错误
 *           [2] 验证返回错误码
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_ExecSqlWithRetry_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_ExecSqlWithRetry_002");

    ASSERT_NE(rdbStorePtr, nullptr);
    string invalidSql = "INSERT INTO invalid_table (name) VALUES (?)";
    int32_t ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return rdbStorePtr->ExecuteSql(invalidSql, {"test"});
    });
    EXPECT_NE(ret, E_OK);
}

/**
 * @tc.name: [异常场景: E_SQLITE_LOCKED错误] MediaLibraryRdbHelper_ExecSqlWithRetry_003
 * @tc.desc: 测试ExecSqlWithRetry在遇到E_SQLITE_LOCKED错误时的重试逻辑
 *           [1] 模拟E_SQLITE_LOCKED错误
 *           [2] 验证重试机制
 *           [3] 验证最终返回错误码
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_ExecSqlWithRetry_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_ExecSqlWithRetry_003");

    int32_t retryCount = 0;
    int32_t maxRetries = EXPECTED_RETRY_COUNT_THRICE;
    int32_t ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        retryCount++;
        if (retryCount < maxRetries) {
            return NativeRdb::E_SQLITE_LOCKED;
        }
        return E_OK;
    });
    EXPECT_EQ(retryCount, maxRetries);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: [异常场景: E_SQLITE_LOCKED超过最大重试次数] MediaLibraryRdbHelper_ExecSqlWithRetry_004
 * @tc.desc: 测试ExecSqlWithRetry在E_SQLITE_LOCKED超过最大重试次数的情况
 *           [1] 模拟持续E_SQLITE_LOCKED错误
 *           [2] 验证达到最大重试次数后返回错误
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_ExecSqlWithRetry_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_ExecSqlWithRetry_004");

    int32_t retryCount = 0;
    int32_t ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        retryCount++;
        return NativeRdb::E_SQLITE_LOCKED;
    });
    EXPECT_GT(retryCount, SQLITE_LOCKED_MAX_RETRIES);
    EXPECT_EQ(ret, NativeRdb::E_SQLITE_LOCKED);
}

/**
 * @tc.name: [异常场景: E_SQLITE_BUSY错误] MediaLibraryRdbHelper_ExecSqlWithRetry_005
 * @tc.desc: 测试ExecSqlWithRetry在遇到E_SQLITE_BUSY错误时的重试逻辑
 *           [1] 模拟E_SQLITE_BUSY错误
 *           [2] 验证重试机制
 *           [3] 验证最终返回错误码
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_ExecSqlWithRetry_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_ExecSqlWithRetry_005");

    int32_t retryCount = 0;
    int32_t maxRetries = EXPECTED_RETRY_COUNT_TWICE;
    int32_t ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        retryCount++;
        if (retryCount < maxRetries) {
            return NativeRdb::E_SQLITE_BUSY;
        }
        return E_OK;
    });
    EXPECT_EQ(retryCount, maxRetries);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: [异常场景: E_SQLITE_BUSY超过最大重试次数] MediaLibraryRdbHelper_ExecSqlWithRetry_006
 * @tc.desc: 测试ExecSqlWithRetry在E_SQLITE_BUSY超过最大重试次数的情况
 *           [1] 模拟持续E_SQLITE_BUSY错误
 *           [2] 验证达到最大重试次数后返回错误
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_ExecSqlWithRetry_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_ExecSqlWithRetry_006");

    int32_t retryCount = 0;
    int32_t ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        retryCount++;
        return NativeRdb::E_SQLITE_BUSY;
    });
    EXPECT_EQ(retryCount, EXPECTED_RETRY_COUNT_TWICE);
    EXPECT_EQ(ret, NativeRdb::E_SQLITE_BUSY);
}

/**
 * @tc.name: [异常场景: E_DATABASE_BUSY错误] MediaLibraryRdbHelper_ExecSqlWithRetry_007
 * @tc.desc: 测试ExecSqlWithRetry在遇到E_DATABASE_BUSY错误时的重试逻辑
 *           [1] 模拟E_DATABASE_BUSY错误
 *           [2] 验证重试机制
 *           [3] 验证最终返回错误码
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_ExecSqlWithRetry_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_ExecSqlWithRetry_007");

    int32_t retryCount = 0;
    int32_t maxRetries = EXPECTED_RETRY_COUNT_TWICE;
    int32_t ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        retryCount++;
        if (retryCount < maxRetries) {
            return NativeRdb::E_DATABASE_BUSY;
        }
        return E_OK;
    });
    EXPECT_EQ(retryCount, maxRetries);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: [异常场景: E_DATABASE_BUSY超过最大重试次数] MediaLibraryRdbHelper_ExecSqlWithRetry_008
 * @tc.desc: 测试ExecSqlWithRetry在E_DATABASE_BUSY超过最大重试次数的情况
 *           [1] 模拟持续E_DATABASE_BUSY错误
 *           [2] 验证达到最大重试次数后返回错误
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_ExecSqlWithRetry_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_ExecSqlWithRetry_008");

    int32_t retryCount = 0;
    int32_t ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        retryCount++;
        return NativeRdb::E_DATABASE_BUSY;
    });
    EXPECT_EQ(retryCount, EXPECTED_RETRY_COUNT_TWICE);
    EXPECT_EQ(ret, NativeRdb::E_DATABASE_BUSY);
}

/**
 * @tc.name: [异常场景: 其他错误不重试] MediaLibraryRdbHelper_ExecSqlWithRetry_009
 * @tc.desc: 测试ExecSqlWithRetry在遇到其他错误时不重试
 *           [1] 模拟其他错误（如E_ERROR）
 *           [2] 验证不进行重试
 *           [3] 验证直接返回错误
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_ExecSqlWithRetry_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_ExecSqlWithRetry_009");

    int32_t retryCount = 0;
    int32_t ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        retryCount++;
        return NativeRdb::E_ERROR;
    });
    EXPECT_EQ(retryCount, EXPECTED_RETRY_COUNT_ONCE);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
}

/**
 * @tc.name: [混合场景: 先失败后成功] MediaLibraryRdbHelper_ExecSqlWithRetry_010
 * @tc.desc: 测试ExecSqlWithRetry在先失败后成功的情况
 *           [1] 模拟先E_SQLITE_LOCKED后E_OK
 *           [2] 验证重试后成功
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_ExecSqlWithRetry_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_ExecSqlWithRetry_010");

    int32_t retryCount = 0;
    int32_t ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        retryCount++;
        if (retryCount == EXPECTED_RETRY_COUNT_ONCE) {
            return NativeRdb::E_SQLITE_LOCKED;
        }
        return E_OK;
    });
    EXPECT_EQ(retryCount, EXPECTED_RETRY_COUNT_TWICE);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: [混合场景: 多种错误类型] MediaLibraryRdbHelper_ExecSqlWithRetry_011
 * @tc.desc: 测试ExecSqlWithRetry在遇到多种错误类型的情况
 *           [1] 模拟先E_SQLITE_LOCKED后E_SQLITE_BUSY
 *           [2] 验证最终返回错误
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_ExecSqlWithRetry_011, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_ExecSqlWithRetry_011");

    int32_t retryCount = 0;
    int32_t ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        retryCount++;
        if (retryCount == EXPECTED_RETRY_COUNT_ONCE) {
            return NativeRdb::E_SQLITE_LOCKED;
        } else if (retryCount == EXPECTED_RETRY_COUNT_TWICE) {
            return NativeRdb::E_SQLITE_BUSY;
        }
        return E_OK;
    });
    EXPECT_GT(retryCount, EXPECTED_RETRY_COUNT_ONCE);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: [边界场景: 空SQL函数] MediaLibraryRdbHelper_ExecSqlWithRetry_012
 * @tc.desc: 测试ExecSqlWithRetry处理空函数的情况
 *           [1] 传入空函数
 *           [2] 验证返回错误
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_ExecSqlWithRetry_012, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_ExecSqlWithRetry_012");

    std::function<int32_t()> emptyFunc = nullptr;
    int32_t ret = MediaLibraryRdbHelper::ExecSqlWithRetry(emptyFunc);
    EXPECT_NE(ret, E_OK);
}

/**
 * @tc.name: [边界场景: 空ValuesBucket] MediaLibraryRdbHelper_BuildValuesSql_001
 * @tc.desc: 测试BuildValuesSql处理空ValuesBucket的情况
 *           [1] 传入空的ValuesBucket
 *           [2] 验证生成的SQL格式正确
 *           [3] 验证bindArgs为空
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_BuildValuesSql_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_BuildValuesSql_001");

    string sql = "INSERT INTO test_table ";
    vector<ValueObject> bindArgs;
    ValuesBucket emptyValues;

    MediaLibraryRdbHelper::BuildValuesSql(emptyValues, bindArgs, sql);

    EXPECT_EQ(sql, "INSERT INTO test_table () select  ");
    EXPECT_EQ(bindArgs.size(), 0);
}

/**
 * @tc.name: [正常场景: 单个元素] MediaLibraryRdbHelper_BuildValuesSql_002
 * @tc.desc: 测试BuildValuesSql处理单个元素的情况
 *           [1] 传入包含一个元素的ValuesBucket
 *           [2] 验证生成的SQL格式正确
 *           [3] 验证bindArgs包含一个元素
 *           [4] 验证bindArgs中的值正确
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_BuildValuesSql_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_BuildValuesSql_002");

    string sql = "INSERT INTO test_table ";
    vector<ValueObject> bindArgs;
    ValuesBucket singleValues;
    singleValues.PutString("name", "test");

    MediaLibraryRdbHelper::BuildValuesSql(singleValues, bindArgs, sql);

    EXPECT_EQ(sql, "INSERT INTO test_table (name) select ? ");
    EXPECT_EQ(bindArgs.size(), 1);
    string value;
    bindArgs[0].GetString(value);
    EXPECT_EQ(value, "test");
}

/**
 * @tc.name: [边界场景: 空列和空条件] MediaLibraryRdbHelper_BuildQuerySql_001
 * @tc.desc: 测试BuildQuerySql处理空列和空条件的情况
 *           [1] 传入空的columns和predicates
 *           [2] 验证生成的SQL格式正确
 *           [3] 验证bindArgs为空
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_BuildQuerySql_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_BuildQuerySql_001");

    string sql = "";
    vector<ValueObject> bindArgs;
    vector<string> columns;
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    MediaLibraryRdbHelper::BuildQuerySql(predicates, columns, bindArgs, sql);

    EXPECT_EQ(sql, "SELECT * FROM " + PhotoColumn::PHOTOS_TABLE);
    EXPECT_EQ(bindArgs.size(), 0);
}

/**
 * @tc.name: [正常场景: 单列查询] MediaLibraryRdbHelper_BuildQuerySql_002
 * @tc.desc: 测试BuildQuerySql处理单列查询的情况
 *           [1] 传入单个column
 *           [2] 验证生成的SQL格式正确
 *           [3] 验证bindArgs为空
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_BuildQuerySql_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_BuildQuerySql_002");

    string sql = "";
    vector<ValueObject> bindArgs;
    vector<string> columns = {PhotoColumn::MEDIA_NAME};
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    MediaLibraryRdbHelper::BuildQuerySql(predicates, columns, bindArgs, sql);

    EXPECT_EQ(sql, "SELECT " + PhotoColumn::MEDIA_NAME + "  FROM " + PhotoColumn::PHOTOS_TABLE);
    EXPECT_EQ(bindArgs.size(), 0);
}

/**
 * @tc.name: [正常场景: 多列查询] MediaLibraryRdbHelper_BuildQuerySql_003
 * @tc.desc: 测试BuildQuerySql处理多列查询的情况
 *           [1] 传入多个columns
 *           [2] 验证生成的SQL格式正确
 *           [3] 验证bindArgs为空
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_BuildQuerySql_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_BuildQuerySql_003");

    string sql = "";
    vector<ValueObject> bindArgs;
    vector<string> columns = {PhotoColumn::MEDIA_NAME, PhotoColumn::MEDIA_ID, PhotoColumn::MEDIA_FILE_PATH};
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    MediaLibraryRdbHelper::BuildQuerySql(predicates, columns, bindArgs, sql);

    EXPECT_EQ(sql, "SELECT " + PhotoColumn::MEDIA_NAME + ", " + PhotoColumn::MEDIA_ID + ", " +
        PhotoColumn::MEDIA_FILE_PATH + "  FROM " + PhotoColumn::PHOTOS_TABLE);
    EXPECT_EQ(bindArgs.size(), 0);
}

/**
 * @tc.name: [正常场景: 带WHERE条件的查询] MediaLibraryRdbHelper_BuildQuerySql_004
 * @tc.desc: 测试BuildQuerySql处理带WHERE条件查询的情况
 *           [1] 传入带WHERE条件的predicates
 *           [2] 验证生成的SQL格式正确
 *           [3] 验证bindArgs包含WHERE参数
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_BuildQuerySql_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_BuildQuerySql_004");

    string sql = "";
    vector<ValueObject> bindArgs;
    vector<string> columns = {PhotoColumn::MEDIA_NAME};
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, "123");

    MediaLibraryRdbHelper::BuildQuerySql(predicates, columns, bindArgs, sql);

    EXPECT_EQ(sql, "SELECT " + PhotoColumn::MEDIA_NAME + "  FROM " + PhotoColumn::PHOTOS_TABLE +
        " WHERE " + PhotoColumn::MEDIA_ID + " = ? ");
    EXPECT_EQ(bindArgs.size(), 1);
    string value;
    bindArgs[0].GetString(value);
    EXPECT_EQ(value, "123");
}

/**
 * @tc.name: [正常场景: 多个WHERE条件] MediaLibraryRdbHelper_BuildQuerySql_005
 * @tc.desc: 测试BuildQuerySql处理多个WHERE条件查询的情况
 *           [1] 传入带多个WHERE条件的predicates
 *           [2] 验证生成的SQL格式正确
 *           [3] 验证bindArgs包含所有WHERE参数
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_BuildQuerySql_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_BuildQuerySql_005");

    string sql = "";
    vector<ValueObject> bindArgs;
    vector<string> columns = {PhotoColumn::MEDIA_NAME, PhotoColumn::MEDIA_ID};
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, "123");
    predicates.And();
    predicates.EqualTo(PhotoColumn::MEDIA_NAME, "test.jpg");

    MediaLibraryRdbHelper::BuildQuerySql(predicates, columns, bindArgs, sql);

    EXPECT_EQ(sql, "SELECT " + PhotoColumn::MEDIA_NAME + ", " + PhotoColumn::MEDIA_ID +
        "  FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " + PhotoColumn::MEDIA_ID + " = ?  AND " +
        PhotoColumn::MEDIA_NAME + " = ? ");
    EXPECT_EQ(bindArgs.size(), 2);
    string value1, value2;
    bindArgs[0].GetString(value1);
    bindArgs[1].GetString(value2);
    EXPECT_EQ(value1, "123");
    EXPECT_EQ(value2, "test.jpg");
}

/**
 * @tc.name: [正常场景: 带LIKE条件的查询] MediaLibraryRdbHelper_BuildQuerySql_006
 * @tc.desc: 测试BuildQuerySql处理带LIKE条件查询的情况
 *           [1] 传入带LIKE条件的predicates
 *           [2] 验证生成的SQL格式正确
 *           [3] 验证bindArgs包含LIKE参数
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_BuildQuerySql_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_BuildQuerySql_006");

    string sql = "";
    vector<ValueObject> bindArgs;
    vector<string> columns = {PhotoColumn::MEDIA_NAME};
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.Like(PhotoColumn::MEDIA_NAME, "%test%");

    MediaLibraryRdbHelper::BuildQuerySql(predicates, columns, bindArgs, sql);

    EXPECT_EQ(sql, "SELECT " + PhotoColumn::MEDIA_NAME + "  FROM " + PhotoColumn::PHOTOS_TABLE +
        " WHERE " + PhotoColumn::MEDIA_NAME + " LIKE ? ");
    EXPECT_EQ(bindArgs.size(), 1);
    string value;
    bindArgs[0].GetString(value);
    EXPECT_EQ(value, "%test%");
}

/**
 * @tc.name: [正常场景: 带IN条件的查询] MediaLibraryRdbHelper_BuildQuerySql_007
 * @tc.desc: 测试BuildQuerySql处理带IN条件查询的情况
 *           [1] 传入带IN条件的predicates
 *           [2] 验证生成的SQL格式正确
 *           [3] 验证bindArgs包含IN参数
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_BuildQuerySql_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_BuildQuerySql_007");

    string sql = "";
    vector<ValueObject> bindArgs;
    vector<string> columns = {PhotoColumn::MEDIA_NAME};
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    std::vector<std::string> fileIds = {"123", "456", "789"};
    predicates.In(PhotoColumn::MEDIA_ID, fileIds);

    MediaLibraryRdbHelper::BuildQuerySql(predicates, columns, bindArgs, sql);

    EXPECT_EQ(sql, "SELECT " + PhotoColumn::MEDIA_NAME + "  FROM " + PhotoColumn::PHOTOS_TABLE +
        " WHERE " + PhotoColumn::MEDIA_ID + " IN (? , ? , ?)");
    EXPECT_EQ(bindArgs.size(), 3);
    string value1, value2, value3;
    bindArgs[0].GetString(value1);
    bindArgs[1].GetString(value2);
    bindArgs[2].GetString(value3);
    EXPECT_EQ(value1, "123");
    EXPECT_EQ(value2, "456");
    EXPECT_EQ(value3, "789");
}

/**
 * @tc.name: [正常场景: 带ORDER BY的查询] MediaLibraryRdbHelper_BuildQuerySql_008
 * @tc.desc: 测试BuildQuerySql处理带ORDER BY查询的情况
 *           [1] 传入带ORDER BY的predicates
 *           [2] 验证生成的SQL格式正确
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_BuildQuerySql_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_BuildQuerySql_008");

    string sql = "";
    vector<ValueObject> bindArgs;
    vector<string> columns = {PhotoColumn::MEDIA_NAME};
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.OrderByDesc(PhotoColumn::MEDIA_DATE_MODIFIED);

    MediaLibraryRdbHelper::BuildQuerySql(predicates, columns, bindArgs, sql);

    EXPECT_EQ(sql, "SELECT " + PhotoColumn::MEDIA_NAME + "  FROM " + PhotoColumn::PHOTOS_TABLE +
        " ORDER BY " + PhotoColumn::MEDIA_DATE_MODIFIED + " DESC ");
    EXPECT_EQ(bindArgs.size(), 0);
}

/**
 * @tc.name: [正常场景: 带LIMIT的查询] MediaLibraryRdbHelper_BuildQuerySql_009
 * @tc.desc: 测试BuildQuerySql处理带LIMIT查询的情况
 *           [1] 传入带LIMIT的predicates
 *           [2] 验证生成的SQL格式正确
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_BuildQuerySql_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_BuildQuerySql_009");

    string sql = "";
    vector<ValueObject> bindArgs;
    vector<string> columns = {PhotoColumn::MEDIA_NAME};
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.Limit(10);

    MediaLibraryRdbHelper::BuildQuerySql(predicates, columns, bindArgs, sql);

    EXPECT_EQ(sql, "SELECT " + PhotoColumn::MEDIA_NAME + "  FROM " + PhotoColumn::PHOTOS_TABLE +
        " LIMIT 10");
    EXPECT_EQ(bindArgs.size(), 0);
}

/**
 * @tc.name: [正常场景: 复杂查询条件] MediaLibraryRdbHelper_BuildQuerySql_010
 * @tc.desc: 测试BuildQuerySql处理复杂查询条件的情况
 *           [1] 传入包含WHERE、ORDER BY、LIMIT的predicates
 *           [2] 验证生成的SQL格式正确
 *           [3] 验证bindArgs包含WHERE参数
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_BuildQuerySql_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_BuildQuerySql_010");

    string sql = "";
    vector<ValueObject> bindArgs;
    vector<string> columns = {PhotoColumn::MEDIA_NAME, PhotoColumn::MEDIA_ID};
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_TYPE, "1");
    predicates.And();
    predicates.GreaterThan(PhotoColumn::MEDIA_DATE_MODIFIED, "1000000");
    predicates.OrderByDesc(PhotoColumn::MEDIA_DATE_MODIFIED);
    predicates.Limit(20);

    MediaLibraryRdbHelper::BuildQuerySql(predicates, columns, bindArgs, sql);

    EXPECT_EQ(sql, "SELECT " + PhotoColumn::MEDIA_NAME + ", " + PhotoColumn::MEDIA_ID +
        "  FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " + PhotoColumn::MEDIA_TYPE + " = ?  AND " +
        PhotoColumn::MEDIA_DATE_MODIFIED + " > ?  ORDER BY " + PhotoColumn::MEDIA_DATE_MODIFIED +
        " DESC  LIMIT 20");
    EXPECT_EQ(bindArgs.size(), 2);
    string value1, value2;
    bindArgs[0].GetString(value1);
    bindArgs[1].GetString(value2);
    EXPECT_EQ(value1, "1");
    EXPECT_EQ(value2, "1000000");
}

/**
 * @tc.name: [边界场景: 空whereArgs] MediaLibraryRdbHelper_ReplacePredicatesUriToId_001
 * @tc.desc: 测试ReplacePredicatesUriToId处理空whereArgs的情况
 *           [1] 传入空的whereArgs
 *           [2] 验证whereArgs保持为空
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_ReplacePredicatesUriToId_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_ReplacePredicatesUriToId_001");

    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    MediaLibraryRdbHelper::ReplacePredicatesUriToId(predicates);

    EXPECT_EQ(predicates.GetWhereArgs().size(), 0);
}

/**
 * @tc.name: [正常场景: 只有非URI参数] MediaLibraryRdbHelper_ReplacePredicatesUriToId_002
 * @tc.desc: 测试ReplacePredicatesUriToId处理只有非URI参数的情况
 *           [1] 传入只有非URI的whereArgs
 *           [2] 验证参数保持不变
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_ReplacePredicatesUriToId_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_ReplacePredicatesUriToId_002");

    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_NAME, "test.jpg");

    MediaLibraryRdbHelper::ReplacePredicatesUriToId(predicates);

    EXPECT_EQ(predicates.GetWhereArgs().size(), 1);
    EXPECT_EQ(predicates.GetWhereArgs()[0], "test.jpg");
}

/**
 * @tc.name: [正常场景: 只有URI参数] MediaLibraryRdbHelper_ReplacePredicatesUriToId_003
 * @tc.desc: 测试ReplacePredicatesUriToId处理只有URI参数的情况
 *           [1] 传入只有URI的whereArgs
 *           [2] 验证URI被转换为ID
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_ReplacePredicatesUriToId_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_ReplacePredicatesUriToId_003");

    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, "file://media/Photo/123");

    MediaLibraryRdbHelper::ReplacePredicatesUriToId(predicates);

    EXPECT_EQ(predicates.GetWhereArgs().size(), 1);
    EXPECT_EQ(predicates.GetWhereArgs()[0], "123");
}

/**
 * @tc.name: [正常场景: 混合URI和非URI参数] MediaLibraryRdbHelper_ReplacePredicatesUriToId_004
 * @tc.desc: 测试ReplacePredicatesUriToId处理混合参数的情况
 *           [1] 传入混合URI和非URI的whereArgs
 *           [2] 验证URI被转换为ID，非URI保持不变
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_ReplacePredicatesUriToId_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_ReplacePredicatesUriToId_004");

    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, "file://media/Photo/123");
    predicates.And();
    predicates.EqualTo(PhotoColumn::MEDIA_NAME, "test.jpg");

    MediaLibraryRdbHelper::ReplacePredicatesUriToId(predicates);

    EXPECT_EQ(predicates.GetWhereArgs().size(), 2);
    EXPECT_EQ(predicates.GetWhereArgs()[0], "123");
    EXPECT_EQ(predicates.GetWhereArgs()[1], "test.jpg");
}

/**
 * @tc.name: [正常场景: 多个URI参数] MediaLibraryRdbHelper_ReplacePredicatesUriToId_005
 * @tc.desc: 测试ReplacePredicatesUriToId处理多个URI参数的情况
 *           [1] 传入多个URI的whereArgs
 *           [2] 验证所有URI都被转换为ID
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_ReplacePredicatesUriToId_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_ReplacePredicatesUriToId_005");

    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    std::vector<std::string> fileIds = {
        "file://media/Photo/12",
        "file://media/Photo/45",
        "file://media/Photo/78",
    };
    predicates.In(PhotoColumn::MEDIA_ID, fileIds);

    MediaLibraryRdbHelper::ReplacePredicatesUriToId(predicates);

    EXPECT_EQ(predicates.GetWhereArgs().size(), 3);
    EXPECT_EQ(predicates.GetWhereArgs()[0], "12");
    EXPECT_EQ(predicates.GetWhereArgs()[1], "45");
    EXPECT_EQ(predicates.GetWhereArgs()[2], "78");
}

/**
 * @tc.name: [异常场景: URI带额外参数] MediaLibraryRdbHelper_ReplacePredicatesUriToId_006
 * @tc.desc: 测试ReplacePredicatesUriToId处理带额外参数的URI的情况
 *           [1] 传入带额外参数的异常URI
 *           [2] 验证URI被正确转换为ID = -1
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_ReplacePredicatesUriToId_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_ReplacePredicatesUriToId_006");

    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, "file://media/Photo/123?param=value");

    MediaLibraryRdbHelper::ReplacePredicatesUriToId(predicates);

    EXPECT_EQ(predicates.GetWhereArgs().size(), 1);
    EXPECT_EQ(predicates.GetWhereArgs()[0], "-1");
}

/**
 * @tc.name: [边界场景: 空字符串参数] MediaLibraryRdbHelper_ReplacePredicatesUriToId_007
 * @tc.desc: 测试ReplacePredicatesUriToId处理空字符串参数的情况
 *           [1] 传入空字符串参数
 *           [2] 验证空字符串保持不变
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_ReplacePredicatesUriToId_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_ReplacePredicatesUriToId_007");

    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_NAME, "");

    MediaLibraryRdbHelper::ReplacePredicatesUriToId(predicates);

    EXPECT_EQ(predicates.GetWhereArgs().size(), 1);
    EXPECT_EQ(predicates.GetWhereArgs()[0], "");
}

/**
 * @tc.name: [边界场景: 无效URI格式] MediaLibraryRdbHelper_ReplacePredicatesUriToId_008
 * @tc.desc: 测试ReplacePredicatesUriToId处理无效URI格式的情况
 *           [1] 传入无效的URI格式
 *           [2] 验证参数保持不变（不以PHOTO_URI_PREFIX开头）
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_ReplacePredicatesUriToId_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_ReplacePredicatesUriToId_008");

    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, "http://example.com/123");

    MediaLibraryRdbHelper::ReplacePredicatesUriToId(predicates);

    EXPECT_EQ(predicates.GetWhereArgs().size(), 1);
    EXPECT_EQ(predicates.GetWhereArgs()[0], "http://example.com/123");
}

/**
 * @tc.name: [正常场景: 添加不存在的列] MediaLibraryRdbHelper_AddColumnIfNotExists_001
 * @tc.desc: 测试AddColumnIfNotExists添加不存在的列
 *           [1] 创建测试表
 *           [2] 添加不存在的列
 *           [3] 验证列添加成功
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_AddColumnIfNotExists_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_AddColumnIfNotExists_001");

    string testTable = "test_table_add_column";
    string createSql = "CREATE TABLE " + testTable + " (id INTEGER PRIMARY KEY, name TEXT)";
    int32_t ret = rdbStorePtr->ExecuteSql(createSql);
    EXPECT_EQ(ret, E_OK);

    MediaLibraryRdbHelper::AddColumnIfNotExists(*rdbStorePtr->GetRaw(), "age", "INTEGER", testTable);

    EXPECT_TRUE(MediaLibraryRdbHelper::HasColumnInTable(*rdbStorePtr->GetRaw(), "age", testTable));

    string dropSql = "DROP TABLE " + testTable;
    rdbStorePtr->ExecuteSql(dropSql);
}

/**
 * @tc.name: [边界场景: 空表名] MediaLibraryRdbHelper_AddColumnIfNotExists_002
 * @tc.desc: 测试AddColumnIfNotExists处理空表名的情况
 *           [1] 传入空表名
 *           [2] 验证函数行为（预期不会崩溃）
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_AddColumnIfNotExists_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_AddColumnIfNotExists_002");

    MediaLibraryRdbHelper::AddColumnIfNotExists(*rdbStorePtr->GetRaw(), "test_column", "TEXT", "");

    EXPECT_FALSE(MediaLibraryRdbHelper::HasColumnInTable(*rdbStorePtr->GetRaw(), "test_column", ""));
}

/**
 * @tc.name: [边界场景: 空列名] MediaLibraryRdbHelper_AddColumnIfNotExists_003
 * @tc.desc: 测试AddColumnIfNotExists处理空列名的情况
 *           [1] 创建测试表
 *           [2] 传入空列名
 *           [3] 验证函数行为（预期不会崩溃）
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_AddColumnIfNotExists_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_AddColumnIfNotExists_003");

    string testTable = "test_table_empty_column";
    string createSql = "CREATE TABLE " + testTable + " (id INTEGER PRIMARY KEY)";
    int32_t ret = rdbStorePtr->ExecuteSql(createSql);
    EXPECT_EQ(ret, E_OK);

    MediaLibraryRdbHelper::AddColumnIfNotExists(*rdbStorePtr->GetRaw(), "", "TEXT", testTable);

    EXPECT_FALSE(MediaLibraryRdbHelper::HasColumnInTable(*rdbStorePtr->GetRaw(), "", testTable));

    string dropSql = "DROP TABLE " + testTable;
    rdbStorePtr->ExecuteSql(dropSql);
}

/**
 * @tc.name: [正常场景: 创建所有拍摄模式相册] MediaLibraryRdbHelper_PrepareShootingModeAlbum_001
 * @tc.desc: 测试PrepareShootingModeAlbum创建所有拍摄模式相册
 *           [1] 调用PrepareShootingModeAlbum
 *           [2] 验证所有拍摄模式相册被创建
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_PrepareShootingModeAlbum_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_PrepareShootingModeAlbum_001");

    int32_t result = MediaLibraryRdbHelper::PrepareShootingModeAlbum(*rdbStorePtr->GetRaw());
    EXPECT_EQ(result, NativeRdb::E_OK);

    int32_t start = static_cast<int>(ShootingModeAlbumType::START);
    int32_t end = static_cast<int>(ShootingModeAlbumType::END);
    for (int i = start; i <= end; ++i) {
        string albumName = to_string(i);
        string checkSql = "SELECT count(*) FROM " + ANALYSIS_ALBUM_TABLE + " WHERE album_name = '" +
            albumName + "' AND album_subtype = " + to_string(PhotoAlbumSubType::SHOOTING_MODE);
        auto resultSet = rdbStorePtr->QuerySql(checkSql);
        int32_t rowCount = 0;
        resultSet->GetRowCount(rowCount);
        EXPECT_GT(rowCount, 0) << "Album " << albumName << " should exist";
    }
}

/**
 * @tc.name: [边界场景: 表不存在] MediaLibraryRdbHelper_PrepareShootingModeAlbum_002
 * @tc.desc: 测试PrepareShootingModeAlbum处理表不存在的情况
 *           [1] 删除analysis_album表
 *           [2] 调用PrepareShootingModeAlbum
 *           [3] 验证函数返回错误
 */
HWTEST_F(MediaLibraryRdbHelperTest, MediaLibraryRdbHelper_PrepareShootingModeAlbum_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbHelper_PrepareShootingModeAlbum_002");

    string dropSql = "DROP TABLE IF EXISTS " + ANALYSIS_ALBUM_TABLE;
    rdbStorePtr->ExecuteSql(dropSql);

    int32_t result = MediaLibraryRdbHelper::PrepareShootingModeAlbum(*rdbStorePtr->GetRaw());
    EXPECT_NE(result, NativeRdb::E_OK);
}
} // namespace Media
} // namespace OHOS