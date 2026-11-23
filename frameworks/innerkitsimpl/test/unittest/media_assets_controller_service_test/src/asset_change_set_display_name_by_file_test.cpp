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

#define MLOG_TAG "MediaAssetsControllerServiceTest"

#include "asset_change_set_display_name_by_file_test.h"

#include <string>
#include <vector>

#include "datashare_result_set.h"
#include "media_assets_controller_service.h"
#include "media_assets_service.h"
#include "rdb_utils.h"

#include "asset_change_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "result_set_utils.h"
#include "media_file_uri.h"
#include "test_data_builder.h"
#include "media_upgrade.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace IPC;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;

static int32_t g_userAlbumId;
static int32_t g_sourceAlbumId;
static int32_t g_fileManagerAlbumId;
static int32_t g_userAlbumAssetId;
static int32_t g_sourceAlbumAssetId;
static int32_t g_fileManagerAlbumAssetId;
static std::string g_fileManagerStoragePath;

static std::vector<std::string> createTableSqlLists = {
    PhotoAlbumColumns::CREATE_TABLE,
    PhotoUpgrade::CREATE_PHOTO_TABLE,
};

static std::vector<std::string> testTables = {
    PhotoAlbumColumns::TABLE,
    PhotoColumn::PHOTOS_TABLE,
};

void AssetChangeSetDisplayNameByFileTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start AssetChangeSetDisplayNameByFileTest failed, can not get g_rdbStore");
        exit(1);
    }
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);

    auto& builder = TestDataBuilder::GetInstance();
    builder.Init(g_rdbStore);
    builder.ClearAllTables();

    g_userAlbumId = builder.CreateAlbum(TestAlbumType::USER_ALBUM, "TestUserAlbum");
    g_sourceAlbumId = builder.CreateAlbum(TestAlbumType::SOURCE_ALBUM, "TestSourceAlbum");
    g_fileManagerAlbumId = builder.CreateAlbum(TestAlbumType::FILE_MANAGER_ALBUM, "TestFileManagerAlbum");
    ASSERT_GT(g_userAlbumId, 0);
    ASSERT_GT(g_sourceAlbumId, 0);
    ASSERT_GT(g_fileManagerAlbumId, 0);

    g_userAlbumAssetId = builder.CreateAsset(g_userAlbumId, "user_asset");
    g_sourceAlbumAssetId = builder.CreateAsset(g_sourceAlbumId, "source_asset");
    
    g_fileManagerStoragePath = "/storage/media/local/files/Docs/test/filemanager_asset.jpg";
    g_fileManagerAlbumAssetId = builder.CreateAssetWithStoragePath(g_fileManagerAlbumId, "filemanager_asset",
        g_fileManagerStoragePath);
    ASSERT_GT(g_userAlbumAssetId, 0);
    ASSERT_GT(g_sourceAlbumAssetId, 0);
    ASSERT_GT(g_fileManagerAlbumAssetId, 0);

    system("mkdir -p /storage/media/local/files/Docs/test");
    system("touch /storage/media/local/files/Docs/test/filemanager_asset.jpg");

    MEDIA_INFO_LOG("SetUpTestCase: userAlbumId=%{public}d, sourceAlbumId=%{public}d, fileManagerAlbumId=%{public}d",
        g_userAlbumId, g_sourceAlbumId, g_fileManagerAlbumId);
}

void AssetChangeSetDisplayNameByFileTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    system("rm -rf /storage/media/local/files/Docs/test");
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void AssetChangeSetDisplayNameByFileTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void AssetChangeSetDisplayNameByFileTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

static shared_ptr<NativeRdb::ResultSet> QueryAssetById(int32_t assetId, const vector<string>& columns)
{
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(MediaColumn::MEDIA_ID, std::to_string(assetId));
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get file asset, assetId: %{public}d", assetId);
        return nullptr;
    }
    return resultSet;
}

static int32_t SetDisplayNameByFile(const string& uri, const string& displayName)
{
    AssetChangeReqBody reqBody;
    reqBody.uri = uri;
    reqBody.displayName = displayName;

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);

    auto service = make_shared<MediaAssetsControllerService>();
    service->AssetChangeSetDisplayNameByFile(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> resp;
    if (resp.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("resp.Unmarshalling failed");
        return -1;
    }

    return resp.GetErrCode();
}

static string GetAssetUri(int32_t assetId)
{
    vector<string> columns = { MediaColumn::MEDIA_NAME, MediaColumn::MEDIA_FILE_PATH };
    auto resultSet = QueryAssetById(assetId, columns);
    if (resultSet == nullptr) {
        return "";
    }
    string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    string path = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    return MediaFileUri::GetPhotoUri(to_string(assetId), path, displayName);
}

static string GetAssetDisplayName(int32_t assetId)
{
    vector<string> columns = { MediaColumn::MEDIA_NAME };
    auto resultSet = QueryAssetById(assetId, columns);
    if (resultSet == nullptr) {
        return "";
    }
    return GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
}

