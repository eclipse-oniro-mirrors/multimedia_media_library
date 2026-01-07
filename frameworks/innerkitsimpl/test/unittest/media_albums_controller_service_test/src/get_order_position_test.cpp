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

#include "get_order_position_test.h"

#include <string>
#include <vector>

#define private public
#define protected public
#include "media_albums_controller_service.h"
#include "media_analysis_data_controller_service.h"
#undef private
#undef protected

#include "get_order_position_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "media_file_uri.h"
#include "vision_db_sqls.h"
#include "vision_photo_map_column.h"
#include "medialibrary_data_manager.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace IPC;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;

static string g_albumName = "test01";
static string g_data = "/storage/cloud/files/Photo/9/IMG_1748505946_009.jpg";
static string g_title = "cam_pic";

static std::vector<std::string> createTableSqlLists = {
    PhotoAlbumColumns::CREATE_TABLE,
    PhotoColumn::CREATE_PHOTO_TABLE,
    CREATE_ANALYSIS_ALBUM,
    CREATE_ANALYSIS_ALBUM_MAP,
};

static std::vector<std::string> testTables = {
    PhotoAlbumColumns::TABLE,
    PhotoColumn::PHOTOS_TABLE,
    ANALYSIS_ALBUM_TABLE,
    ANALYSIS_PHOTO_MAP_TABLE,
};

void GetOrderPositionTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    MEDIA_INFO_LOG("GetOrderPositionTest SetUpTestCase");
}

void GetOrderPositionTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("GetOrderPositionTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void GetOrderPositionTest::SetUp()
{
    MEDIA_INFO_LOG("GetOrderPositionTest SetUp");
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
}

void GetOrderPositionTest::TearDown(void) {}

static const string SQL_CREATE_ALBUM = "INSERT INTO " + ANALYSIS_ALBUM_TABLE + "(" +
    PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
    PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
    PhotoAlbumColumns::ALBUM_COUNT + ", " + PhotoAlbumColumns::ALBUM_DATE_MODIFIED + ")";

// (albumType == PhotoAlbumType::SMART) &&
// (albumSubType >= PhotoAlbumSubType::ANALYSIS_START && albumSubType <= PhotoAlbumSubType::ANALYSIS_END); 4097-4111
static void CreateAnalysisAlbum(const std::string &albumName)
{
    // album_type, album_subtype, album_name, date_modified, is_local, date_added, lpath, priority
    int32_t count = g_rdbStore->ExecuteSql("SELECT COUNT(*) FROM " + ANALYSIS_ALBUM_TABLE);
    int32_t albumId = count + 1;
    g_rdbStore->ExecuteSql(SQL_CREATE_ALBUM + "VALUES (" + to_string(albumId) +
        ", 4096, 4097, '"+ albumName + "', 2, 1748354341383)");
}

static const string SQL_INSERT_ASSET_POSITION = "INSERT INTO " + ANALYSIS_PHOTO_MAP_TABLE + "(" +
    MAP_ALBUM + ", " + MAP_ASSET + ", " + ORDER_POSITION + ")";

static void InsertAssetIntoPhotosTable(int32_t albumId, int32_t mapAsset, int32_t orderPosition)
{
    // map_album, map_asset, order_position
    g_rdbStore->ExecuteSql(SQL_INSERT_ASSET_POSITION + "VALUES (" + to_string(albumId) + ", " +
        to_string(mapAsset) + ", " + to_string(orderPosition) + ")");
}

static shared_ptr<NativeRdb::ResultSet> QueryAsset(const string& table, const string& key, const string& value,
    const vector<string>& columns)
{
    RdbPredicates rdbPredicates(table);
    rdbPredicates.EqualTo(key, value);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get file asset");
        return nullptr;
    }
    return resultSet;
}

static void GetOrderPositionPrepare(int32_t& albumId)
{
    // 1、创建AnalysisAlbum
    CreateAnalysisAlbum(g_albumName);
    vector<string> columns;
    auto resultSet = QueryAsset(ANALYSIS_ALBUM_TABLE, PhotoAlbumColumns::ALBUM_NAME, g_albumName, columns);
    if (resultSet == nullptr) {
        return;
    }

    albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
    if (albumId <= 0) {
        return;
    }

    // 2、插入一条数据照片到相册
    InsertAssetIntoPhotosTable(albumId, 10010, 666);
    InsertAssetIntoPhotosTable(albumId, 10011, 888);
    resultSet = QueryAsset(ANALYSIS_PHOTO_MAP_TABLE, MAP_ASSET, to_string(10010), columns);
    if (resultSet == nullptr) {
        return;
    }

    int32_t orderPosition = GetInt32Val(ORDER_POSITION, resultSet);
    if (orderPosition <= 0) {
        return;
    }
}

static int32_t GetOrderPosition(int32_t albumId, int32_t albumType, int32_t albumSubType,
    const std::vector<std::string> &assetsArray)
{
    GetOrderPositionReqBody reqBody;
    reqBody.albumId = albumId;
    reqBody.albumType = albumType;
    reqBody.albumSubType = albumSubType;
    reqBody.assetIdArray = assetsArray;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->GetOrderPosition(data, reply);

    IPC::MediaRespVo<GetOrderPositionRespBody> resp;
    if (resp.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("resp.Unmarshalling failed");
        return -1;
    }

    return resp.GetErrCode();
}

HWTEST_F(GetOrderPositionTest, GetOrderPositionTest_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetOrderPositionTest_Test_001");
    // 1、前置条件准备
    int32_t albumId = -1;
    GetOrderPositionPrepare(albumId);
    EXPECT_GT(albumId, 0);

    // 2、查询资产orderposition
    vector<string> assetsArray = {"10010", "10011"};
    int32_t ret = GetOrderPosition(albumId, PhotoAlbumType::USER, PhotoAlbumSubType::CLASSIFY, assetsArray);
    EXPECT_EQ(ret, E_INVALID_VALUES);

    ret = GetOrderPosition(albumId, PhotoAlbumType::SMART, PhotoAlbumSubType::ANALYSIS_START - 1, assetsArray);
    EXPECT_EQ(ret, E_INVALID_VALUES);

    ret = GetOrderPosition(albumId, PhotoAlbumType::SMART, PhotoAlbumSubType::ANALYSIS_END + 1, assetsArray);
    EXPECT_EQ(ret, E_INVALID_VALUES);

    ret = GetOrderPosition(albumId, PhotoAlbumType::SMART, PhotoAlbumSubType::CLASSIFY, {});
    EXPECT_EQ(ret, E_INVALID_VALUES);

    ret = GetOrderPosition(albumId, PhotoAlbumType::SMART, PhotoAlbumSubType::CLASSIFY, {"10010", "10010"});
    EXPECT_EQ(ret, E_INVALID_VALUES);

    ret = GetOrderPosition(albumId, PhotoAlbumType::SMART, PhotoAlbumSubType::CLASSIFY, assetsArray);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end GetOrderPositionTest_Test_001");
}

HWTEST_F(GetOrderPositionTest, GetOrderPositionTest_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetOrderPositionTest_Test_002");
    MessageParcel data;
    MessageParcel reply;
    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->GetOrderPosition(data, reply);

    IPC::MediaRespVo<GetOrderPositionRespBody> resp;
    ASSERT_EQ(resp.Unmarshalling(reply), true);
    ASSERT_LT(resp.GetErrCode(), 0);
    MEDIA_INFO_LOG("end GetOrderPositionTest_Test_002");
}
}  // namespace OHOS::Media