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

#include "media_fetch_result_test.h"

#include "datashare_result_set.h"
#include "media_log.h"
#include "rdb_helper.h"
#include "rdb_store_config.h"
#include "rdb_utils.h"
#define private public
#include "base_column.h"
#include "fetch_result.h"
#include "media_column.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "result_set_utils.h"
#undef private
#include "userfilemgr_uri.h"
#include "media_file_utils.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_data_manager.h"
#include "media_audio_column.h"
#include "media_upgrade.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
static shared_ptr<MediaLibraryRdbStore> g_rdbStore = nullptr;

static const int32_t TEST_IS_TRASH = 0;
static const int64_t TEST_SIZE = 0;
static const string TEST_PATH = "/data/test";
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
static const string TYPE_ONE = "1";

void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        MEDIALIBRARY_TABLE
    };
    for (auto &dropTable : dropTableList) {
        string dropSql = "DROP TABLE " + dropTable + ";";
        int32_t ret = g_rdbStore->ExecuteSql(dropSql);
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
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}

void ClearAndRestart()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    system("rm -rf /storage/cloud/files/.thumbs");
    system("rm -rf /storage/cloud/files/.editData");
    for (const auto &dir : TEST_ROOT_DIRS) {
        string ROOT_PATH = "/storage/cloud/100/files/";
        bool ret = MediaFileUtils::CreateDirectory(ROOT_PATH + dir + "/");
        CHECK_AND_PRINT_LOG(ret, "make %{public}s dir failed, ret=%{public}d", dir.c_str(), ret);
    }
    CleanTestTables();
    SetTables();
}

void MediaLibraryFetchResultUnitTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    EXPECT_NE(g_rdbStore, nullptr);
    ClearAndRestart();
    NativeRdb::ValuesBucket valuesBucket;
    for (int32_t mediaType = MediaType::MEDIA_TYPE_FILE; mediaType <= MEDIA_TYPE_MEDIA; mediaType ++) {
        int64_t rowId = -1;
        valuesBucket.PutInt(MEDIA_DATA_DB_MEDIA_TYPE, mediaType);
        valuesBucket.PutInt(MEDIA_DATA_DB_DATE_TRASHED, TEST_IS_TRASH);
        valuesBucket.PutLong(MEDIA_DATA_DB_SIZE, TEST_SIZE);
        valuesBucket.PutString(MEDIA_DATA_DB_FILE_PATH, TEST_PATH + to_string(mediaType));
        g_rdbStore->Insert(rowId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
        valuesBucket.Clear();
        EXPECT_EQ(rowId > 0, true);
    }
}

void MediaLibraryFetchResultUnitTest::TearDownTestCase(void)
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    this_thread::sleep_for(chrono::seconds(SLEEP_FIVE_SECONDS));
    MEDIA_INFO_LOG("Clean is finish");
    MEDIA_ERR_LOG("TearDownTestCase finish");
}

void MediaLibraryFetchResultUnitTest::SetUp(void) {}
void MediaLibraryFetchResultUnitTest::TearDown(void) {}

shared_ptr<DataShare::DataShareResultSet> GetFetchResult()
{
    vector<string> columns;
    NativeRdb::AbsRdbPredicates dirAbsPred(PhotoColumn::PHOTOS_TABLE);
    dirAbsPred.EqualTo(MEDIA_DATA_DB_SIZE, to_string(0))->And()->NotEqualTo(MEDIA_DATA_DB_MEDIA_TYPE, to_string(0));
    std::shared_ptr<NativeRdb::ResultSet> queryResultSet = g_rdbStore->Query(dirAbsPred, columns);
    shared_ptr<DataShare::ResultSetBridge> result = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(queryResultSet);
    return make_shared<DataShare::DataShareResultSet>(result);
}

