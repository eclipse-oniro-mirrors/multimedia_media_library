/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
#include <cstdint>
#include <thread>
#include "context.h"
#include "ability_context_impl.h"
#include "js_runtime.h"
#include "photo_album_column.h"
#include "media_column.h"
#include "media_file_utils.h"
#define private public
#include "medialibrary_asset_operations.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_sync_operation.h"
#include "medialibrary_rdb_test.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_rdbstore.h"
#undef private
#include "transaction.h"

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

shared_ptr<MediaLibraryRdbStore> rdbStorePtr = nullptr;

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
        PhotoColumn::CREATE_PHOTO_TABLE,
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

void MediaLibraryRdbTest::SetUpTestCase(void)
{
    auto stageContext = std::make_shared<AbilityRuntime::ContextImpl>();
    auto abilityContextImpl = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    abilityContextImpl->SetStageContext(stageContext);
    rdbStorePtr = std::make_shared<MediaLibraryRdbStore>(abilityContextImpl);
    int32_t ret = rdbStorePtr->Init();
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MediaLibraryRdbTest rdbstore start ret = %{public}d", ret);
}

void MediaLibraryRdbTest::TearDownTestCase(void) {}

// SetUp:Execute before each test case
void MediaLibraryRdbTest::SetUp() {}

void MediaLibraryRdbTest::TearDown(void) {}

HWTEST_F(MediaLibraryRdbTest, medialib_Insert_test_001, TestSize.Level0)
{
    if (rdbStorePtr == nullptr) {
        exit(1);
    }
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    cmd.SetTableName(MEDIALIBRARY_TABLE);
    ValuesBucket values;
    string name = "test name";
    values.PutString(MEDIA_DATA_DB_NAME, name);
    string data = "medialib_Insert_test_001";
    values.PutString(MEDIA_DATA_DB_FILE_PATH, data);
    string title = "insert test";
    values.PutString(MEDIA_DATA_DB_TITLE, title);
    int32_t fileId = 3;
    values.PutInt(MEDIA_DATA_DB_ID, fileId);
    cmd.SetValueBucket(values);
    int64_t rowId = 1;
    rdbStorePtr->Init();
    int32_t ret = rdbStorePtr->Insert(cmd, rowId);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Insert_test_002, TestSize.Level0)
{
    if (rdbStorePtr == nullptr) {
        exit(1);
    }
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    cmd.SetTableName(MEDIALIBRARY_TABLE);
    ValuesBucket values;
    string name = "medialib_Insert_test_002";
    values.PutString(MEDIA_DATA_DB_NAME, name);
    string displayname = "medialib_Insert_test_002/test";
    values.PutString(MEDIA_DATA_DB_NAME, displayname);
    cmd.SetValueBucket(values);
    int64_t rowId = 1;
    int32_t ret = rdbStorePtr->Insert(cmd, rowId);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Insert_test_003, TestSize.Level0)
{
    if (rdbStorePtr == nullptr) {
        exit(1);
    }
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    int64_t rowId = 1;
    int32_t ret = rdbStorePtr->Insert(cmd, rowId);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Insert_test_004, TestSize.Level0)
{
    if (rdbStorePtr == nullptr) {
        exit(1);
    }
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    cmd.SetTableName(MEDIALIBRARY_TABLE);
    ValuesBucket values;
    string name = "medialib_Insert_test_004";
    values.PutString(MEDIA_DATA_DB_NAME, name);
    cmd.SetValueBucket(values);
    int64_t rowId = 1;
    rdbStorePtr->Stop();
    int32_t ret = rdbStorePtr->Insert(cmd, rowId);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Query_test_001, TestSize.Level0)
{
    if (rdbStorePtr == nullptr) {
        exit(1);
    }
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::QUERY);
    vector<string> columns;
    columns.push_back(MEDIA_DATA_DB_RECYCLE_PATH);
    rdbStorePtr->Init();
    auto queryResultSet = rdbStorePtr->Query(cmd, columns);
    EXPECT_NE(queryResultSet, nullptr);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Query_test_002, TestSize.Level0)
{
    if (rdbStorePtr == nullptr) {
        exit(1);
    }
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::QUERY);
    vector<string> columns;
    columns.push_back(MEDIA_DATA_DB_DATE_TRASHED);
    rdbStorePtr->Stop();
    auto queryResultSet = rdbStorePtr->Query(cmd, columns);
    EXPECT_EQ(queryResultSet, nullptr);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Delete_test_001, TestSize.Level0)
{
    if (rdbStorePtr == nullptr) {
        exit(1);
    }
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::DELETE);
    string selection = MEDIA_DATA_DB_ID + " = ? OR " + MEDIA_DATA_DB_PARENT_ID + " = ?";
    cmd.GetAbsRdbPredicates()->SetWhereClause(selection);
    vector<string> selectionArgs;
    int32_t rowId = 3;
    selectionArgs.push_back(to_string(rowId));
    selectionArgs.push_back(to_string(rowId));
    cmd.GetAbsRdbPredicates()->SetWhereArgs(selectionArgs);
    rdbStorePtr->Init();
    int32_t ret = rdbStorePtr->Delete(cmd, rowId);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Delete_test_002, TestSize.Level0)
{
    if (rdbStorePtr == nullptr) {
        exit(1);
    }
    rdbStorePtr->Stop();
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::DELETE);
    int32_t rowId = 1;
    int32_t ret = rdbStorePtr->Delete(cmd, rowId);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Update_test_001, TestSize.Level0)
{
    if (rdbStorePtr == nullptr) {
        exit(1);
    }
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::UPDATE);
    ValuesBucket valuesBucket;
    string title = "medialib_Update_test_001";
    valuesBucket.PutString(MEDIA_DATA_DB_TITLE, title);
    cmd.SetValueBucket(valuesBucket);
    int32_t updatedRows = E_HAS_DB_ERROR;
    rdbStorePtr->Init();
    int32_t ret = rdbStorePtr->Update(cmd, updatedRows);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Update_test_002, TestSize.Level0)
{
    if (rdbStorePtr == nullptr) {
        exit(1);
    }
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::UPDATE);
    int32_t updatedRows = -1;
    int32_t ret = rdbStorePtr->Update(cmd, updatedRows);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Update_test_003, TestSize.Level0)
{
    if (rdbStorePtr == nullptr) {
        exit(1);
    }
    rdbStorePtr->Stop();
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::UPDATE);
    int32_t updatedRows = E_HAS_DB_ERROR;
    int32_t ret = rdbStorePtr->Update(cmd, updatedRows);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

