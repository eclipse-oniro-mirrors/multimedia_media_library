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
#include "medialibrary_rdb_helper.h"
#include "medialibrary_rdb_operations.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_rdb_test.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_rdbstore.h"
#undef private
#include "transaction.h"
#include "media_upgrade.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
static constexpr int32_t SLEEP_SECOND = 1;
const std::string QUERY_MEDIA_VOLUME = std::string("SELECT sum(") + CONST_MEDIA_DATA_DB_SIZE + ") AS " +
    CONST_MEDIA_DATA_DB_SIZE + "," +
    CONST_MEDIA_DATA_DB_MEDIA_TYPE + " FROM " +
    CONST_MEDIALIBRARY_TABLE + " WHERE " +
    CONST_MEDIA_DATA_DB_MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_FILE) + " OR " +
    CONST_MEDIA_DATA_DB_MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_IMAGE) + " OR " +
    CONST_MEDIA_DATA_DB_MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_VIDEO) + " OR " +
    CONST_MEDIA_DATA_DB_MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_ALBUM) + " OR " +
    CONST_MEDIA_DATA_DB_MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_AUDIO) + " GROUP BY " +
    CONST_MEDIA_DATA_DB_MEDIA_TYPE;

static shared_ptr<MediaLibraryRdbStore> rdbStorePtr = nullptr;

void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        CONST_MEDIALIBRARY_TABLE,
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

void MediaLibraryRdbTest::TearDownTestCase(void)
{}

// SetUp:Execute before each test case
void MediaLibraryRdbTest::SetUp() {}

void MediaLibraryRdbTest::TearDown(void)
{
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECOND));
}

HWTEST_F(MediaLibraryRdbTest, medialib_Insert_test_001, TestSize.Level1)
{
    ASSERT_NE(rdbStorePtr, nullptr);
    rdbStorePtr->Init();
    string deleteSql = string("DELETE FROM ") + CONST_MEDIALIBRARY_TABLE +";";
    int32_t ret = rdbStorePtr->ExecuteSql(deleteSql);
    EXPECT_EQ(ret, E_OK);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    cmd.SetTableName(CONST_MEDIALIBRARY_TABLE);
    ValuesBucket values;
    string name = "test name";
    values.PutString(CONST_MEDIA_DATA_DB_NAME, name);
    string data = "medialib_Insert_test_001";
    values.PutString(CONST_MEDIA_DATA_DB_FILE_PATH, data);
    string title = "insert test";
    values.PutString(CONST_MEDIA_DATA_DB_TITLE, title);
    int32_t fileId = 3;
    values.PutInt(CONST_MEDIA_DATA_DB_ID, fileId);
    cmd.SetValueBucket(values);
    int64_t rowId = 1;
    ret = rdbStorePtr->Insert(cmd, rowId);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Insert_test_002, TestSize.Level1)
{
    ASSERT_NE(rdbStorePtr, nullptr);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    cmd.SetTableName(CONST_MEDIALIBRARY_TABLE);
    ValuesBucket values;
    string name = "medialib_Insert_test_002";
    values.PutString(CONST_MEDIA_DATA_DB_NAME, name);
    string displayname = "medialib_Insert_test_002/test";
    values.PutString(CONST_MEDIA_DATA_DB_NAME, displayname);
    cmd.SetValueBucket(values);
    int64_t rowId = 1;
    int32_t ret = rdbStorePtr->Insert(cmd, rowId);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Insert_test_003, TestSize.Level1)
{
    ASSERT_NE(rdbStorePtr, nullptr);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    int64_t rowId = 1;
    int32_t ret = rdbStorePtr->Insert(cmd, rowId);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Insert_test_004, TestSize.Level1)
{
    ASSERT_NE(rdbStorePtr, nullptr);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    cmd.SetTableName(CONST_MEDIALIBRARY_TABLE);
    ValuesBucket values;
    string name = "medialib_Insert_test_004";
    values.PutString(CONST_MEDIA_DATA_DB_NAME, name);
    cmd.SetValueBucket(values);
    int64_t rowId = 1;
    rdbStorePtr->Stop();
    int32_t ret = rdbStorePtr->Insert(cmd, rowId);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Query_test_001, TestSize.Level1)
{
    ASSERT_NE(rdbStorePtr, nullptr);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::QUERY);
    vector<string> columns;
    columns.push_back(CONST_MEDIA_DATA_DB_RECYCLE_PATH);
    rdbStorePtr->Init();
    auto queryResultSet = rdbStorePtr->Query(cmd, columns);
    EXPECT_NE(queryResultSet, nullptr);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Query_test_002, TestSize.Level1)
{
    ASSERT_NE(rdbStorePtr, nullptr);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::QUERY);
    vector<string> columns;
    columns.push_back(CONST_MEDIA_DATA_DB_DATE_TRASHED);
    rdbStorePtr->Stop();
    auto queryResultSet = rdbStorePtr->Query(cmd, columns);
    EXPECT_EQ(queryResultSet, nullptr);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Delete_test_001, TestSize.Level1)
{
    ASSERT_NE(rdbStorePtr, nullptr);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::DELETE);
    string selection = string(CONST_MEDIA_DATA_DB_ID) + " = ? OR " + CONST_MEDIA_DATA_DB_PARENT_ID + " = ?";
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

