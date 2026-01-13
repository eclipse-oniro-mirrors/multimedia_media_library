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

#define MLOG_TAG "MediaAlbumControllerServiceTest"

#include "album_delete_assets_test.h"

#include <string>
#include <vector>

#define private public
#define protected public
#include "media_assets_controller_service.h"
#undef private
#undef protected

#include "delete_photos_vo.h"
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

void AlbumDeleteAssetsTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    MEDIA_INFO_LOG("AlbumDeleteAssetsTest SetUpTestCase succeed");
}

void AlbumDeleteAssetsTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("AlbumDeleteAssetsTest TearDownTestCase done");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void AlbumDeleteAssetsTest::SetUp()
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
    MEDIA_INFO_LOG("AlbumDeleteAssetsTest SetUp");
}

void AlbumDeleteAssetsTest::TearDown(void) {}

static const string SQL_INSERT_PHOTO = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" +
    MediaColumn::MEDIA_FILE_PATH + ", " + MediaColumn::MEDIA_SIZE + ", " + MediaColumn::MEDIA_TITLE + ", " +
    MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_TYPE + ", " + MediaColumn::MEDIA_OWNER_PACKAGE + ", " +
    MediaColumn::MEDIA_PACKAGE_NAME + ", " + MediaColumn::MEDIA_DATE_ADDED + ", "  +
    MediaColumn::MEDIA_DATE_MODIFIED + ", " + MediaColumn::MEDIA_DATE_TAKEN + ", " +
    MediaColumn::MEDIA_DURATION + ", " + MediaColumn::MEDIA_IS_FAV + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
    MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_HEIGHT + ", " + PhotoColumn::PHOTO_WIDTH + ", " +
    PhotoColumn::PHOTO_EDIT_TIME + ", " + PhotoColumn::PHOTO_SHOOTING_MODE + ")";

static void InsertTrashAssetIntoPhotosTable(const string& data, const string& title)
{
    // data, size, title, display_name, media_type,
    // owner_package, package_name, date_added, date_modified, date_taken, duration, is_favorite, date_trashed, hidden
    // height, width, edit_time, shooting_mode
    g_rdbStore->ExecuteSql(SQL_INSERT_PHOTO + "VALUES ('" + data + "', 175258, '" + title + "', '" +
        title + ".jpg', 1, 'com.ohos.camera', '相机', 1748423617814, 1748424146785, 1748423617706, " +
        "0, 0, 1748488395008, 0, 1280, 960, 0, '1' )"); // cam, pic, shootingmode = 1
}

static shared_ptr<NativeRdb::ResultSet> QueryAsset(const string& displayName, const vector<string>& columns)
{
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(MediaColumn::MEDIA_NAME, displayName);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get file asset");
        return nullptr;
    }
    return resultSet;
}

// {[file://media/Photo/1/IMG_1748423699_000/IMG_20250528_171318.jpg]}
static int32_t DeleteAssets(const std::vector<std::string> &uris)
{
    DeletePhotosReqBody reqBody;
    reqBody.uris = uris;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->DeletePhotos(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> resp;
    if (resp.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("resp.Unmarshalling failed");
        return -1;
    }

    return resp.GetErrCode();
}

HWTEST_F(AlbumDeleteAssetsTest, DeleteAssets_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start DeleteAssets_Test_001");
    // 1、前置条件准备，插入一条删除的照片到数据库
    string data = "/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg";
    string title = "cam_pic";
    InsertTrashAssetIntoPhotosTable(data, title);

    // 2、查询数据是否插入成功
    vector<string> columns;
    auto resultSet = QueryAsset(title + ".jpg", columns);
    ASSERT_NE(resultSet, nullptr);
    string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    EXPECT_GT(displayName.size(), 0);

    int32_t fileId = GetInt32Val(PhotoColumn::MEDIA_ID, resultSet);
    string path = GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet);
    string uri = MediaFileUri::GetPhotoUri(to_string(fileId), path, displayName);

    // 3、从回收站彻底删除照片
    int32_t changedRows = DeleteAssets({});
    EXPECT_EQ(changedRows, -EINVAL);

    changedRows = DeleteAssets({ uri });
    EXPECT_GT(changedRows, 0);
    MEDIA_INFO_LOG("end DeleteAssets_Test_001");
}

HWTEST_F(AlbumDeleteAssetsTest, DeleteAssets_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start DeleteAssets_Test_002");
    MessageParcel data;
    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->DeletePhotos(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> resp;
    ASSERT_EQ(resp.Unmarshalling(reply), true);
    ASSERT_LT(resp.GetErrCode(), 0);
    MEDIA_INFO_LOG("end DeleteAssets_Test_002");
}
}  // namespace OHOS::Media