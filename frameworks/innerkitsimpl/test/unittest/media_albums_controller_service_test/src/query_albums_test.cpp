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

#define MLOG_TAG "MediaAlbumsControllerServiceTest"

#include "query_albums_test.h"

#include <memory>
#include <string>

#include "media_albums_controller_service.h"

#include "media_file_utils.h"
#include "vision_db_sqls.h"
#include "query_albums_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "vision_column.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 3;
static std::vector<std::string> ALBUM_FETCH_COLUMNS = {
    PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_NAME, PhotoAlbumColumns::ALBUM_SUBTYPE
};

static std::vector<std::string> createTableSqlLists = {
    PhotoAlbumColumns::CREATE_TABLE,
    PhotoColumn::CREATE_PHOTO_TABLE,
    CREATE_ANALYSIS_ALBUM,
};

static std::vector<std::string> testTables = {
    PhotoColumn::PHOTOS_TABLE,
    PhotoAlbumColumns::TABLE,
    ANALYSIS_ALBUM_TABLE,
};

static std::string Quote(const std::string &str)
{
    return "'" + str + "'";
}

static void ClearTable(const string &table)
{
    int32_t rows = 0;
    RdbPredicates predicates(table);
    int32_t errCode = g_rdbStore->Delete(rows, predicates);
    CHECK_AND_RETURN_LOG(errCode == E_OK, "g_rdbStore->Delete errCode:%{public}d", errCode);
}

static void ShowResultSet(std::shared_ptr<DataShare::DataShareResultSet> resultSet)
{
    std::vector<std::string> columns;
    resultSet->GetAllColumnNames(columns);
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string tag;
        std::string rowData;
        for (size_t i = 0; i < columns.size(); i++) {
            std::string value;
            resultSet->GetString(i, value);
            rowData += tag + columns[i] + ":'" + value + "'";
            tag = ",";
        }
        MEDIA_INFO_LOG("rowData:[%{public}s]", rowData.c_str());
    }
}

static int32_t DeleteAlbum(int32_t albumType, int32_t albumSubType)
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(albumType));
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(albumSubType));
    int32_t rows = 0;
    int32_t err = g_rdbStore->Delete(rows, predicates);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to clear album table, err: %{public}d", err);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

static int32_t CreateAlbum(int32_t albumType, int32_t albumSubType)
{
    int64_t now = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t hidden = (albumSubType == PhotoAlbumSubType::HIDDEN) ? 1 : 0;
    const std::string albumName = "albumSubType" + to_string(albumSubType);
    std::vector<std::pair<std::string, std::string>> items = {
        {PhotoAlbumColumns::ALBUM_NAME, Quote(albumName)},
        {PhotoAlbumColumns::ALBUM_TYPE, to_string(albumType)},
        {PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(albumSubType)},
        {PhotoAlbumColumns::ALBUM_DATE_MODIFIED, to_string(now)},
        {PhotoAlbumColumns::ALBUM_DATE_ADDED, to_string(now)},
        {PhotoAlbumColumns::ALBUM_LPATH, Quote("/Pictures/" + albumName)},
        {PhotoAlbumColumns::CONTAINS_HIDDEN, to_string(hidden)},
        {PhotoAlbumColumns::ALBUM_IS_LOCAL, "1"},
        {PhotoAlbumColumns::ALBUM_PRIORITY, "1"},
    };

    std::string values;
    std::string columns;
    for (const auto &item : items) {
        if (!columns.empty()) {
            columns.append(",");
            values.append(",");
        }
        columns.append(item.first);
        values.append(item.second);
    }
    std::string sql = "INSERT INTO " + PhotoAlbumColumns::TABLE + "(" + columns + ") VALUES (" + values + ")";
    return g_rdbStore->ExecuteSql(sql);
}

void QueryAlbumsTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    MEDIA_INFO_LOG("QueryAlbumsTest  SetUpTestCase done");
}

void QueryAlbumsTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("QueryAlbumsTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void QueryAlbumsTest::SetUp()
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
    MEDIA_INFO_LOG("QueryAlbumsTest SetUp");
}