shared_ptr<DataShare::DataShareResultSet> GetEmptyFetchResult()
{
    vector<string> columns;
    NativeRdb::AbsRdbPredicates dirAbsPred(PhotoColumn::PHOTOS_TABLE);
    dirAbsPred.EqualTo(MEDIA_DATA_DB_ID, to_string(0));
    std::shared_ptr<NativeRdb::ResultSet> queryResultSet = g_rdbStore->Query(dirAbsPred, columns);
    shared_ptr<DataShare::ResultSetBridge> result = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(queryResultSet);
    return make_shared<DataShare::DataShareResultSet>(result);
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Check set get function
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_SetGet_Test_001, TestSize.Level1)
{
    shared_ptr<FetchResult<FileAsset>> fetchResult = make_shared<FetchResult<FileAsset>>();
    fetchResult->Close();
    EXPECT_EQ(fetchResult->GetCount(), 0);

    auto result = make_unique<FetchResult<FileAsset>>(GetFetchResult());
    fetchResult->SetInfo(result);

    const string TEST_NETWORKID = "1d3cb099659d53b3ee15faaab3c00a8ff983382ebc8b01aabde039ed084e167b";
    fetchResult->SetNetworkId(TEST_NETWORKID);
    shared_ptr<DataShare::DataShareResultSet> datashareResultSet = nullptr;
    shared_ptr<FetchResult<FileAsset>> fetchResultTest = make_shared<FetchResult<FileAsset>>(datashareResultSet);
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Check getobjectatposition
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_GetObjectAtPosition_Test_001, TestSize.Level1)
{
    auto fetchResult = make_shared<FetchResult<FileAsset>>(GetFetchResult());
    EXPECT_NE(fetchResult->GetObjectAtPosition(1), nullptr);
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Check getobjectatposition
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_GetObjectAtPosition_Test_002, TestSize.Level1)
{
    shared_ptr<DataShare::DataShareResultSet> datashareResultSet = nullptr;
    shared_ptr<FetchResult<FileAsset>> fetchResult = make_shared<FetchResult<FileAsset>>(datashareResultSet);

    const int32_t TEST_INVALID_INDEX = -1;
    EXPECT_EQ(fetchResult->GetObjectAtPosition(TEST_INVALID_INDEX), nullptr);
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Check getfirstobject
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_GetFirstObject_Test_001, TestSize.Level1)
{
    auto fileFetchResult = make_shared<FetchResult<FileAsset>>(GetFetchResult());
    fileFetchResult->SetResultNapiType(ResultNapiType::TYPE_USERFILE_MGR);
    auto fileAsset = fileFetchResult->GetFirstObject();
    string filePath = TEST_PATH + TYPE_ONE;
    EXPECT_EQ(fileAsset->GetPath(), filePath);

    auto albumFetchResult = make_shared<FetchResult<AlbumAsset>>(GetFetchResult());
    albumFetchResult->SetResultNapiType(ResultNapiType::TYPE_USERFILE_MGR);
    auto albumAsset = albumFetchResult->GetFirstObject();
    EXPECT_EQ(albumAsset->GetResultNapiType(), ResultNapiType::TYPE_USERFILE_MGR);

    auto smartAlbumFetchResult = make_shared<FetchResult<SmartAlbumAsset>>(GetFetchResult());
    smartAlbumFetchResult->SetResultNapiType(ResultNapiType::TYPE_USERFILE_MGR);
    auto smartAlbumAsset = smartAlbumFetchResult->GetFirstObject();
    EXPECT_EQ(smartAlbumAsset->GetResultNapiType(), ResultNapiType::TYPE_USERFILE_MGR);

    auto result = make_shared<FetchResult<FileAsset>>();
    EXPECT_EQ(result->GetFirstObject(), nullptr);
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Check getfirstobject
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_GetFirstObject_Test_002, TestSize.Level1)
{
    auto fileFetchResult = make_shared<FetchResult<FileAsset>>(GetFetchResult());
    auto fileAsset = fileFetchResult->GetFirstObject();
    string filePath = TEST_PATH + TYPE_ONE;
    EXPECT_EQ(fileAsset->GetPath(), filePath);

    auto albumFetchResult = make_shared<FetchResult<AlbumAsset>>(GetFetchResult());
    auto albumAsset = albumFetchResult->GetFirstObject();
    EXPECT_EQ(albumAsset->GetResultNapiType(), ResultNapiType::TYPE_NAPI_MAX);

    auto smartAlbumFetchResult = make_shared<FetchResult<SmartAlbumAsset>>(GetFetchResult());
    auto smartAlbumAsset = smartAlbumFetchResult->GetFirstObject();
    EXPECT_EQ(smartAlbumAsset->GetResultNapiType(), ResultNapiType::TYPE_NAPI_MAX);

    auto result = make_shared<FetchResult<FileAsset>>();
    EXPECT_EQ(result->GetFirstObject(), nullptr);
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Check getfirstobject
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_GetFirstObject_Test_003, TestSize.Level1)
{
    auto fetchResult = make_shared<FetchResult<FileAsset>>(GetEmptyFetchResult());
    EXPECT_EQ(fetchResult->GetFirstObject(), nullptr);
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Check getnextobject
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_GetNextObject_Test_001, TestSize.Level1)
{
    auto fetchResult = make_shared<FetchResult<FileAsset>>(GetFetchResult());
    EXPECT_NE(fetchResult->GetNextObject(), nullptr);

    auto result = make_shared<FetchResult<FileAsset>>();
    EXPECT_EQ(result->GetNextObject(), nullptr);
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Check getnextobject
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_GetNextObject_Test_002, TestSize.Level1)
{
    auto fetchResult = make_shared<FetchResult<FileAsset>>(GetEmptyFetchResult());
    EXPECT_EQ(fetchResult->GetNextObject(), nullptr);
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Check getlastobject
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_GetLastObject_Test_001, TestSize.Level1)
{
    auto fetchResult = make_shared<FetchResult<FileAsset>>(GetFetchResult());
    EXPECT_NE(fetchResult->GetLastObject(), nullptr);

    auto result = make_shared<FetchResult<FileAsset>>();
    EXPECT_EQ(result->GetLastObject(), nullptr);
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Check getlastobject
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_GetLastObject_Test_002, TestSize.Level1)
{
    auto fetchResult = make_shared<FetchResult<FileAsset>>(GetEmptyFetchResult());
    EXPECT_EQ(fetchResult->GetLastObject(), nullptr);
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Check getobject
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_GetObject_Test_001, TestSize.Level1)
{
    auto fetchResult = make_shared<FetchResult<FileAsset>>(GetFetchResult());
    EXPECT_NE(fetchResult->GetObject(), nullptr);

    auto result = make_shared<FetchResult<FileAsset>>();
    EXPECT_NE(result->GetObject(), nullptr);
    EXPECT_EQ(result->GetObject()->GetPath(), DEFAULT_MEDIA_PATH);
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Check getobject
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_GetObject_Test_002, TestSize.Level1)
{
    auto fetchResult = make_shared<FetchResult<FileAsset>>(GetEmptyFetchResult());
    EXPECT_NE(fetchResult->GetObject(), nullptr);
    EXPECT_EQ(fetchResult->GetObject()->GetPath(), DEFAULT_MEDIA_PATH);
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Check getobject
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_GetObject_Test_003, TestSize.Level1)
{
    auto fetchResult = make_shared<FetchResult<FileAsset>>(GetEmptyFetchResult());
    shared_ptr<NativeRdb::ResultSet> resultSet;
    auto ret = fetchResult->GetObject(resultSet);
    EXPECT_NE(ret, nullptr);
    fetchResult->SetFetchResType(FetchResType::TYPE_ALBUM);
    fetchResult->GetNetworkId();
    fetchResult->GetDataShareResultSet();
    fetchResult->GetFetchResType();
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Check isatlastrow
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_IsAtLastRow_Test_001, TestSize.Level1)
{
    auto fetchResult = make_shared<FetchResult<FileAsset>>(GetFetchResult());
    EXPECT_EQ(fetchResult->IsAtLastRow(), false);

    auto result = make_shared<FetchResult<FileAsset>>();
    EXPECT_EQ(result->IsAtLastRow(), false);
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Check getobjectfromrdb
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_GetObjectFromRdb_Test_001, TestSize.Level1)
{
    vector<string> columns;
    NativeRdb::AbsRdbPredicates dirAbsPred(MEDIALIBRARY_TABLE);
    dirAbsPred.EqualTo(MEDIA_DATA_DB_SIZE, to_string(0))->And()->NotEqualTo(MEDIA_DATA_DB_MEDIA_TYPE, to_string(0));
    std::shared_ptr<NativeRdb::ResultSet> queryResultSet = g_rdbStore->Query(dirAbsPred, columns);
    auto result = make_shared<FetchResult<FileAsset>>();
    auto fileAsset = result->GetObjectFromRdb(queryResultSet, 1);
    EXPECT_EQ(fileAsset, nullptr);
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Check getobjectfromrdb
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_GetObjectFromRdb_Test_002, TestSize.Level1)
{
    auto result = make_shared<FetchResult<FileAsset>>();
    shared_ptr<NativeRdb::ResultSet> resultSet = nullptr;
    auto fileAsset = result->GetObjectFromRdb(resultSet, 1);
    EXPECT_EQ(fileAsset, nullptr);
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Check getobjectfromrdb
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_GetObjectFromRdb_Test_003, TestSize.Level1)
{
    vector<string> columns;
    NativeRdb::AbsRdbPredicates dirAbsPred(MEDIALIBRARY_TABLE);
    dirAbsPred.EqualTo(MEDIA_DATA_DB_ID, to_string(0));
    std::shared_ptr<NativeRdb::ResultSet> queryResultSet = g_rdbStore->Query(dirAbsPred, columns);
    auto result = make_shared<FetchResult<FileAsset>>();
    EXPECT_EQ(result->GetObjectFromRdb(queryResultSet, 1), nullptr);
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Check getobjectfromrdb
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_GetObjectFromRdb_Test_004, TestSize.Level1)
{
    vector<string> columns;
    NativeRdb::AbsRdbPredicates dirAbsPred(MEDIALIBRARY_TABLE);
    dirAbsPred.EqualTo(MEDIA_DATA_DB_SIZE, to_string(0))->And()->NotEqualTo(MEDIA_DATA_DB_MEDIA_TYPE, to_string(0));
    std::shared_ptr<NativeRdb::ResultSet> queryResultSet = g_rdbStore->Query(dirAbsPred, columns);
    auto result = make_shared<FetchResult<FileAsset>>();
    const int32_t TEST_INDEX_LARGE_THAN_ROWS = 100;
    EXPECT_EQ(result->GetObjectFromRdb(queryResultSet, TEST_INDEX_LARGE_THAN_ROWS), nullptr);
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Check SetFileAsset
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_SetFileAsset_Test_001, TestSize.Level1)
{
    auto result = make_shared<FetchResult<FileAsset>>();
    EXPECT_EQ(result->GetObject()->GetAlbumUri(), DEFAULT_MEDIA_ALBUM_URI);
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Check GetRowValFromColumn
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_GetRowValFromColumn_Test_001, TestSize.Level1)
{
    auto fetchResult = make_shared<FetchResult<FileAsset>>(GetEmptyFetchResult());
    shared_ptr<NativeRdb::ResultSet> resultSet;
    string columnName = "GetRowValFromColumn";
    ResultSetDataType dataType = TYPE_STRING;
    variant<int32_t, int64_t, string, double> ret = fetchResult->GetRowValFromColumn(columnName, dataType, resultSet);
    EXPECT_EQ(get<string>(ret), "");
    fetchResult->Close();
    ret = fetchResult->GetRowValFromColumn(columnName, dataType, resultSet);
    EXPECT_EQ(get<string>(ret), "");
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Check base
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_base_Test_001, TestSize.Level1)
{
    string ret = BaseColumn::CreateIndex();
    EXPECT_EQ(ret, "CREATE INDEX IF NOT EXISTS ");
    ret = BaseColumn::CreateTable();
    EXPECT_EQ(ret, "CREATE TABLE IF NOT EXISTS ");
    ret = BaseColumn::CreateTrigger();
    EXPECT_EQ(ret, "CREATE TRIGGER IF NOT EXISTS ");
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Check media_column
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_media_column_Test_001, TestSize.Level1)
{
    bool ret = PhotoColumn::IsPhotoColumn(PhotoColumn::PHOTO_ORIENTATION);
    EXPECT_EQ(ret, true);
    ret = PhotoColumn::IsPhotoColumn("media_column");
    EXPECT_EQ(ret, false);
    ret = AudioColumn::IsAudioColumn(AudioColumn::AUDIO_ALBUM);
    EXPECT_EQ(ret, true);
    ret = AudioColumn::IsAudioColumn("media_column");
    EXPECT_EQ(ret, false);
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : Close GetFirstObject GetNextObject GetLastObject
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_Test_001, TestSize.Level1)
{
    shared_ptr<FetchResult<AlbumAsset>> fetchResult = make_shared<FetchResult<AlbumAsset>>();
    ASSERT_NE(fetchResult, nullptr);

    fetchResult->resultset_ = nullptr;
    fetchResult->Close();
    auto res = fetchResult->GetNextObject();
    fetchResult->GetLastObject();
    EXPECT_EQ(res, nullptr);

    fetchResult->resultset_ = make_shared<DataShare::DataShareResultSet>();
    ASSERT_NE(fetchResult->resultset_, nullptr);

    fetchResult->GetFirstObject();
    res = fetchResult->GetNextObject();
    fetchResult->GetLastObject();
    EXPECT_EQ(res, nullptr);

    fetchResult->Close();
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : IsAtLastRow
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_Test_002, TestSize.Level1)
{
    shared_ptr<FetchResult<AlbumAsset>> fetchResult = make_shared<FetchResult<AlbumAsset>>();
    ASSERT_NE(fetchResult, nullptr);

    fetchResult->resultset_ = nullptr;
    bool res = fetchResult->IsAtLastRow();
    EXPECT_FALSE(res);

    fetchResult->resultset_ = make_shared<DataShare::DataShareResultSet>();
    ASSERT_NE(fetchResult->resultset_, nullptr);
    res = fetchResult->IsAtLastRow();
    bool retVal = false;
    fetchResult->resultset_->IsAtLastRow(retVal);
    EXPECT_EQ(res, retVal);

    fetchResult->Close();
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : GetRowValFromColumn
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_Test_003, TestSize.Level1)
{
    shared_ptr<FetchResult<AlbumAsset>> fetchResult = make_shared<FetchResult<AlbumAsset>>();
    ASSERT_NE(fetchResult, nullptr);

    fetchResult->resultset_ = nullptr;
    string columnName = "";
    shared_ptr<NativeRdb::ResultSet> resultSet = nullptr;
    ResultSetDataType dataType = TYPE_STRING;
    auto res = get<string>(fetchResult->GetRowValFromColumn(columnName, dataType, resultSet));
    EXPECT_EQ(res, "");

    fetchResult->resultset_ = make_shared<DataShare::DataShareResultSet>();
    ASSERT_NE(fetchResult->resultset_, nullptr);
    fetchResult->GetRowValFromColumn(columnName, dataType, resultSet);
}

/*
 * Feature : MediaLibraryFetchResultUnitTest
 * Function : GetValByIndex
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryFetchResultUnitTest, FetchResult_Test_004, TestSize.Level1)
{
    shared_ptr<FetchResult<AlbumAsset>> fetchResult = make_shared<FetchResult<AlbumAsset>>();
    ASSERT_NE(fetchResult, nullptr);

    fetchResult->resultset_ = nullptr;
    int32_t index = 1;
    ResultSetDataType dataType = TYPE_STRING;
    shared_ptr<NativeRdb::ResultSet> resultSet = nullptr;
    auto res = get<string>(fetchResult->GetValByIndex(index, dataType, resultSet));
    EXPECT_EQ(res, "");

    dataType = TYPE_DOUBLE;
    fetchResult->resultset_ = make_shared<DataShare::DataShareResultSet>();
    ASSERT_NE(fetchResult->resultset_, nullptr);
    fetchResult->GetValByIndex(index, dataType, resultSet);
}
} // namespace Media
} // namespace OHOS