HWTEST_F(MediaLibraryRdbTest, medialib_QuerySql_test_001, TestSize.Level0)
{
    if (rdbStorePtr == nullptr) {
        exit(1);
    }
    rdbStorePtr->Init();
    auto queryResultSet = rdbStorePtr->QuerySql(QUERY_MEDIA_VOLUME);
    EXPECT_NE(queryResultSet, nullptr);
}

HWTEST_F(MediaLibraryRdbTest, medialib_QuerySql_test_002, TestSize.Level0)
{
    if (rdbStorePtr == nullptr) {
        exit(1);
    }
    rdbStorePtr->Stop();
    auto queryResultSet = rdbStorePtr->QuerySql(QUERY_MEDIA_VOLUME);
    EXPECT_EQ(queryResultSet, nullptr);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Transaction_test_001, TestSize.Level0)
{
    if (rdbStorePtr == nullptr) {
        exit(1);
    }
    rdbStorePtr->Init();
    TransactionOperations trans1;
    int32_t ret = trans1.Start(__func__);
    EXPECT_EQ(ret, E_OK);
    TransactionOperations trans2;
    int32_t ret1 = trans2.Start(__func__);
    EXPECT_NE(ret1, E_OK);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::UPDATE);
    ValuesBucket valuesBucket;
    string title = "medialib_Update_test_001";
    valuesBucket.PutString(MEDIA_DATA_DB_TITLE, title);
    cmd.SetValueBucket(valuesBucket);
    int32_t updatedRows = E_HAS_DB_ERROR;
    ret = rdbStorePtr->Update(cmd, updatedRows);
    EXPECT_NE(ret, E_OK);
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.GreaterThan("file_id", 0);
    ret = trans1.Update(valuesBucket, predicates);
    EXPECT_TRUE(ret >= 0);
    trans2.Finish();
}