void QueryAlbumsTest::TearDown(void) {}

using ServiceCall = std::function<void(MessageParcel &data, MessageParcel &reply)>;

int32_t ServiceQueryAlbumsCount(QueryAlbumsReqBody &reqBody, ServiceCall call)
{
    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    call(data, reply);

    IPC::MediaRespVo<QueryAlbumsRespBody> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }

    int32_t errCode = respVo.GetErrCode();
    if (errCode != 0) {
        MEDIA_ERR_LOG("respVo.GetErrCode: %{public}d", errCode);
        return errCode;
    }

    auto resultSet = respVo.GetBody().resultSet;
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet nullptr");
        return -1;
    }

    ShowResultSet(resultSet);

    int32_t rowCount = 0;
    resultSet->GetRowCount(rowCount);
    resultSet->Close();
    return rowCount;
}

int32_t GetAlbumsCount(int32_t albumType, int32_t albumSubType)
{
    ServiceCall call = [](MessageParcel &data, MessageParcel &reply) {
        auto service = make_shared<MediaAlbumsControllerService>();
        service->QueryAlbums(data, reply);
    };

    QueryAlbumsReqBody reqBody;
    reqBody.albumType = albumType;
    reqBody.albumSubType = albumSubType;
    reqBody.columns = ALBUM_FETCH_COLUMNS;
    return ServiceQueryAlbumsCount(reqBody, call);
}

int32_t GetHiddenAlbumsCount(int32_t hiddenAlbumFetchMode, const std::vector<std::string> &columns = {})
{
    ServiceCall call = [](MessageParcel &data, MessageParcel &reply) {
        auto service = make_shared<MediaAlbumsControllerService>();
        service->QueryHiddenAlbums(data, reply);
    };

    QueryAlbumsReqBody reqBody;
    reqBody.hiddenAlbumFetchMode = hiddenAlbumFetchMode;
    reqBody.columns = ALBUM_FETCH_COLUMNS;
    reqBody.columns.insert(reqBody.columns.end(), columns.begin(), columns.end());
    return ServiceQueryAlbumsCount(reqBody, call);
}

int32_t CreateAlbumTest(int32_t albumType, int32_t albumSubType)
{
    int32_t count = 0;
    if (CreateAlbum(albumType, albumSubType) == E_OK) {
        count = GetAlbumsCount(albumType, albumSubType);
        DeleteAlbum(albumType, albumSubType);
    }
    return count;
}

HWTEST_F(QueryAlbumsTest, QueryHiddenAlbums_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start QueryHiddenAlbums_Test_001");
    ASSERT_LT(GetHiddenAlbumsCount(0, {"aaa"}), 0);
    ASSERT_LT(GetHiddenAlbumsCount(1, {"aaa"}), 0);
    ASSERT_EQ(GetHiddenAlbumsCount(0), 0);
    ASSERT_EQ(GetHiddenAlbumsCount(1), 0);
    if (CreateAlbum(PhotoAlbumType::SYSTEM, PhotoAlbumSubType::HIDDEN) == E_OK) {
        ASSERT_EQ(GetHiddenAlbumsCount(0), 1);
        DeleteAlbum(PhotoAlbumType::SYSTEM, PhotoAlbumSubType::HIDDEN);
    }
}

HWTEST_F(QueryAlbumsTest, QueryAlbums_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start QueryAlbums_Test_001");
    ASSERT_EQ(GetAlbumsCount(PhotoAlbumType::USER, PhotoAlbumSubType::USER_GENERIC), 0);
    ASSERT_EQ(CreateAlbumTest(PhotoAlbumType::USER, PhotoAlbumSubType::USER_GENERIC), 1);
}

HWTEST_F(QueryAlbumsTest, QueryAlbums_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start QueryAlbums_Test_002");
    ASSERT_EQ(GetAlbumsCount(PhotoAlbumType::SOURCE, PhotoAlbumSubType::SOURCE_GENERIC), 0);
    ASSERT_EQ(CreateAlbumTest(PhotoAlbumType::SOURCE, PhotoAlbumSubType::SOURCE_GENERIC), 1);
}

