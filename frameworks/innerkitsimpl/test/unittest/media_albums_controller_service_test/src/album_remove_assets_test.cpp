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

#include "album_remove_assets_test.h"

#include <string>
#include <vector>

#define private public
#define protected public
#include "media_albums_controller_service.h"
#undef private
#undef protected

#include "album_remove_assets_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "result_set_utils.h"
#include "media_file_uri.h"
#include "media_upgrade.h"

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
    PhotoUpgrade::CREATE_PHOTO_TABLE,
};

static std::vector<std::string> testTables = {
    PhotoAlbumColumns::TABLE,
    PhotoColumn::PHOTOS_TABLE,
};

void AlbumRemoveAssetsTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    MEDIA_INFO_LOG("AlbumRemoveAssetsTest SetUpTestCase succeed");
}

void AlbumRemoveAssetsTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("AlbumRemoveAssetsTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void AlbumRemoveAssetsTest::SetUp()
{
    MEDIA_INFO_LOG("AlbumRemoveAssetsTest SetUp");
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
}

void AlbumRemoveAssetsTest::TearDown(void) {}

static const string SQL_CREATE_ALBUM = "INSERT INTO " + PhotoAlbumColumns::TABLE + "(" +
    PhotoAlbumColumns::ALBUM_TYPE + ", " + PhotoAlbumColumns::ALBUM_SUBTYPE + ", " +
    PhotoAlbumColumns::ALBUM_NAME + ", " + PhotoAlbumColumns::ALBUM_DATE_MODIFIED + ", " +
    PhotoAlbumColumns::ALBUM_IS_LOCAL + ", " + PhotoAlbumColumns::ALBUM_DATE_ADDED + ", " +
    PhotoAlbumColumns::ALBUM_LPATH + ", " + PhotoAlbumColumns::ALBUM_PRIORITY + ")";

static void CreateUserAlbum(const std::string &albumName)
{
    // album_type, album_subtype, album_name, date_modified, is_local, date_added, lpath, priority
    g_rdbStore->ExecuteSql(SQL_CREATE_ALBUM + "VALUES (0, 1, '"+
        albumName + "', 1748354341383, 1 , 1748354341383, '/Pictures/Users/" + albumName + "', 1)");
}

static const string SQL_INSERT_PHOTO = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" +
    MediaColumn::MEDIA_FILE_PATH + ", " + MediaColumn::MEDIA_SIZE + ", " + MediaColumn::MEDIA_TITLE + ", " +
    MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_TYPE + ", " + MediaColumn::MEDIA_OWNER_PACKAGE + ", " +
    MediaColumn::MEDIA_PACKAGE_NAME + ", " + MediaColumn::MEDIA_DATE_ADDED + ", "  +
    MediaColumn::MEDIA_DATE_MODIFIED + ", " + MediaColumn::MEDIA_DATE_TAKEN + ", " +
    MediaColumn::MEDIA_DURATION + ", " + MediaColumn::MEDIA_IS_FAV + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
    MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_HEIGHT + ", " + PhotoColumn::PHOTO_WIDTH + ", " +
    PhotoColumn::PHOTO_EDIT_TIME + ", " + PhotoColumn::PHOTO_POSITION + ", " +
    PhotoColumn::PHOTO_SHOOTING_MODE + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ")";

static void InsertAssetIntoPhotosTable(const string& data, const string& title, int32_t albumId)
{
    // data, size, title, display_name, media_type,position
    // owner_package, package_name, date_added, date_modified, date_taken, duration, is_favorite, date_trashed, hidden
    // height, width, edit_time, position, shooting_mode, owner_album_id
    g_rdbStore->ExecuteSql(SQL_INSERT_PHOTO + "VALUES ('" + data + "', 175258, '" + title + "', '" +
        title + ".jpg', 1, 'com.ohos.camera', '相机', 1748423617814, 1748424146785, 1748424146785, 0, 0, 0, 0, " +
        "1280, 960, 0, 1, '1', " + to_string(albumId) + ")"); // cam, pic, shootingmode = 1
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

static void RemoveAssetsPrepare(int32_t& albumId, string& uri)
{
    // 1、创建用户相册
    CreateUserAlbum(g_albumName);
    vector<string> columns;
    auto resultSet = QueryAsset(PhotoAlbumColumns::TABLE, PhotoAlbumColumns::ALBUM_NAME, g_albumName, columns);
    if (resultSet == nullptr) {
        return;
    }

    albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
    if (albumId <= 0) {
        return;
    }

    // 2、插入一条数据照片到相册
    InsertAssetIntoPhotosTable(g_data, g_title, albumId);
    resultSet = QueryAsset(PhotoColumn::PHOTOS_TABLE, MediaColumn::MEDIA_NAME, g_title + ".jpg", columns);
    if (resultSet == nullptr) {
        return;
    }

    string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    if (displayName.size() <= 0) {
        return;
    }

    int32_t fileId = GetInt32Val(PhotoColumn::MEDIA_ID, resultSet);
    string path = GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet);
    uri = MediaFileUri::GetPhotoUri(to_string(fileId), path, displayName);
}

// {"albumId": "13", "albumType": "0","albumSubType": "1",
// "assetsArray": " [file://media/Photo/5/IMG_1748424853_004/IMG_20250528_171337.jpg]}
// (albumType == PhotoAlbumType::USER) && (albumSubType == PhotoAlbumSubType::USER_GENERIC);
static int32_t RemoveAssets(int32_t albumId, int32_t albumType, int32_t albumSubType,
    const std::vector<std::string> &assetsArray)
{
    AlbumRemoveAssetsReqBody reqBody;
    reqBody.albumId = albumId;
    reqBody.albumType = albumType;
    reqBody.albumSubType = albumSubType;
    reqBody.assetsArray = assetsArray;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAlbumsControllerService>();
    service->AlbumRemoveAssets(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> resp;
    if (resp.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("resp.Unmarshalling failed");
        return -1;
    }

    return resp.GetErrCode();
}

HWTEST_F(AlbumRemoveAssetsTest, RemoveAssets_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start RemoveAssets_Test_001");
    // 1、前置条件准备
    int32_t albumId = -1;
    string uri  = "";
    RemoveAssetsPrepare(albumId, uri);
    EXPECT_GT(albumId, 0);
    EXPECT_FALSE(uri.empty());

    // 2、删除照片
    vector<string> assetsArray = { uri };
    int32_t changedRows = RemoveAssets(albumId, PhotoAlbumType::USER, PhotoAlbumSubType::USER_GENERIC, {});
    EXPECT_EQ(changedRows, E_INVALID_VALUES);

    changedRows = RemoveAssets(albumId, PhotoAlbumType::SYSTEM, PhotoAlbumSubType::USER_GENERIC, assetsArray);
    EXPECT_EQ(changedRows, E_INVALID_VALUES);

    changedRows = RemoveAssets(albumId, PhotoAlbumType::USER, PhotoAlbumSubType::SYSTEM_START, assetsArray);
    EXPECT_EQ(changedRows, E_INVALID_VALUES);

    changedRows = RemoveAssets(albumId, PhotoAlbumType::USER, PhotoAlbumSubType::USER_GENERIC, assetsArray);
    EXPECT_GT(changedRows, 0);
    MEDIA_INFO_LOG("end RemoveAssets_Test_001");
}

HWTEST_F(AlbumRemoveAssetsTest, RemoveAssets_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start RemoveAssets_Test_002");
    MessageParcel data;
    MessageParcel reply;
    auto service = make_shared<MediaAlbumsControllerService>();
    service->AlbumRemoveAssets(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> resp;
    ASSERT_EQ(resp.Unmarshalling(reply), true);
    ASSERT_LT(resp.GetErrCode(), 0);
    MEDIA_INFO_LOG("end RemoveAssets_Test_002");
}
}  // namespace OHOS::Media