HWTEST_F(MediaLibraryRdbTest, medialib_Transaction_test_002, TestSize.Level0)
{
    if (rdbStorePtr == nullptr) {
        exit(1);
    }
    rdbStorePtr->Stop();
    TransactionOperations trans;
    int32_t ret = trans.Start(__func__);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

#ifdef DISTRIBUTED
HWTEST_F(MediaLibraryRdbTest, medialib_MediaLibraryRdbStoreObserver_test_001, TestSize.Level0)
{
    string bundleName = "medialib_MediaLibraryRdbStoreObserver_test_001";
    std::shared_ptr<MediaLibraryRdbStoreObserver> test = make_shared<MediaLibraryRdbStoreObserver>(bundleName);
    EXPECT_NE(test, nullptr);
}
#endif

HWTEST_F(MediaLibraryRdbTest, medialib_ExecuteSql_test_001, TestSize.Level0)
{
    if (rdbStorePtr == nullptr) {
        exit(1);
    }
    string modifySql = "UPDATE " + MEDIALIBRARY_TABLE + " SET ";
    int32_t ret = rdbStorePtr->ExecuteSql(modifySql);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

HWTEST_F(MediaLibraryRdbTest, medialib_ExecuteSql_test_002, TestSize.Level0)
{
    if (rdbStorePtr == nullptr) {
        exit(1);
    }
    rdbStorePtr->Stop();
    string modifySql = "UPDATE " + MEDIALIBRARY_TABLE + " SET ";
    int32_t ret = rdbStorePtr->ExecuteSql(modifySql);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Stop_test_001, TestSize.Level0)
{
    if (rdbStorePtr == nullptr) {
        exit(1);
    }
    rdbStorePtr->Stop();
    EXPECT_NE(rdbStorePtr, nullptr);
}

inline void PrepareUserAlbum(const string &albumName, const string &relativePath, ValuesBucket &values)
{
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, albumName);
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumType::USER);
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::USER_GENERIC);
    values.PutLong(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());

    if (!relativePath.empty()) {
        values.PutString(PhotoAlbumColumns::ALBUM_RELATIVE_PATH, relativePath);
    }
}

HWTEST_F(MediaLibraryRdbTest, medialib_BuildValuesSql_test_001, TestSize.Level0)
{
    if (rdbStorePtr == nullptr) {
        exit(1);
    }
    string sql;
    vector<ValueObject> bindArgs;
    sql.append("INSERT").append(" OR ROLLBACK").append(" INTO ").append(PhotoColumn::PHOTOS_TABLE).append(" ");
    ValuesBucket albumValues;
    PrepareUserAlbum("BuildValuesSql", "Documents/", albumValues);
    MediaLibraryRdbStore::BuildValuesSql(albumValues, bindArgs, sql);
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    vector<std::string> columns;
    MediaLibraryRdbStore::BuildQuerySql(predicates, columns, bindArgs, sql);
    int32_t ret = MediaLibraryRdbStore::ExecuteForLastInsertedRowId(sql, bindArgs);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    rdbStorePtr->Init();
    ret = MediaLibraryAssetOperations::DeleteFromDisk(predicates, false);
    EXPECT_EQ(ret, E_SUCCESS);
    string retTest = MediaLibraryRdbStore::CloudSyncTriggerFunc(columns);
    EXPECT_EQ(retTest, "");
    retTest = MediaLibraryRdbStore::IsCallerSelfFunc(columns);
    EXPECT_EQ(retTest, "true");
    rdbStorePtr->Stop();
    ret = MediaLibraryRdbStore::ExecuteForLastInsertedRowId(sql, bindArgs);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

HWTEST_F(MediaLibraryRdbTest, medialib_TransactionOperations_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("medialib_TransactionOperations_test_001 begin");
    rdbStorePtr->Init();
    TransactionOperations trans1;
    trans1.Finish();
    int32_t ret = trans1.Start(__func__);
    EXPECT_EQ(ret, E_OK);
    TransactionOperations trans2;
    auto res = trans2.Start(__func__);
    ret = res;
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    trans1.Finish();
    trans2.Finish();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    rdbStorePtr->Stop();
    MEDIA_INFO_LOG("medialib_TransactionOperations_test_001 end");
}

void TransactionTestFunc(shared_ptr<MediaLibraryRdbStore> rdbStorePtr, int *startSignal, int *endSignal,
    int32_t sleepTimeMs)
{
    TransactionOperations trans;
    int32_t ret = trans.Start(__func__);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Start failed, ret=%{public}d", ret);
        return;
    }
    (*startSignal)++;
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMs));
    trans.Finish();
    (*endSignal)++;
}