HWTEST_F(QueryAlbumsTest, QueryAlbums_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start QueryAlbums_Test_003");
    ASSERT_EQ(GetAlbumsCount(PhotoAlbumType::SYSTEM, PhotoAlbumSubType::FAVORITE), 0);
    ASSERT_EQ(GetAlbumsCount(PhotoAlbumType::SYSTEM, PhotoAlbumSubType::VIDEO), 0);
    ASSERT_EQ(GetAlbumsCount(PhotoAlbumType::SYSTEM, PhotoAlbumSubType::HIDDEN), 0);
    ASSERT_EQ(GetAlbumsCount(PhotoAlbumType::SYSTEM, PhotoAlbumSubType::TRASH), 0);
    ASSERT_EQ(GetAlbumsCount(PhotoAlbumType::SYSTEM, PhotoAlbumSubType::SCREENSHOT), 0);
    ASSERT_EQ(GetAlbumsCount(PhotoAlbumType::SYSTEM, PhotoAlbumSubType::CAMERA), 0);
    ASSERT_EQ(GetAlbumsCount(PhotoAlbumType::SYSTEM, PhotoAlbumSubType::IMAGE), 0);
    ASSERT_EQ(GetAlbumsCount(PhotoAlbumType::SYSTEM, PhotoAlbumSubType::CLOUD_ENHANCEMENT), 0);

    ASSERT_EQ(CreateAlbumTest(PhotoAlbumType::SYSTEM, PhotoAlbumSubType::FAVORITE), 1);
    ASSERT_EQ(CreateAlbumTest(PhotoAlbumType::SYSTEM, PhotoAlbumSubType::VIDEO), 1);
    ASSERT_EQ(CreateAlbumTest(PhotoAlbumType::SYSTEM, PhotoAlbumSubType::HIDDEN), 0);
    ASSERT_EQ(CreateAlbumTest(PhotoAlbumType::SYSTEM, PhotoAlbumSubType::TRASH), 0);
    ASSERT_EQ(CreateAlbumTest(PhotoAlbumType::SYSTEM, PhotoAlbumSubType::SCREENSHOT), 0);
    ASSERT_EQ(CreateAlbumTest(PhotoAlbumType::SYSTEM, PhotoAlbumSubType::CAMERA), 0);
    ASSERT_EQ(CreateAlbumTest(PhotoAlbumType::SYSTEM, PhotoAlbumSubType::IMAGE), 1);
    ASSERT_EQ(CreateAlbumTest(PhotoAlbumType::SYSTEM, PhotoAlbumSubType::CLOUD_ENHANCEMENT), 0);
}

HWTEST_F(QueryAlbumsTest, QueryAlbums_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start QueryAlbums_Test_004");
    ASSERT_EQ(GetAlbumsCount(PhotoAlbumType::SMART, PhotoAlbumSubType::CLASSIFY), 0);
    ASSERT_EQ(GetAlbumsCount(PhotoAlbumType::SMART, PhotoAlbumSubType::GEOGRAPHY_LOCATION), 0);
    ASSERT_EQ(GetAlbumsCount(PhotoAlbumType::SMART, PhotoAlbumSubType::GEOGRAPHY_CITY), 0);
    ASSERT_EQ(GetAlbumsCount(PhotoAlbumType::SMART, PhotoAlbumSubType::SHOOTING_MODE), 0);
    ASSERT_EQ(GetAlbumsCount(PhotoAlbumType::SMART, PhotoAlbumSubType::PORTRAIT), 0);
    ASSERT_EQ(GetAlbumsCount(PhotoAlbumType::SMART, PhotoAlbumSubType::GROUP_PHOTO), 0);
    ASSERT_EQ(GetAlbumsCount(PhotoAlbumType::SMART, PhotoAlbumSubType::HIGHLIGHT), 0);
    ASSERT_EQ(GetAlbumsCount(PhotoAlbumType::SMART, PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS), 0);
}
}  // namespace OHOS::Media