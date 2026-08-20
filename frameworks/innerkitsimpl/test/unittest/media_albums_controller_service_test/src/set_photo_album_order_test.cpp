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

#include "set_photo_album_order_test.h"

#include <memory>
#include <string>

#define private public
#define protected public
#include "media_albums_controller_service.h"
#undef private
#undef protected

#include "set_photo_album_order_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "result_set_utils.h"
#include "photo_album_column.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;

static std::vector<std::string> createTableSqlLists = {
    PhotoAlbumColumns::CREATE_TABLE,
};

static std::vector<std::string> testTables = {
    PhotoAlbumColumns::TABLE,
};

static const string SQL_CREATE_ALBUM = "INSERT INTO " + PhotoAlbumColumns::TABLE + "(" +
    PhotoAlbumColumns::ALBUM_TYPE + ", " + PhotoAlbumColumns::ALBUM_SUBTYPE + ", " +
    PhotoAlbumColumns::ALBUM_NAME + ", " + PhotoAlbumColumns::ALBUM_DATE_MODIFIED + ", " +
    PhotoAlbumColumns::ALBUM_IS_LOCAL + ", " + PhotoAlbumColumns::ALBUM_DATE_ADDED + ", " +
    PhotoAlbumColumns::ALBUM_LPATH + ", " + PhotoAlbumColumns::ALBUM_PRIORITY + ", " +
    PhotoAlbumColumns::ALBUMS_ORDER + ", " + PhotoAlbumColumns::ORDER_SECTION + ", " +
    PhotoAlbumColumns::ORDER_TYPE + ", " + PhotoAlbumColumns::ORDER_STATUS + ")";

static const vector<string> ADD_ALBUMORDER_COLUMN_SQLS = {
    "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
        PhotoAlbumColumns::ALBUMS_ORDER + " INT NOT NULL DEFAULT -1",
    "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
        PhotoAlbumColumns::ORDER_SECTION + " INT NOT NULL DEFAULT -1",
    "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
        PhotoAlbumColumns::ORDER_TYPE + " INT NOT NULL DEFAULT -1",
    "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
        PhotoAlbumColumns::ORDER_STATUS + " INT NOT NULL DEFAULT 0"
};

static shared_ptr<NativeRdb::ResultSet> QueryAlbum(const string& album_name, const vector<string>& columns)
{
    RdbPredicates rdbPredicates(PhotoAlbumColumns::TABLE);
    rdbPredicates.EqualTo(PhotoAlbumColumns::ALBUM_NAME, album_name);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get file asset");
        return nullptr;
    }
    return resultSet;
}

static void CreateUserAlbum(const std::string &albumName)
{
    // album_type, album_subtype, album_name, date_modified, is_local, date_added, lpath, priority
    g_rdbStore->ExecuteSql(SQL_CREATE_ALBUM + "VALUES (0, 1, '"+
        albumName + "', 1748354341383, 1 , 1748354341383, '/Pictures/Users/" + albumName + "', 1, -1, -1, -1, 0)");
}

static void AddAlbumOrderColumn()
{
    for (const auto &sql : ADD_ALBUMORDER_COLUMN_SQLS) {
        g_rdbStore->ExecuteSql(sql);
    }
}

static void CreateSetAlbumOrderReqBody(SetPhotoAlbumOrderReqBody &reqBody, const int32_t &albumId)
{
    reqBody.albumOrderColumn = "albums_order";
    reqBody.orderSectionColumn = "order_section";
    reqBody.orderTypeColumn = "order_type";
    reqBody.orderStatusColumn = "order_status";

    reqBody.albumIds = {albumId};
    reqBody.albumOrders = {10};
    reqBody.orderSection = {0};
    reqBody.orderType = {1};
    reqBody.orderStatus = {1};
}

void SetPhotoAlbumOrderTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    MEDIA_INFO_LOG("SetPhotoAlbumOrderTest SetUpTestCase");
}

void SetPhotoAlbumOrderTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("SetPhotoAlbumOrderTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void SetPhotoAlbumOrderTest::SetUp()
{
    MEDIA_INFO_LOG("SetPhotoAlbumOrderTest SetUp");
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
}

void SetPhotoAlbumOrderTest::TearDown(void) {}

HWTEST_F(SetPhotoAlbumOrderTest, SetPhotoAlbumOrderTest_001, TestSize.Level0) {
    MEDIA_INFO_LOG("Start SetPhotoAlbumOrderTest_001");
    MessageParcel data;
    MessageParcel reply;

    auto service = make_shared<MediaAlbumsControllerService>();
    service->UpdatePhotoAlbumOrder(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    ASSERT_LT(respVo.GetErrCode(), 0);

    MEDIA_INFO_LOG("End SetPhotoAlbumOrderTest_001");
}

HWTEST_F(SetPhotoAlbumOrderTest, SetPhotoAlbumOrderTest_002, TestSize.Level0) {
    MEDIA_INFO_LOG("Start SetPhotoAlbumOrderTest_002");
    MessageParcel data;
    MessageParcel reply;
    SetPhotoAlbumOrderReqBody reqBody;
    
    AddAlbumOrderColumn();

    string albumName = "test02";
    CreateUserAlbum(albumName);
    vector<string> columns;
    auto resultSet = QueryAlbum(albumName, columns);
    ASSERT_NE(resultSet, nullptr);
    int32_t albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
    EXPECT_GT(albumId, 0);

    CreateSetAlbumOrderReqBody(reqBody, albumId);

    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);
    auto service = make_shared<MediaAlbumsControllerService>();
    service->UpdatePhotoAlbumOrder(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);

    int32_t err = respVo.GetErrCode();
    EXPECT_EQ(err, 0);

    MEDIA_INFO_LOG("End SetPhotoAlbumOrderTest_002");
}
}  // namespace OHOS::Media