HWTEST_F(MediaLibraryRdbTest, medialib_Delete_test_002, TestSize.Level1)
{
    ASSERT_NE(rdbStorePtr, nullptr);
    rdbStorePtr->Stop();
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::DELETE);
    int32_t rowId = 1;
    int32_t ret = rdbStorePtr->Delete(cmd, rowId);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Update_test_001, TestSize.Level1)
{
    ASSERT_NE(rdbStorePtr, nullptr);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::UPDATE);
    ValuesBucket valuesBucket;
    string title = "medialib_Update_test_001";
    valuesBucket.PutString(CONST_MEDIA_DATA_DB_TITLE, title);
    cmd.SetValueBucket(valuesBucket);
    int32_t updatedRows = E_HAS_DB_ERROR;
    rdbStorePtr->Init();
    int32_t ret = rdbStorePtr->Update(cmd, updatedRows);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Update_test_002, TestSize.Level1)
{
    ASSERT_NE(rdbStorePtr, nullptr);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::UPDATE);
    int32_t updatedRows = -1;
    int32_t ret = rdbStorePtr->Update(cmd, updatedRows);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Update_test_003, TestSize.Level1)
{
    ASSERT_NE(rdbStorePtr, nullptr);
    rdbStorePtr->Stop();
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::UPDATE);
    int32_t updatedRows = E_HAS_DB_ERROR;
    int32_t ret = rdbStorePtr->Update(cmd, updatedRows);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

HWTEST_F(MediaLibraryRdbTest, medialib_QuerySql_test_001, TestSize.Level1)
{
    ASSERT_NE(rdbStorePtr, nullptr);
    rdbStorePtr->Init();
    auto queryResultSet = rdbStorePtr->QuerySql(QUERY_MEDIA_VOLUME);
    EXPECT_NE(queryResultSet, nullptr);
}