HWTEST_F(AssetChangeSetDisplayNameByFileTest, SetDisplayNameByFile_UserAlbum_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetDisplayNameByFile_UserAlbum_001");
    
    string uri = GetAssetUri(g_userAlbumAssetId);
    ASSERT_FALSE(uri.empty());

    int32_t ret = SetDisplayNameByFile(uri, "new_user_asset_name.jpg");
    EXPECT_EQ(ret, E_OK);

    string newDisplayName = GetAssetDisplayName(g_userAlbumAssetId);
    EXPECT_EQ(newDisplayName, "new_user_asset_name.jpg");

    MEDIA_INFO_LOG("end SetDisplayNameByFile_UserAlbum_001");
}

HWTEST_F(AssetChangeSetDisplayNameByFileTest, SetDisplayNameByFile_SourceAlbum_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetDisplayNameByFile_SourceAlbum_001");
    
    string uri = GetAssetUri(g_sourceAlbumAssetId);
    ASSERT_FALSE(uri.empty());

    int32_t ret = SetDisplayNameByFile(uri, "new_source_asset_name.jpg");
    EXPECT_EQ(ret, E_OK);

    string newDisplayName = GetAssetDisplayName(g_sourceAlbumAssetId);
    EXPECT_EQ(newDisplayName, "new_source_asset_name.jpg");

    MEDIA_INFO_LOG("end SetDisplayNameByFile_SourceAlbum_001");
}

HWTEST_F(AssetChangeSetDisplayNameByFileTest, SetDisplayNameByFile_FileManagerAlbum_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetDisplayNameByFile_FileManagerAlbum_001");
    
    auto& builder = TestDataBuilder::GetInstance();
    builder.CreatePhysicalFile(g_fileManagerStoragePath);

    string uri = GetAssetUri(g_fileManagerAlbumAssetId);
    ASSERT_FALSE(uri.empty());

    string newDisplayName = "new_filemanager_asset_name.jpg";
    int32_t ret = SetDisplayNameByFile(uri, newDisplayName);
    EXPECT_EQ(ret, E_OK);

    string actualDisplayName = GetAssetDisplayName(g_fileManagerAlbumAssetId);
    EXPECT_EQ(actualDisplayName, newDisplayName);

    string newStoragePath = "/storage/media/local/files/Docs/test/" + newDisplayName;
    bool fileExists = builder.IsPhysicalFileExists(newStoragePath);
    EXPECT_TRUE(fileExists);

    if (fileExists) {
        builder.DeletePhysicalFile(newStoragePath);
    }

    MEDIA_INFO_LOG("end SetDisplayNameByFile_FileManagerAlbum_001");
}

HWTEST_F(AssetChangeSetDisplayNameByFileTest, SetDisplayNameByFile_ValidName_Dots_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetDisplayNameByFile_ValidName_Dots_001");

    string uri = GetAssetUri(g_userAlbumAssetId);
    ASSERT_FALSE(uri.empty());

    int32_t ret = SetDisplayNameByFile(uri, "test..name.jpg");
    EXPECT_EQ(ret, E_OK);

    ret = SetDisplayNameByFile(uri, "test...name.jpg");
    EXPECT_EQ(ret, E_OK);

    MEDIA_INFO_LOG("end SetDisplayNameByFile_ValidName_Dots_001");
}

HWTEST_F(AssetChangeSetDisplayNameByFileTest, SetDisplayNameByFile_InvalidName_OnlyDots_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetDisplayNameByFile_InvalidName_OnlyDots_001");
    
    string uri = GetAssetUri(g_userAlbumAssetId);
    ASSERT_FALSE(uri.empty());

    int32_t ret = SetDisplayNameByFile(uri, ".");
    EXPECT_LT(ret, 0);

    ret = SetDisplayNameByFile(uri, "..");
    EXPECT_LT(ret, 0);

    MEDIA_INFO_LOG("end SetDisplayNameByFile_InvalidName_OnlyDots_001");
}

HWTEST_F(AssetChangeSetDisplayNameByFileTest, SetDisplayNameByFile_InvalidUri_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetDisplayNameByFile_InvalidUri_001");
    
    string largeString(PATH_MAX + 4, 'a');
    int32_t ret = SetDisplayNameByFile(largeString, "test_name.jpg");
    EXPECT_LT(ret, 0);

    ret = SetDisplayNameByFile("", "test_name.jpg");
    EXPECT_LT(ret, 0);

    ret = SetDisplayNameByFile("invalid_uri", "test_name.jpg");
    EXPECT_LT(ret, 0);

    MEDIA_INFO_LOG("end SetDisplayNameByFile_InvalidUri_001");
}

HWTEST_F(AssetChangeSetDisplayNameByFileTest, SetDisplayNameByFile_EmptyName_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetDisplayNameByFile_EmptyName_001");
    
    string uri = GetAssetUri(g_userAlbumAssetId);
    ASSERT_FALSE(uri.empty());

    int32_t ret = SetDisplayNameByFile(uri, "");
    EXPECT_LT(ret, 0);

    MEDIA_INFO_LOG("end SetDisplayNameByFile_EmptyName_001");
}

HWTEST_F(AssetChangeSetDisplayNameByFileTest, SetDisplayNameByFile_EmptyParcel_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetDisplayNameByFile_EmptyParcel_001");
    
    MessageParcel data;
    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->AssetChangeSetDisplayNameByFile(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> resp;
    ASSERT_EQ(resp.Unmarshalling(reply), true);
    ASSERT_LT(resp.GetErrCode(), 0);

    MEDIA_INFO_LOG("end SetDisplayNameByFile_EmptyParcel_001");
}
}  // namespace OHOS::Media
