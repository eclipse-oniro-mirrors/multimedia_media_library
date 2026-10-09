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
#define MLOG_TAG "MediaLibraryRdbStoreInsertTest"

#include "medialibrary_rdbstore_insert_test.h"

#include <chrono>
#include <cstdint>
#include <thread>

#include "ability_context_impl.h"
#include "context.h"
#include "cover_record_columns.h"
#include "js_runtime.h"
#include "media_column.h"
#include "media_log.h"
#include "media_upgrade.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "rdb_table_strategy_manager.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace NativeRdb;

namespace OHOS {
namespace Media {

static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
static constexpr int64_t DATE_ADDED = 1732767140000; // 2024-11-28 02:25:40
static shared_ptr<MediaLibraryRdbStore> rdbStorePtr = nullptr;

// 查询列定义 - Photos表
static const vector<string> PHOTO_QUERY_COLUMNS = {
    PhotoColumn::MEDIA_ID,
    PhotoColumn::MEDIA_NAME,
    PhotoColumn::MEDIA_TYPE,
    MediaColumn::MEDIA_DATE_ADDED,
    PhotoColumn::PHOTO_DATE_ADDED_YEAR,
    PhotoColumn::PHOTO_DATE_ADDED_MONTH,
    PhotoColumn::PHOTO_DATE_ADDED_DAY,
    PhotoColumn::PHOTO_MEDIA_SUFFIX,
};

// 查询列定义 - PhotoAlbum表
static const vector<string> PHOTO_ALBUM_QUERY_COLUMNS = {
    PhotoAlbumColumns::ALBUM_ID,
    PhotoAlbumColumns::ALBUM_TYPE,
    PhotoAlbumColumns::ALBUM_SUBTYPE,
    PhotoAlbumColumns::ALBUM_NAME,
    PhotoAlbumColumns::ALBUM_LPATH,
    PhotoAlbumColumns::COVER_ORDER_KEY,
    PhotoAlbumColumns::COVER_ORDER_SUBKEY,
    PhotoAlbumColumns::COVER_ORDER_TYPE,
    PhotoAlbumColumns::HIDDEN_COVER_ORDER_KEY,
    PhotoAlbumColumns::HIDDEN_COVER_ORDER_SUBKEY,
    PhotoAlbumColumns::HIDDEN_COVER_ORDER_TYPE,
};

// 查询列定义 - Files表
static const vector<string> FILES_QUERY_COLUMNS = {
    CONST_MEDIA_DATA_DB_ID,
    CONST_MEDIA_DATA_DB_FILE_PATH,
    MEDIA_DATA_DB_NAME,
    CONST_MEDIA_DATA_DB_MEDIA_TYPE,
};

// 查询列定义 - PhotoMap表
static const vector<string> PHOTO_MAP_QUERY_COLUMNS = {
    PhotoMap::ALBUM_ID,
    PhotoMap::ASSET_ID,
};

static void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        PhotoAlbumColumns::TABLE,
        CoverRecordColumns::COVER_RECORD_TABLE,
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
        CoverRecordColumns::CREATE_COVER_RECORD_TABLE,
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

void MediaLibraryRdbStoreInsertTest::SetUpTestCase(void)
{
    auto stageContext = std::make_shared<AbilityRuntime::ContextImpl>();
    auto abilityContextImpl = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    abilityContextImpl->SetStageContext(stageContext);
    rdbStorePtr = std::make_shared<MediaLibraryRdbStore>(abilityContextImpl);
    int32_t ret = rdbStorePtr->Init();
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MediaLibraryRdbStoreInsertTest rdbstore start ret = %{public}d", ret);
}

void MediaLibraryRdbStoreInsertTest::TearDownTestCase(void)
{
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void MediaLibraryRdbStoreInsertTest::SetUp()
{
    CleanTestTables();
    SetTables();
}

void MediaLibraryRdbStoreInsertTest::TearDown() {}

/**
 * 通用查询测试结果 - 根据rowId查询指定表的信息
 * @param tableName 表名
 * @param idColumn ID列名
 * @param rowId 行ID
 * @param columns 查询的列
 */
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

/**
 * 查询照片测试结果 - 根据rowId查询照片信息
 */
static shared_ptr<NativeRdb::ResultSet> QueryForTestResult(int64_t rowId)
{
    return QueryForTestResult(PhotoColumn::PHOTOS_TABLE, PhotoColumn::MEDIA_ID, rowId, PHOTO_QUERY_COLUMNS);
}

/**
 * @tc.name: [基础测试: 使用MediaLibraryCommand插入数据] MediaLibraryRdbStore_InsertCmd_001
 * @tc.desc: 测试Insert(MediaLibraryCommand &cmd, int64_t &rowId)方法正常插入数据
 *           [1] 创建MediaLibraryCommand并设置表名和ValuesBucket
 *           [2] 调用Insert方法插入数据
 *           [3] 验证插入成功并返回正确的rowId
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_InsertCmd_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_InsertCmd_001");
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE);
    cmd.SetTableName(PhotoColumn::PHOTOS_TABLE);
    ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_NAME, "test_cmd.jpg");
    cmd.SetValueBucket(values);
    int64_t rowId = -1;
    int32_t ret = rdbStorePtr->Insert(cmd, rowId);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    auto resultSet = QueryForTestResult(rowId);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    string mediaName = GetStringVal(PhotoColumn::MEDIA_NAME, resultSet);
    resultSet->Close();
    EXPECT_EQ(mediaName, "test_cmd.jpg");
}

/**
 * @tc.name: [基础测试: 插入最小数据] MediaLibraryRdbStore_InsertCmd_002
 * @tc.desc: 测试Insert(MediaLibraryCommand &cmd, int64_t &rowId)方法插入最小数据
 *           [1] 创建MediaLibraryCommand并设置表名和ValuesBucket（只设置MEDIA_NAME）
 *           [2] 调用Insert方法插入数据
 *           [3] 验证插入成功
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_InsertCmd_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_InsertCmd_002");
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE);
    cmd.SetTableName(PhotoColumn::PHOTOS_TABLE);
    ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_NAME, "test_cmd_002.jpg");
    cmd.SetValueBucket(values);
    int64_t rowId = -1;
    int32_t ret = rdbStorePtr->Insert(cmd, rowId);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);
}

/**
 * @tc.name: [异常场景: rdbStore已停止] MediaLibraryRdbStore_InsertCmd_003
 * @tc.desc: 测试Insert(MediaLibraryCommand &cmd, int64_t &rowId)方法在rdbStore停止时插入失败
 *           [1] 创建MediaLibraryCommand并设置表名和ValuesBucket
 *           [2] 调用rdbStorePtr->Stop()停止数据库
 *           [3] 调用Insert方法插入数据
 *           [4] 验证插入失败返回E_HAS_DB_ERROR
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_InsertCmd_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_InsertCmd_003");
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE);
    cmd.SetTableName(PhotoColumn::PHOTOS_TABLE);
    ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_NAME, "test_cmd_004.jpg");
    cmd.SetValueBucket(values);
    rdbStorePtr->Stop();
    int64_t rowId = -1;
    int32_t ret = rdbStorePtr->Insert(cmd, rowId);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    rdbStorePtr->Init();
}

/**
 * @tc.name: [Photo表策略测试: 插入照片数据] MediaLibraryRdbStore_Insert_PhotoTable_001
 * @tc.desc: 测试Insert静态方法正常插入照片数据
 *           [1] 创建ValuesBucket并设置MEDIA_NAME
 *           [2] 调用Insert方法插入数据到PHOTOS_TABLE
 *           [3] 查询验证PHOTO_MEDIA_SUFFIX为jpg
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_Insert_PhotoTable_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Insert_PhotoTable_001");
    ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_NAME, "test.jpg");
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    auto resultSet = QueryForTestResult(rowId);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    string mediaName = GetStringVal(PhotoColumn::MEDIA_NAME, resultSet);
    string suffix = GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet);
    resultSet->Close();
    EXPECT_EQ(mediaName, "test.jpg");
    EXPECT_EQ(suffix, "jpg");
}

/**
 * @tc.name: [Photo表策略测试: 无MEDIA_NAME字段] MediaLibraryRdbStore_Insert_PhotoTable_002
 * @tc.desc: 测试Insert方法当没有MEDIA_NAME字段时不设置PHOTO_MEDIA_SUFFIX
 *           [1] 创建ValuesBucket并设置MEDIA_TYPE和PHOTO_DATE_ADDED，不设置MEDIA_NAME
 *           [2] 调用Insert方法插入数据
 *           [3] 查询验证PHOTO_MEDIA_SUFFIX未设置
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_Insert_PhotoTable_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Insert_PhotoTable_002");
    ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_TYPE, 1);
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    auto resultSet = QueryForTestResult(rowId);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    string mediaName = GetStringVal(PhotoColumn::MEDIA_NAME, resultSet);
    string suffix = GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet);
    resultSet->Close();
    EXPECT_EQ(mediaName, "");
    EXPECT_EQ(suffix, "");
}

/**
 * @tc.name: [Photo表策略测试: MEDIA_NAME无扩展名] MediaLibraryRdbStore_Insert_PhotoTable_003
 * @tc.desc: 测试Insert方法当MEDIA_NAME没有扩展名时不设置PHOTO_MEDIA_SUFFIX
 *           [1] 创建ValuesBucket并设置MEDIA_NAME为无扩展名的文件
 *           [2] 调用Insert方法插入数据
 *           [3] 查询验证PHOTO_MEDIA_SUFFIX未设置
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_Insert_PhotoTable_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Insert_PhotoTable_003");
    ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_NAME, "testfile");
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    auto resultSet = QueryForTestResult(rowId);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    string mediaName = GetStringVal(PhotoColumn::MEDIA_NAME, resultSet);
    string suffix = GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet);
    resultSet->Close();
    EXPECT_EQ(mediaName, "testfile");
    EXPECT_EQ(suffix, "");
}

/**
 * @tc.name: [Photo表策略测试: 无dateAdded字段] MediaLibraryRdbStore_Insert_PhotoTable_004
 * @tc.desc: 测试Insert方法当没有PHOTO_DATE_ADDED字段时使用当前时间并填充日期字段
 *           [1] 创建ValuesBucket不设置PHOTO_DATE_ADDED
 *           [2] 调用Insert方法插入数据
 *           [3] 查询验证日期字段已自动填充
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_Insert_PhotoTable_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Insert_PhotoTable_004");
    ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_NAME, "test.jpg");
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    auto resultSet = QueryForTestResult(rowId);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    int64_t dateAdded = GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet);
    int32_t dateYear = GetInt32Val(PhotoColumn::PHOTO_DATE_ADDED_YEAR, resultSet);
    int32_t dateMonth = GetInt32Val(PhotoColumn::PHOTO_DATE_ADDED_MONTH, resultSet);
    int32_t dateDay = GetInt32Val(PhotoColumn::PHOTO_DATE_ADDED_DAY, resultSet);
    resultSet->Close();
    EXPECT_EQ(dateAdded, 0);
    EXPECT_GT(dateYear, 0);
    EXPECT_GT(dateMonth, 0);
    EXPECT_GT(dateDay, 0);
}

/**
 * @tc.name: [Photo表策略测试: dateAdded为0] MediaLibraryRdbStore_Insert_PhotoTable_005
 * @tc.desc: 测试Insert方法当PHOTO_DATE_ADDED为0时使用当前时间
 *           [1] 创建ValuesBucket并设置PHOTO_DATE_ADDED为0
 *           [2] 调用Insert方法插入数据
 *           [3] 查询验证日期字段已自动填充
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_Insert_PhotoTable_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Insert_PhotoTable_005");
    ValuesBucket values;
    values.Put(MediaColumn::MEDIA_DATE_ADDED, 0L);
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    auto resultSet = QueryForTestResult(rowId);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    int64_t dateAdded = GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet);
    int32_t dateYear = GetInt32Val(PhotoColumn::PHOTO_DATE_ADDED_YEAR, resultSet);
    int32_t dateMonth = GetInt32Val(PhotoColumn::PHOTO_DATE_ADDED_MONTH, resultSet);
    int32_t dateDay = GetInt32Val(PhotoColumn::PHOTO_DATE_ADDED_DAY, resultSet);
    resultSet->Close();
    EXPECT_EQ(dateAdded, 0);
    EXPECT_GT(dateYear, 0);
    EXPECT_GT(dateMonth, 0);
    EXPECT_GT(dateDay, 0);
}

/**
 * @tc.name: [Photo表策略测试: dateAdded为负数] MediaLibraryRdbStore_Insert_PhotoTable_006
 * @tc.desc: 测试Insert方法当PHOTO_DATE_ADDED为负数时使用当前时间
 *           [1] 创建ValuesBucket并设置MEDIA_NAME、MEDIA_TYPE和PHOTO_DATE_ADDED为负数
 *           [2] 调用Insert方法插入数据
 *           [3] 查询验证日期字段已自动填充
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_Insert_PhotoTable_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Insert_PhotoTable_006");
    ValuesBucket values;
    values.Put(MediaColumn::MEDIA_DATE_ADDED, -1000L);
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    auto resultSet = QueryForTestResult(rowId);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    int64_t dateAdded = GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet);
    int32_t dateYear = GetInt32Val(PhotoColumn::PHOTO_DATE_ADDED_YEAR, resultSet);
    int32_t dateMonth = GetInt32Val(PhotoColumn::PHOTO_DATE_ADDED_MONTH, resultSet);
    int32_t dateDay = GetInt32Val(PhotoColumn::PHOTO_DATE_ADDED_DAY, resultSet);
    resultSet->Close();
    EXPECT_EQ(dateAdded, -1000L);
    EXPECT_GT(dateYear, 0);
    EXPECT_GT(dateMonth, 0);
    EXPECT_GT(dateDay, 0);
}

/**
 * @tc.name: [Photo表策略测试: 设置PHOTO_DATE_ADDED] MediaLibraryRdbStore_Insert_PhotoTable_007
 * @tc.desc: 测试Insert方法设置PHOTO_DATE_ADDED
 *           [1] 创建ValuesBucket并设置PHOTO_DATE_ADDED
 *           [2] 调用Insert方法插入数据
 *           [3] 查询验证日期字段已正确填充
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_Insert_PhotoTable_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Insert_PhotoTable_007");
    ValuesBucket values;
    values.Put(MediaColumn::MEDIA_DATE_ADDED, DATE_ADDED);
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    auto resultSet = QueryForTestResult(rowId);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    int64_t dateAdded = GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet);
    int32_t dateYear = GetInt32Val(PhotoColumn::PHOTO_DATE_ADDED_YEAR, resultSet);
    int32_t dateMonth = GetInt32Val(PhotoColumn::PHOTO_DATE_ADDED_MONTH, resultSet);
    int32_t dateDay = GetInt32Val(PhotoColumn::PHOTO_DATE_ADDED_DAY, resultSet);
    resultSet->Close();
    EXPECT_EQ(dateAdded, DATE_ADDED);
    EXPECT_EQ(dateYear, 2024);
    EXPECT_EQ(dateMonth, 202411);
    EXPECT_EQ(dateDay, 20241128);
}

/**
 * @tc.name: [Photo表策略测试: 日期字段已设置] MediaLibraryRdbStore_Insert_PhotoTable_008
 * @tc.desc: 测试Insert方法当日期字段已设置时不覆盖
 *           [1] 创建ValuesBucket并设置PHOTO_DATE_ADDED和日期字段
 *           [2] 调用Insert方法插入数据
 *           [3] 查询验证日期字段保持原值
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_Insert_PhotoTable_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Insert_PhotoTable_008");
    ValuesBucket values;
    values.Put(MediaColumn::MEDIA_DATE_ADDED, DATE_ADDED);
    values.Put(PhotoColumn::PHOTO_DATE_ADDED_YEAR, 2020);
    values.Put(PhotoColumn::PHOTO_DATE_ADDED_MONTH, 1);
    values.Put(PhotoColumn::PHOTO_DATE_ADDED_DAY, 1);
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    auto resultSet = QueryForTestResult(rowId);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    int64_t dateAdded = GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet);
    int32_t dateYear = GetInt32Val(PhotoColumn::PHOTO_DATE_ADDED_YEAR, resultSet);
    int32_t dateMonth = GetInt32Val(PhotoColumn::PHOTO_DATE_ADDED_MONTH, resultSet);
    int32_t dateDay = GetInt32Val(PhotoColumn::PHOTO_DATE_ADDED_DAY, resultSet);
    resultSet->Close();
    EXPECT_EQ(dateAdded, DATE_ADDED);
    EXPECT_EQ(dateYear, 2020);
    EXPECT_EQ(dateMonth, 1);
    EXPECT_EQ(dateDay, 1);
}

/**
 * 查询相册测试结果 - 根据rowId查询相册信息
 */
static shared_ptr<NativeRdb::ResultSet> QueryPhotoAlbumForTestResult(int64_t rowId)
{
    MEDIA_INFO_LOG("QueryPhotoAlbumForTestResult, rowId: %{public}" PRId64, rowId);
    return QueryForTestResult(PhotoAlbumColumns::TABLE, PhotoAlbumColumns::ALBUM_ID,
        rowId, PHOTO_ALBUM_QUERY_COLUMNS);
}

/**
 * 插入tab_cover_record表数据
 */
static int32_t InsertTabCoverRecord(const vector<ValueObject>& args)
{
    MEDIA_INFO_LOG("InsertTabCoverRecord");
    string insertCoverSql = "INSERT INTO tab_cover_record (album_type, album_subtype, lpath, "
        "cover_order_key, cover_order_subkey, cover_order_type, "
        "hidden_cover_order_key, hidden_cover_order_subkey, hidden_cover_order_type) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";
    return rdbStorePtr->ExecuteSql(insertCoverSql, args);
}

/**
 * @tc.name: [PhotoAlbum表策略测试: 插入相册数据] MediaLibraryRdbStore_Insert_PhotoAlbumTable_001
 * @tc.desc: 测试Insert静态方法正常插入相册数据
 *           [1] 创建ValuesBucket并设置ALBUM_NAME、ALBUM_TYPE、ALBUM_SUBTYPE
 *           [2] 调用Insert方法插入数据到PHOTO_ALBUM_TABLE
 *           [3] 查询验证相册信息正确
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_Insert_PhotoAlbumTable_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Insert_PhotoAlbumTable_001");
    ValuesBucket values;
    values.Put(PhotoAlbumColumns::ALBUM_NAME, "test_album");
    values.Put(PhotoAlbumColumns::ALBUM_TYPE, static_cast<int32_t>(PhotoAlbumType::USER));
    values.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    values.Put(PhotoAlbumColumns::ALBUM_LPATH, "/test/path");
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoAlbumColumns::TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    auto resultSet = QueryPhotoAlbumForTestResult(rowId);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    string albumName = GetStringVal(PhotoAlbumColumns::ALBUM_NAME, resultSet);
    int32_t albumType = GetInt32Val(PhotoAlbumColumns::ALBUM_TYPE, resultSet);
    int32_t albumSubtype = GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet);
    string lpath = GetStringVal(PhotoAlbumColumns::ALBUM_LPATH, resultSet);
    resultSet->Close();
    EXPECT_EQ(albumName, "test_album");
    EXPECT_EQ(albumType, static_cast<int32_t>(PhotoAlbumType::USER));
    EXPECT_EQ(albumSubtype, static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    EXPECT_EQ(lpath, "/test/path");
}

/**
 * @tc.name: [PhotoAlbum表策略测试: 系统相册不设置LPATH] MediaLibraryRdbStore_Insert_PhotoAlbumTable_002
 * @tc.desc: 测试Insert方法插入系统相册时不设置ALBUM_LPATH
 *           [1] 创建ValuesBucket并设置ALBUM_TYPE为SYSTEM，不设置ALBUM_LPATH
 *           [2] 调用Insert方法插入数据
 *           [3] 验证返回值为E_OK
 *           [4] 验证rowId大于0
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_Insert_PhotoAlbumTable_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Insert_PhotoAlbumTable_002");
    ValuesBucket values;
    values.Put(PhotoAlbumColumns::ALBUM_NAME, "system_album_no_lpath");
    values.Put(PhotoAlbumColumns::ALBUM_TYPE, static_cast<int32_t>(PhotoAlbumType::SYSTEM));
    values.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, static_cast<int32_t>(PhotoAlbumSubType::VIDEO));
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoAlbumColumns::TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    auto resultSet = QueryPhotoAlbumForTestResult(rowId);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    string lpath = GetStringVal(PhotoAlbumColumns::ALBUM_LPATH, resultSet);
    resultSet->Close();
    EXPECT_EQ(lpath, "");
}

/**
 * @tc.name: [PhotoAlbum表策略测试: tab_cover_record有数据-系统相册] MediaLibraryRdbStore_Insert_PhotoAlbumTable_003
 * @tc.desc: 测试AddCoverOrderValuesFromRecord逻辑-系统相册从tab_cover_record获取封面排序字段
 *           [1] 向tab_cover_record插入系统相册的封面排序数据
 *           [2] 创建ValuesBucket并设置ALBUM_TYPE为SYSTEM、ALBUM_SUBTYPE
 *           [3] 调用Insert方法插入数据
 *           [4] 查询验证封面排序字段从tab_cover_record正确获取
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_Insert_PhotoAlbumTable_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Insert_PhotoAlbumTable_003");
    // 插入tab_cover_record数据 - 系统相册
    vector<ValueObject> args = {
        static_cast<int32_t>(PhotoAlbumType::SYSTEM), static_cast<int32_t>(PhotoAlbumSubType::CAMERA),
        "", "sys_cover_key", "sys_cover_subkey", 1, "sys_hidden_key", "sys_hidden_subkey", 0
    };
    int32_t ret = InsertTabCoverRecord(args);
    EXPECT_EQ(ret, NativeRdb::E_OK);

    // 插入相册数据
    ValuesBucket values;
    values.Put(PhotoAlbumColumns::ALBUM_NAME, "system_album_with_cover");
    values.Put(PhotoAlbumColumns::ALBUM_TYPE, static_cast<int32_t>(PhotoAlbumType::SYSTEM));
    values.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, static_cast<int32_t>(PhotoAlbumSubType::CAMERA));
    int64_t rowId = -1;
    ret = MediaLibraryRdbStore::Insert(rowId, PhotoAlbumColumns::TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 验证封面排序字段
    auto resultSet = QueryPhotoAlbumForTestResult(rowId);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    string coverKey = GetStringVal(PhotoAlbumColumns::COVER_ORDER_KEY, resultSet);
    string coverSubKey = GetStringVal(PhotoAlbumColumns::COVER_ORDER_SUBKEY, resultSet);
    int32_t coverType = GetInt32Val(PhotoAlbumColumns::COVER_ORDER_TYPE, resultSet);
    string hiddenKey = GetStringVal(PhotoAlbumColumns::HIDDEN_COVER_ORDER_KEY, resultSet);
    string hiddenSubKey = GetStringVal(PhotoAlbumColumns::HIDDEN_COVER_ORDER_SUBKEY, resultSet);
    int32_t hiddenType = GetInt32Val(PhotoAlbumColumns::HIDDEN_COVER_ORDER_TYPE, resultSet);
    resultSet->Close();
    EXPECT_EQ(coverKey, "sys_cover_key");
    EXPECT_EQ(coverSubKey, "sys_cover_subkey");
    EXPECT_EQ(coverType, 1);
    EXPECT_EQ(hiddenKey, "sys_hidden_key");
    EXPECT_EQ(hiddenSubKey, "sys_hidden_subkey");
    EXPECT_EQ(hiddenType, 0);
}

/**
 * @tc.name: [PhotoAlbum表策略测试: tab_cover_record有数据-用户相册] MediaLibraryRdbStore_Insert_PhotoAlbumTable_004
 * @tc.desc: 测试AddCoverOrderValuesFromRecord逻辑-用户相册从tab_cover_record获取封面排序字段
 *           [1] 向tab_cover_record插入用户相册的封面排序数据（包含lpath）
 *           [2] 创建ValuesBucket并设置ALBUM_TYPE为USER、ALBUM_SUBTYPE、ALBUM_LPATH
 *           [3] 调用Insert方法插入数据
 *           [4] 查询验证封面排序字段从tab_cover_record正确获取
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_Insert_PhotoAlbumTable_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Insert_PhotoAlbumTable_004");
    // 插入tab_cover_record数据 - 用户相册
    vector<ValueObject> args = {
        static_cast<int32_t>(PhotoAlbumType::USER), static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC),
        "/user/path", "user_cover_key", "user_cover_subkey", 0, "user_hidden_key", "user_hidden_subkey", 1
    };
    int32_t ret = InsertTabCoverRecord(args);
    EXPECT_EQ(ret, NativeRdb::E_OK);

    // 插入相册数据
    ValuesBucket values;
    values.Put(PhotoAlbumColumns::ALBUM_NAME, "user_album_with_cover");
    values.Put(PhotoAlbumColumns::ALBUM_TYPE, static_cast<int32_t>(PhotoAlbumType::USER));
    values.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    values.Put(PhotoAlbumColumns::ALBUM_LPATH, "/user/path");
    int64_t rowId = -1;
    ret = MediaLibraryRdbStore::Insert(rowId, PhotoAlbumColumns::TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 验证封面排序字段
    auto resultSet = QueryPhotoAlbumForTestResult(rowId);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    string coverKey = GetStringVal(PhotoAlbumColumns::COVER_ORDER_KEY, resultSet);
    string coverSubKey = GetStringVal(PhotoAlbumColumns::COVER_ORDER_SUBKEY, resultSet);
    int32_t coverType = GetInt32Val(PhotoAlbumColumns::COVER_ORDER_TYPE, resultSet);
    string hiddenKey = GetStringVal(PhotoAlbumColumns::HIDDEN_COVER_ORDER_KEY, resultSet);
    string hiddenSubKey = GetStringVal(PhotoAlbumColumns::HIDDEN_COVER_ORDER_SUBKEY, resultSet);
    int32_t hiddenType = GetInt32Val(PhotoAlbumColumns::HIDDEN_COVER_ORDER_TYPE, resultSet);
    resultSet->Close();
    EXPECT_EQ(coverKey, "user_cover_key");
    EXPECT_EQ(coverSubKey, "user_cover_subkey");
    EXPECT_EQ(coverType, 0);
    EXPECT_EQ(hiddenKey, "user_hidden_key");
    EXPECT_EQ(hiddenSubKey, "user_hidden_subkey");
    EXPECT_EQ(hiddenType, 1);
}

/**
 * @tc.name: [PhotoAlbum表策略测试: tab_cover_record无数据] MediaLibraryRdbStore_Insert_PhotoAlbumTable_005
 * @tc.desc: 测试AddCoverOrderValuesFromRecord逻辑-tab_cover_record无数据时不设置封面排序字段
 *           [1] 创建ValuesBucket并设置相册属性
 *           [2] 调用Insert方法插入数据
 *           [3] 查询验证封面排序字段未设置
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_Insert_PhotoAlbumTable_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Insert_PhotoAlbumTable_005");
    // 不插入tab_cover_record数据
    ValuesBucket values;
    values.Put(PhotoAlbumColumns::ALBUM_NAME, "album_no_cover_record");
    values.Put(PhotoAlbumColumns::ALBUM_TYPE, static_cast<int32_t>(PhotoAlbumType::USER));
    values.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    values.Put(PhotoAlbumColumns::ALBUM_LPATH, "/no/record/path");
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoAlbumColumns::TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 验证封面排序字段未设置
    auto resultSet = QueryPhotoAlbumForTestResult(rowId);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    string coverKey = GetStringVal(PhotoAlbumColumns::COVER_ORDER_KEY, resultSet);
    string coverSubKey = GetStringVal(PhotoAlbumColumns::COVER_ORDER_SUBKEY, resultSet);
    int32_t coverType = GetInt32Val(PhotoAlbumColumns::COVER_ORDER_TYPE, resultSet);
    resultSet->Close();
    EXPECT_EQ(coverKey, "");
    EXPECT_EQ(coverSubKey, "");
    EXPECT_EQ(coverType, 0);
}

/**
 * @tc.name: [PhotoAlbum表策略测试: tab_cover_record部分字段为空] MediaLibraryRdbStore_Insert_PhotoAlbumTable_006
 * @tc.desc: 测试AddCoverOrderValuesFromRecord逻辑-tab_cover_record部分字段为空时不覆盖
 *           [1] 向tab_cover_record插入部分字段为空的数据
 *           [2] 创建ValuesBucket并设置部分封面排序字段
 *           [3] 调用Insert方法插入数据
 *           [4] 查询验证封面排序字段保持原值
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_Insert_PhotoAlbumTable_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Insert_PhotoAlbumTable_006");
    // 插入tab_cover_record数据 - 部分字段为空
    vector<ValueObject> args = {
        static_cast<int32_t>(PhotoAlbumType::SOURCE),
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC),
        "/source/path",
        "",  // cover_order_key为空
        "",  // cover_order_subkey为空
        2,  // cover_order_type非0/1
        "",  // hidden_cover_order_key为空
        "",  // hidden_cover_order_subkey为空
        2   // hidden_cover_order_type非0/1
    };
    int32_t ret = InsertTabCoverRecord(args);
    EXPECT_EQ(ret, NativeRdb::E_OK);

    // 插入相册数据 - 预先设置封面排序字段
    ValuesBucket values;
    values.Put(PhotoAlbumColumns::ALBUM_NAME, "source_album_partial_cover");
    values.Put(PhotoAlbumColumns::ALBUM_TYPE, static_cast<int32_t>(PhotoAlbumType::SOURCE));
    values.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC));
    values.Put(PhotoAlbumColumns::ALBUM_LPATH, "/source/path");
    values.Put(PhotoAlbumColumns::COVER_ORDER_KEY, "my_cover_key");
    values.Put(PhotoAlbumColumns::COVER_ORDER_SUBKEY, "my_cover_subkey");
    values.Put(PhotoAlbumColumns::COVER_ORDER_TYPE, 1);
    values.Put(PhotoAlbumColumns::HIDDEN_COVER_ORDER_KEY, "my_hidden_key");
    values.Put(PhotoAlbumColumns::HIDDEN_COVER_ORDER_SUBKEY, "my_hidden_subkey");
    values.Put(PhotoAlbumColumns::HIDDEN_COVER_ORDER_TYPE, 0);
    int64_t rowId = -1;
    ret = MediaLibraryRdbStore::Insert(rowId, PhotoAlbumColumns::TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 验证封面排序字段保持原值（因为tab_cover_record中为空）
    auto resultSet = QueryPhotoAlbumForTestResult(rowId);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    string coverKey = GetStringVal(PhotoAlbumColumns::COVER_ORDER_KEY, resultSet);
    string coverSubKey = GetStringVal(PhotoAlbumColumns::COVER_ORDER_SUBKEY, resultSet);
    int32_t coverType = GetInt32Val(PhotoAlbumColumns::COVER_ORDER_TYPE, resultSet);
    string hiddenKey = GetStringVal(PhotoAlbumColumns::HIDDEN_COVER_ORDER_KEY, resultSet);
    string hiddenSubKey = GetStringVal(PhotoAlbumColumns::HIDDEN_COVER_ORDER_SUBKEY, resultSet);
    int32_t hiddenType = GetInt32Val(PhotoAlbumColumns::HIDDEN_COVER_ORDER_TYPE, resultSet);
    resultSet->Close();
    EXPECT_EQ(coverKey, "my_cover_key");
    EXPECT_EQ(coverSubKey, "my_cover_subkey");
    EXPECT_EQ(coverType, 1);
    EXPECT_EQ(hiddenKey, "my_hidden_key");
    EXPECT_EQ(hiddenSubKey, "my_hidden_subkey");
    EXPECT_EQ(hiddenType, 0);
}

/**
 * @tc.name: [PhotoAlbum表策略测试: tab_cover_record覆盖字段] MediaLibraryRdbStore_Insert_PhotoAlbumTable_007
 * @tc.desc: 测试AddCoverOrderValuesFromRecord逻辑-tab_cover_record有数据时覆盖空字段
 *           [1] 向tab_cover_record插入完整数据
 *           [2] 创建ValuesBucket不设置封面排序字段
 *           [3] 调用Insert方法插入数据
 *           [4] 查询验证封面排序字段从tab_cover_record获取
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_Insert_PhotoAlbumTable_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Insert_PhotoAlbumTable_007");
    // 插入tab_cover_record数据 - 完整数据
    vector<ValueObject> args = {
        static_cast<int32_t>(PhotoAlbumType::SMART), static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT),
        "", "smart_cover_key", "smart_cover_subkey", 1, "smart_hidden_key", "smart_hidden_subkey", 0
    };
    int32_t ret = InsertTabCoverRecord(args);
    EXPECT_EQ(ret, NativeRdb::E_OK);

    // 插入相册数据 - 不设置封面排序字段
    ValuesBucket values;
    values.Put(PhotoAlbumColumns::ALBUM_NAME, "smart_album_auto_cover");
    values.Put(PhotoAlbumColumns::ALBUM_TYPE, static_cast<int32_t>(PhotoAlbumType::SMART));
    values.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT));
    values.Put(PhotoAlbumColumns::ALBUM_LPATH, "");
    int64_t rowId = -1;
    ret = MediaLibraryRdbStore::Insert(rowId, PhotoAlbumColumns::TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 验证封面排序字段从tab_cover_record获取
    auto resultSet = QueryPhotoAlbumForTestResult(rowId);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    string coverKey = GetStringVal(PhotoAlbumColumns::COVER_ORDER_KEY, resultSet);
    string coverSubKey = GetStringVal(PhotoAlbumColumns::COVER_ORDER_SUBKEY, resultSet);
    int32_t coverType = GetInt32Val(PhotoAlbumColumns::COVER_ORDER_TYPE, resultSet);
    string hiddenKey = GetStringVal(PhotoAlbumColumns::HIDDEN_COVER_ORDER_KEY, resultSet);
    string hiddenSubKey = GetStringVal(PhotoAlbumColumns::HIDDEN_COVER_ORDER_SUBKEY, resultSet);
    int32_t hiddenType = GetInt32Val(PhotoAlbumColumns::HIDDEN_COVER_ORDER_TYPE, resultSet);
    resultSet->Close();
    EXPECT_EQ(coverKey, "smart_cover_key");
    EXPECT_EQ(coverSubKey, "smart_cover_subkey");
    EXPECT_EQ(coverType, 1);
    EXPECT_EQ(hiddenKey, "smart_hidden_key");
    EXPECT_EQ(hiddenSubKey, "smart_hidden_subkey");
    EXPECT_EQ(hiddenType, 0);
}

/**
 * @tc.name: [PhotoAlbum表策略测试: tab_cover_record表不存在] MediaLibraryRdbStore_Insert_PhotoAlbumTable_008
 * @tc.desc: 测试AddCoverOrderValuesFromRecord逻辑-tab_cover_record表不存在时insert不应失败
 *           [1] 删除tab_cover_record表
 *           [2] 创建ValuesBucket插入相册数据
 *           [3] 调用Insert方法插入数据
 *           [4] 验证insert成功，封面排序字段为空
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_Insert_PhotoAlbumTable_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Insert_PhotoAlbumTable_008");
    // 删除tab_cover_record表
    string dropCoverTableSql = "DROP TABLE IF EXISTS tab_cover_record";
    int32_t ret = rdbStorePtr->ExecuteSql(dropCoverTableSql);
    EXPECT_EQ(ret, NativeRdb::E_OK);

    // 插入相册数据 - 不设置封面排序字段
    ValuesBucket values;
    values.Put(PhotoAlbumColumns::ALBUM_NAME, "test_album_no_cover_table");
    values.Put(PhotoAlbumColumns::ALBUM_TYPE, static_cast<int32_t>(PhotoAlbumType::USER));
    values.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    int64_t rowId = -1;
    ret = MediaLibraryRdbStore::Insert(rowId, PhotoAlbumColumns::TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 验证封面排序字段为空
    auto resultSet = QueryPhotoAlbumForTestResult(rowId);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    string coverKey = GetStringVal(PhotoAlbumColumns::COVER_ORDER_KEY, resultSet);
    string coverSubKey = GetStringVal(PhotoAlbumColumns::COVER_ORDER_SUBKEY, resultSet);
    int32_t coverType = GetInt32Val(PhotoAlbumColumns::COVER_ORDER_TYPE, resultSet);
    string hiddenKey = GetStringVal(PhotoAlbumColumns::HIDDEN_COVER_ORDER_KEY, resultSet);
    string hiddenSubKey = GetStringVal(PhotoAlbumColumns::HIDDEN_COVER_ORDER_SUBKEY, resultSet);
    int32_t hiddenType = GetInt32Val(PhotoAlbumColumns::HIDDEN_COVER_ORDER_TYPE, resultSet);
    resultSet->Close();
    EXPECT_TRUE(coverKey.empty());
    EXPECT_TRUE(coverSubKey.empty());
    EXPECT_EQ(coverType, 0);
    EXPECT_TRUE(hiddenKey.empty());
    EXPECT_TRUE(hiddenSubKey.empty());
    EXPECT_EQ(hiddenType, 0);
}

/**
 * @tc.name: [策略测试: 无策略表插入] MediaLibraryRdbStore_Insert_NoStrategyTable_001
 * @tc.desc: 测试没有注册策略的表插入数据应成功
 *           [1] 创建一个没有注册策略的测试表
 *           [2] 创建ValuesBucket插入数据
 *           [3] 验证插入成功
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_Insert_NoStrategyTable_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Insert_NoStrategyTable_001");
    // 创建没有注册策略的测试表
    string createTestTableSql = "CREATE TABLE IF NOT EXISTS test_no_strategy_table ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT, "
        "value INTEGER)";
    int32_t ret = rdbStorePtr->ExecuteSql(createTestTableSql);
    EXPECT_EQ(ret, NativeRdb::E_OK);

    // 插入数据
    ValuesBucket values;
    values.Put("name", "test_no_strategy");
    values.Put("value", 123);
    int64_t rowId = -1;
    ret = MediaLibraryRdbStore::Insert(rowId, "test_no_strategy_table", values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 查询验证数据
    std::vector<std::string> columns = {
        "name",
        "value",
    };
    auto resultSet = QueryForTestResult("test_no_strategy_table", "id", rowId, columns);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    string name = GetStringVal("name", resultSet);
    int32_t value = GetInt32Val("value", resultSet);
    resultSet->Close();
    EXPECT_EQ(name, "test_no_strategy");
    EXPECT_EQ(value, 123);

    // 清理测试表
    string dropTestTableSql = "DROP TABLE IF EXISTS test_no_strategy_table";
    ret = rdbStorePtr->ExecuteSql(dropTestTableSql);
    EXPECT_EQ(ret, NativeRdb::E_OK);
}

/**
 * @tc.name: [策略测试: Files表插入] MediaLibraryRdbStore_Insert_FilesTable_001
 * @tc.desc: 测试FilesTableStrategy没有重写ExtendInsertValues不影响插入
 *           [1] 创建ValuesBucket插入Files表数据
 *           [2] 调用Insert方法插入数据
 *           [3] 验证插入成功
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_Insert_FilesTable_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Insert_FilesTable_001");
    // 插入Files表数据
    ValuesBucket values;
    values.Put(CONST_MEDIA_DATA_DB_FILE_PATH, "/test/path/file.jpg");
    values.Put(MEDIA_DATA_DB_NAME, "file.jpg");
    values.Put(CONST_MEDIA_DATA_DB_MEDIA_TYPE, 1);
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, CONST_MEDIALIBRARY_TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 查询验证数据
    auto resultSet = QueryForTestResult(CONST_MEDIALIBRARY_TABLE, CONST_MEDIA_DATA_DB_ID, rowId, FILES_QUERY_COLUMNS);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    string filePath = GetStringVal(CONST_MEDIA_DATA_DB_FILE_PATH, resultSet);
    string fileName = GetStringVal(MEDIA_DATA_DB_NAME, resultSet);
    int32_t fileType = GetInt32Val(CONST_MEDIA_DATA_DB_MEDIA_TYPE, resultSet);
    resultSet->Close();
    EXPECT_EQ(filePath, "/test/path/file.jpg");
    EXPECT_EQ(fileName, "file.jpg");
    EXPECT_EQ(fileType, 1);
}

/**
 * @tc.name: [策略测试: PhotoMap表插入] MediaLibraryRdbStore_Insert_PhotoMapTable_001
 * @tc.desc: 测试PhotoMapTableStrategy没有重写ExtendInsertValues不影响插入
 *           [1] 创建ValuesBucket插入PhotoMap表数据
 *           [2] 调用Insert方法插入数据
 *           [3] 验证插入成功
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_Insert_PhotoMapTable_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Insert_PhotoMapTable_001");
    // 插入PhotoMap表数据
    ValuesBucket values;
    values.Put(PhotoMap::ALBUM_ID, 1);
    values.Put(PhotoMap::ASSET_ID, 100);
    int64_t rowId = -1;
    int32_t ret = MediaLibraryRdbStore::Insert(rowId, PhotoMap::TABLE, values);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowId, 0);

    // 查询验证数据 - PhotoMap使用复合主键，没有id字段
    auto resultSet = QueryForTestResult(PhotoMap::TABLE, PhotoMap::ALBUM_ID, 1, PHOTO_MAP_QUERY_COLUMNS);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    int32_t albumId = GetInt32Val(PhotoMap::ALBUM_ID, resultSet);
    int32_t assetId = GetInt32Val(PhotoMap::ASSET_ID, resultSet);
    resultSet->Close();
    EXPECT_EQ(albumId, 1);
    EXPECT_EQ(assetId, 100);
}

/**
 * @tc.name: [策略测试: ExtendInsertValues返回错误] MediaLibraryRdbStore_Insert_StrategyError_001
 * @tc.desc: 测试ExtendInsertValues返回非E_OK时插入失败
 *           [1] 创建自定义策略，重写ExtendInsertValues返回E_HAS_DB_ERROR
 *           [2] 注册策略到RdbTableStrategyManager
 *           [3] 调用Insert方法插入数据
 *           [4] 验证插入失败返回E_HAS_DB_ERROR
 *           [5] 清理注册的策略
 */
HWTEST_F(MediaLibraryRdbStoreInsertTest, MediaLibraryRdbStore_Insert_StrategyError_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbStore_Insert_StrategyError_001");

    // 创建测试表
    string createTestTableSql = "CREATE TABLE IF NOT EXISTS test_error_strategy_table ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT, "
        "value INTEGER)";
    int32_t ret = rdbStorePtr->ExecuteSql(createTestTableSql);
    EXPECT_EQ(ret, NativeRdb::E_OK);

    // 注册自定义策略
    auto errorStrategy = std::make_shared<MockErrorStrategy>();
    RdbTableStrategyManager::GetInstance().RegisterStrategy("test_error_strategy_table", errorStrategy);

    // 尝试插入数据 - 应该失败
    ValuesBucket values;
    values.Put("name", "test_error");
    values.Put("value", 999);
    int64_t rowId = -1;
    ret = MediaLibraryRdbStore::Insert(rowId, "test_error_strategy_table", values);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    EXPECT_EQ(rowId, -1);

    // 清理测试表
    string dropTestTableSql = "DROP TABLE IF EXISTS test_error_strategy_table";
    ret = rdbStorePtr->ExecuteSql(dropTestTableSql);
    EXPECT_EQ(ret, NativeRdb::E_OK);
}

} // namespace Media
} // namespace OHOS