HWTEST_F(MediaLibraryRdbTest, medialib_TransactionOperations_test_002, TestSize.Level0)
{
    // test Transcation success
    MEDIA_INFO_LOG("medialib_TransactionOperations_test_002 begin");
    rdbStorePtr->Init();
    int startSignal = 0;
    int endSignal = 0;
    thread(TransactionTestFunc, rdbStorePtr, (&startSignal), (&endSignal), 0).detach();
    thread(TransactionTestFunc, rdbStorePtr, (&startSignal), (&endSignal), 10).detach();
    thread(TransactionTestFunc, rdbStorePtr, (&startSignal), (&endSignal), 50).detach();
    thread(TransactionTestFunc, rdbStorePtr, (&startSignal), (&endSignal), 100).detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_EQ(startSignal, 4);
    EXPECT_EQ(endSignal, 4);
    rdbStorePtr->Stop();
    MEDIA_INFO_LOG("medialib_TransactionOperations_test_002 finish");
}

HWTEST_F(MediaLibraryRdbTest, medialib_TransactionOperations_test_003, TestSize.Level0)
{
    // test Transcation failed
    MEDIA_INFO_LOG("medialib_TransactionOperations_test_003 begin");
    int startSignal = 0;
    int endSignal = 0;
    rdbStorePtr->Init();
    thread(TransactionTestFunc, rdbStorePtr, (&startSignal), (&endSignal), 4000).detach();
    thread(TransactionTestFunc, rdbStorePtr, (&startSignal), (&endSignal), 4000).detach();
    thread(TransactionTestFunc, rdbStorePtr, (&startSignal), (&endSignal), 4000).detach();
    thread(TransactionTestFunc, rdbStorePtr, (&startSignal), (&endSignal), 4000).detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    EXPECT_EQ(startSignal, 1);
    EXPECT_EQ(endSignal, 1);
    rdbStorePtr->Stop();
    MEDIA_INFO_LOG("medialib_TransactionOperations_test_003 finish");
}

HWTEST_F(MediaLibraryRdbTest, medialib_UpdateLastVisitTime_test_001, TestSize.Level0)
{
    if (rdbStorePtr == nullptr) {
        exit(1);
    }
    string id = "1";
    rdbStorePtr->Init();
    int32_t ret = rdbStorePtr->UpdateLastVisitTime(id);
    EXPECT_GE(ret, E_OK);
}

HWTEST_F(MediaLibraryRdbTest, medialib_ResetAnalysisTables_test, TestSize.Level0)
{
    if (rdbStorePtr == nullptr) {
        exit(1);
    }
    // normal rdbStore_ ResetAnalysisTables will success
    rdbStorePtr->Init();
    auto ret = MediaLibraryRdbStore::ResetAnalysisTables();
    EXPECT_EQ(ret, true);

    // abnormal rdbStore_ ResetAnalysisTables will fail
    rdbStorePtr->Stop();
    ret = MediaLibraryRdbStore::ResetAnalysisTables();
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryRdbTest, medialib_ResetSearchTables_test, TestSize.Level0)
{
    if (rdbStorePtr == nullptr) {
        exit(1);
    }
    // normal rdbStore_ ResetSearchTables will success
    rdbStorePtr->Init();
    auto ret = MediaLibraryRdbStore::ResetSearchTables();
    EXPECT_EQ(ret, true);

    // abnormal rdbStore_ ResetSearchTables will fail
    rdbStorePtr->Stop();
    ret = MediaLibraryRdbStore::ResetSearchTables();
    EXPECT_EQ(ret, false);
}
} // namespace Media
} // namespace OHOS