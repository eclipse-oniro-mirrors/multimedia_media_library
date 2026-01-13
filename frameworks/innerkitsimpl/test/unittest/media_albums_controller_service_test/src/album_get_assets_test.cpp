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

#include "album_get_assets_test.h"

#include <string>
#include <vector>

#define private public
#define protected public
#include "media_albums_controller_service.h"
#undef private
#undef protected

#include "album_get_assets_vo.h"
#include "album_get_assets_dto.h"
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

static std::vector<std::string> createTableSqlLists = {
    PhotoAlbumColumns::CREATE_TABLE,
    PhotoUpgrade::CREATE_PHOTO_TABLE,
};

static std::vector<std::string> testTables = {
    PhotoAlbumColumns::TABLE,
    PhotoColumn::PHOTOS_TABLE,
};

void AlbumGetAssetsTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    MEDIA_INFO_LOG("AlbumGetAssetsTest SetUpTestCase succeed");
}

void AlbumGetAssetsTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("AlbumGetAssetsTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void AlbumGetAssetsTest::SetUp()
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
    MEDIA_INFO_LOG("AlbumGetAssetsTest SetUp");
}

void AlbumGetAssetsTest::TearDown(void) {}

static const string SQL_CREATE_ALBUM = "INSERT INTO " + PhotoAlbumColumns::TABLE + "(" + PhotoAlbumColumns::ALBUM_TYPE +
                                       ", " + PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME +
                                       ", " + PhotoAlbumColumns::ALBUM_DATE_MODIFIED + ", " +
                                       PhotoAlbumColumns::ALBUM_IS_LOCAL + ", " + PhotoAlbumColumns::ALBUM_DATE_ADDED +
                                       ", " + PhotoAlbumColumns::ALBUM_LPATH + ", " +
                                       PhotoAlbumColumns::ALBUM_PRIORITY + ")";

static void CreateUserAlbum(const std::string &albumName)
{
    // album_type, album_subtype, album_name, date_modified, is_local, date_added, lpath, priority
    g_rdbStore->ExecuteSql(SQL_CREATE_ALBUM + "VALUES (0, 1, '" + albumName +
                           "', 1748354341383, 1 , 1748354341383, '/Pictures/Users/" + albumName + "', 1)");
}

static const string SQL_INSERT_PHOTO =
    "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" + MediaColumn::MEDIA_FILE_PATH + ", " + MediaColumn::MEDIA_SIZE +
    ", " + MediaColumn::MEDIA_TITLE + ", " + MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_TYPE + ", " +
    MediaColumn::MEDIA_OWNER_PACKAGE + ", " + MediaColumn::MEDIA_PACKAGE_NAME + ", " + MediaColumn::MEDIA_DATE_ADDED +
    ", " + MediaColumn::MEDIA_DATE_MODIFIED + ", " + MediaColumn::MEDIA_DATE_TAKEN + ", " +
    MediaColumn::MEDIA_DURATION + ", " + MediaColumn::MEDIA_IS_FAV + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
    MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_HEIGHT + ", " + PhotoColumn::PHOTO_WIDTH + ", " +
    PhotoColumn::PHOTO_EDIT_TIME + ", " + PhotoColumn::PHOTO_POSITION + ", " + PhotoColumn::PHOTO_SHOOTING_MODE + ", " +
    PhotoColumn::PHOTO_OWNER_ALBUM_ID + ")";

static void InsertAssetIntoPhotosTable(const string &data, const string &title, int32_t albumId)
{
    // data, size, title, display_name, media_type,position
    // owner_package, package_name, date_added, date_modified, date_taken, duration, is_favorite, date_trashed, hidden
    // height, width, edit_time, position, shooting_mode, owner_album_id
    g_rdbStore->ExecuteSql(
        SQL_INSERT_PHOTO + "VALUES ('" + data + "', 175258, '" + title + "', '" + title +
        ".jpg', 1, 'com.ohos.camera', '相机', 1748423617814, 1748424146785, 1748424146785, 0, 0, 0, 0, " +
        "1280, 960, 0, 1, '1', " + to_string(albumId) + ")");  // cam, pic, shootingmode = 1
}

static shared_ptr<NativeRdb::ResultSet> QueryAsset(
    const string &table, const string &key, const string &value, const vector<string> &columns)
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

static void AlbumGetAssetsPrepare(int32_t &albumId, string &uri)
{
    // 1、创建用户相册
    string albumName = "test01";
    CreateUserAlbum(albumName);
    vector<string> columns;
    auto resultSet = QueryAsset(PhotoAlbumColumns::TABLE, PhotoAlbumColumns::ALBUM_NAME, albumName, columns);
    if (resultSet == nullptr) {
        return;
    }

    albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
    if (albumId <= 0) {
        return;
    }

    // 2、插入一条数据照片到相册
    string title = "cam_pic";
    string data = "/storage/cloud/files/Photo/9/IMG_1748505946_009.jpg";
    InsertAssetIntoPhotosTable(data, title, albumId);
    resultSet = QueryAsset(PhotoColumn::PHOTOS_TABLE, MediaColumn::MEDIA_NAME, title + ".jpg", columns);
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

static int32_t AlbumGetAssets(string albumId, int32_t byPassCode = E_SUCCESS)
{
    AlbumGetAssetsReqBody reqBody;
    reqBody.predicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, albumId);

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    auto service = make_shared<MediaAlbumsControllerService>();
    MessageParcel reply;
    MessageOption option;
    IPCContext context(option, byPassCode);
    service->AlbumGetAssets(data, reply, context);

    IPC::MediaRespVo<AlbumGetAssetsRespBody> resp;
    if (resp.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("resp.Unmarshalling failed");
        return -1;
    }
    auto err = resp.GetErrCode();
    if (err != E_OK) {
        MEDIA_ERR_LOG("resp.GetErrCode is not E_OK");
        return -1;
    }
    AlbumGetAssetsRespBody respBody = resp.GetBody();
    if (respBody.resultSet == nullptr) {
        MEDIA_ERR_LOG("respBody.resultSet is nullptr");
        return -1;
    }
    int count = -1;

    auto errCode = respBody.resultSet->GetRowCount(count);
    if (errCode != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("ResultSet GetRowCount failed, errCode=%{public}d", errCode);
        return -1;
    }
    return count;
}

HWTEST_F(AlbumGetAssetsTest, GetAssets_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetAssets_Test_001");
    // 1、前置条件准备
    int32_t albumId = -1;
    string uri = "";
    AlbumGetAssetsPrepare(albumId, uri);
    EXPECT_GT(albumId, 0);
    EXPECT_FALSE(uri.empty());

    // 2、获取相册资产
    int32_t count = AlbumGetAssets(to_string(albumId));
    EXPECT_GT(count, 0);

    count = AlbumGetAssets("invalid_album_id");
    EXPECT_EQ(count, 0);

    count = AlbumGetAssets(to_string(albumId), E_PERMISSION_DB_BYPASS);
    EXPECT_LT(count, 0);

    count = AlbumGetAssets("invalid_album_id", E_PERMISSION_DB_BYPASS);
    EXPECT_LT(count, 0);

    MEDIA_INFO_LOG("end GetAssets_Test_001");
}
}  // namespace OHOS::Media