HWTEST_F(MediaLibraryRdbTest, medialib_QuerySql_test_002, TestSize.Level1)
{
    ASSERT_NE(rdbStorePtr, nullptr);
    rdbStorePtr->Stop();
    auto queryResultSet = rdbStorePtr->QuerySql(QUERY_MEDIA_VOLUME);
    EXPECT_EQ(queryResultSet, nullptr);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Transaction_test_001, TestSize.Level1)
{
    ASSERT_NE(rdbStorePtr, nullptr);
    rdbStorePtr->Init();
    TransactionOperations trans1{ __func__ };
    int32_t ret = trans1.Start();
    EXPECT_EQ(ret, E_OK);
    TransactionOperations trans2{ __func__ };
    int32_t ret1 = trans2.Start();
    EXPECT_EQ(ret1, E_OK);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::UPDATE);
    ValuesBucket valuesBucket;
    string title = "medialib_Update_test_001";
    valuesBucket.PutString(CONST_MEDIA_DATA_DB_TITLE, title);
    cmd.SetValueBucket(valuesBucket);
    int32_t updatedRows = E_HAS_DB_ERROR;
    ret = rdbStorePtr->UpdateWithDateTime(cmd, updatedRows);
    EXPECT_EQ(ret, E_OK);
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.GreaterThan("file_id", 0);
    ret = trans1.Update(valuesBucket, predicates);
    EXPECT_TRUE(ret >= 0);
    trans2.Finish();
}

HWTEST_F(MediaLibraryRdbTest, medialib_Transaction_test_002, TestSize.Level1)
{
    ASSERT_NE(rdbStorePtr, nullptr);
    rdbStorePtr->Stop();
    TransactionOperations trans{ __func__ };
    int32_t ret = trans.Start();
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

HWTEST_F(MediaLibraryRdbTest, medialib_ExecuteSql_test_001, TestSize.Level1)
{
    ASSERT_NE(rdbStorePtr, nullptr);
    string modifySql = string("UPDATE ") + CONST_MEDIALIBRARY_TABLE + " SET ";
    int32_t ret = rdbStorePtr->ExecuteSql(modifySql);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

HWTEST_F(MediaLibraryRdbTest, medialib_ExecuteSql_test_002, TestSize.Level1)
{
    ASSERT_NE(rdbStorePtr, nullptr);
    rdbStorePtr->Stop();
    string modifySql = string("UPDATE ") + CONST_MEDIALIBRARY_TABLE + " SET ";
    int32_t ret = rdbStorePtr->ExecuteSql(modifySql);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Stop_test_001, TestSize.Level1)
{
    ASSERT_NE(rdbStorePtr, nullptr);
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

HWTEST_F(MediaLibraryRdbTest, medialib_TransactionOperations_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_TransactionOperations_test_001 begin");
    rdbStorePtr->Init();
    TransactionOperations trans1{ __func__ };
    trans1.Finish();
    int32_t ret = trans1.Start();
    EXPECT_EQ(ret, E_OK);
    TransactionOperations trans2{ __func__ };
    auto res = trans2.Start();
    ret = res;
    EXPECT_EQ(ret, E_OK);
    trans1.Finish();
    trans2.Finish();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    rdbStorePtr->Stop();
    MEDIA_INFO_LOG("medialib_TransactionOperations_test_001 end");
}

void TransactionTestFunc(shared_ptr<MediaLibraryRdbStore> rdbStorePtr, int* startSignal, int* endSignal,
    int32_t sleepTimeMs)
{
    TransactionOperations trans{ __func__ };
    int32_t ret = trans.Start();
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Start failed, ret=%{public}d", ret);
        return;
    }
    (*startSignal)++;
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMs));
    trans.Finish();
    (*endSignal)++;
}

HWTEST_F(MediaLibraryRdbTest, medialib_TransactionOperations_test_002, TestSize.Level1)
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

HWTEST_F(MediaLibraryRdbTest, medialib_TransactionOperations_test_003, TestSize.Level1)
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
    EXPECT_EQ(startSignal, 4);
    EXPECT_EQ(endSignal, 4);
    rdbStorePtr->Stop();
    MEDIA_INFO_LOG("medialib_TransactionOperations_test_003 finish");
}

HWTEST_F(MediaLibraryRdbTest, medialib_GenerateHighlightThumbnail_test, TestSize.Level1)
{
    ASSERT_NE(rdbStorePtr, nullptr);
    rdbStorePtr->Init();
    vector<string> args = {"1", "tracks"};
    auto ret = MediaLibraryRdbStore::BeginGenerateHighlightThumbnail(args);
    EXPECT_EQ(ret, "");
    args = {"1", "tracks", "", "insert"};
    ret = MediaLibraryRdbStore::BeginGenerateHighlightThumbnail(args);
    EXPECT_EQ(ret, "");
    rdbStorePtr->Stop();
}

HWTEST_F(MediaLibraryRdbTest, medialib_PhotoAlbumNotifyFunc_001, TestSize.Level1)
{
    vector<string> args = {};
    auto ret = MediaLibraryRdbStore::PhotoAlbumNotifyFunc(args);
    EXPECT_EQ(ret, "");
}

class RdbStoreImplTestOpenCallback : public RdbOpenCallback {
public:
    int OnCreate(RdbStore &store) override;
    int OnUpgrade(RdbStore &store, int oldVersion, int newVersion) override;
};

int RdbStoreImplTestOpenCallback::OnCreate(RdbStore &store)
{
    return E_OK;
}

int RdbStoreImplTestOpenCallback::OnUpgrade(RdbStore &store, int oldVersion, int newVersion)
{
    return E_OK;
}

constexpr const char *CREATE_TABLE_TEST = "CREATE TABLE IF NOT EXISTS test"
                                          "(id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                          "name TEXT NOT NULL, age INTEGER, salary "
                                          "REAL, blobType BLOB)";
const std::string RDB_TEST_PATH = "/data/test/";

/**
 * @tc.name: MultiThread_Release_OldHandle_0001
 *           After one thread fully releases the store and clears the cache, another thread holding its
 *           own shared_ptr copy still sees the stale handle alive but its pool gone.
 * @tc.desc: 1.thread A (main): store->Release() + RdbHelper::ClearStoreCache(), then store.reset()
 *           2.thread B (executor): with its own shared_ptr copy, Insert returns E_ALREADY_CLOSED and
 *                       QuerySql returns nullptr after A finishes, with no crash.
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryRdbTest, MultiThread_Release_OldHandle_0001, TestSize.Level2)
{
    RdbHelper::DeleteRdbStore(RDB_TEST_PATH + "release_basic_test.db");
    RdbStoreConfig sqliteSharedRstConfig(RDB_TEST_PATH + "release_basic_test.db");
    RdbStoreImplTestOpenCallback sqliteSharedRstHelper;
    int errCode = E_OK;
    std::shared_ptr<RdbStore> rdbStore =
        RdbHelper::GetRdbStore(sqliteSharedRstConfig, 1, sqliteSharedRstHelper, errCode);
    EXPECT_NE(rdbStore, nullptr);
    EXPECT_EQ(errCode, E_OK);
 
    // Thread A (main): fully release and clear the cache, then drop the reference.
    EXPECT_EQ(E_OK, rdbStore->Release());
}

/**
 * @tc.name: RdbStore_Release_001
 * @tc.desc: Test RdbStore::Release() hard-closes the connection pool; later operations fail.
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryRdbTest, RdbStore_Release_001, TestSize.Level2)
{
    const std::string db = RDB_TEST_PATH + "release_basic_test.db";
    RdbHelper::DeleteRdbStore(db);
    RdbStoreConfig config(db);
    config.SetBundleName("com.example.distributed.rdb");
    RdbStoreImplTestOpenCallback helper;
    int errCode = E_OK;
    std::shared_ptr<RdbStore> store = RdbHelper::GetRdbStore(config, 1, helper, errCode);
    ASSERT_NE(store, nullptr);
    ASSERT_EQ(E_OK, errCode);
 
    EXPECT_EQ(E_OK, store->ExecuteSql(CREATE_TABLE_TEST));
    ASSERT_EQ(E_OK, store->Release());
    EXPECT_NE(E_OK, store->ExecuteSql(CREATE_TABLE_TEST));
 
    RdbHelper::ClearStoreCache(config);
    RdbHelper::DeleteRdbStore(db);
}
 
/**
 * @tc.name: RdbStore_ClearStoreCache_001
 * @tc.desc: ClearStoreCache evicts the cached store so the next GetRdbStore yields a fresh instance.
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryRdbTest, RdbStore_ClearStoreCache_001, TestSize.Level2)
{
    const std::string db = RDB_TEST_PATH + "clearcache_test.db";
    RdbHelper::DeleteRdbStore(db);
    RdbStoreConfig config(db);
    config.SetBundleName("com.example.distributed.rdb");
    RdbStoreImplTestOpenCallback helper;
    int errCode = E_OK;
    std::shared_ptr<RdbStore> storeA = RdbHelper::GetRdbStore(config, 1, helper, errCode);
    ASSERT_NE(storeA, nullptr);
    ASSERT_EQ(E_OK, errCode);
 
    ASSERT_EQ(E_OK, storeA->Release());
    EXPECT_NE(E_OK, storeA->ExecuteSql(CREATE_TABLE_TEST));
    EXPECT_EQ(E_OK, RdbHelper::ClearStoreCache(config));
 
    std::shared_ptr<RdbStore> storeB = RdbHelper::GetRdbStore(config, 1, helper, errCode);
    ASSERT_NE(storeB, nullptr);
    EXPECT_NE(storeA.get(), storeB.get());
    EXPECT_EQ(E_OK, storeB->ExecuteSql(CREATE_TABLE_TEST));
 
    RdbHelper::DeleteRdbStore(db);
}
 
/**
 * @tc.name: RdbStore_Release_Timeout_ResultSet_001
 * @tc.desc: Release returns E_DATABASE_BUSY while ResultSets still hold read connections, and E_OK
 *           once the last one is closed. Bounded-wait semantics: Release never blocks past waitTime.
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryRdbTest, RdbStore_Release_Timeout_ResultSet_001, TestSize.Level2)
{
    const std::string db = RDB_TEST_PATH + "release_timeout_resultset_test.db";
    RdbHelper::DeleteRdbStore(db);
 
    RdbStoreConfig config(db);
    config.SetJournalMode(JournalMode::MODE_WAL);
    config.SetReadConSize(8);
    config.SetBundleName("com.example.distributed.rdb");
    RdbStoreImplTestOpenCallback helper;
    int errCode = E_OK;
    std::shared_ptr<RdbStore> store = RdbHelper::GetRdbStore(config, 1, helper, errCode);
    ASSERT_NE(store, nullptr);
    ASSERT_EQ(E_OK, errCode);
    EXPECT_EQ(E_OK, store->ExecuteSql(CREATE_TABLE_TEST));
    EXPECT_EQ(E_OK, store->ExecuteSql("INSERT INTO test (name, age) VALUES ('a', 1);"));
 
    std::vector<std::shared_ptr<ResultSet>> resultSets;
    for (int i = 0; i < 5; i++) {
        auto rs = store->QueryByStep("SELECT * FROM test");
        ASSERT_NE(rs, nullptr);
        resultSets.push_back(rs);
    }
 
    EXPECT_EQ(E_DATABASE_BUSY, store->Release({ 1000 }));
    EXPECT_EQ(E_OK, store->ExecuteSql("INSERT INTO test (name, age) VALUES ('b', 2);"));
 
    EXPECT_EQ(E_OK, resultSets[0]->Close());
    resultSets.erase(resultSets.begin());
    EXPECT_EQ(E_DATABASE_BUSY, store->Release({ 1000 }));
 
    for (size_t i = 0; i + 1 < resultSets.size(); i++) {
        EXPECT_EQ(E_OK, resultSets[i]->Close());
    }
    EXPECT_EQ(E_DATABASE_BUSY, store->Release({ 1000 }));
 
    EXPECT_EQ(E_OK, resultSets.back()->Close());
    resultSets.clear();
    EXPECT_EQ(E_OK, store->Release({ 1000 }));
 
    RdbHelper::ClearStoreCache(config);
    RdbHelper::DeleteRdbStore(db);
}
} // namespace Media
} // namespace OHOS