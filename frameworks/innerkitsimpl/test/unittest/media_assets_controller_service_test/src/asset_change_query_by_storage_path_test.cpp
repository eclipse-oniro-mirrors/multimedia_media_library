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

#include "asset_change_query_by_storage_path_test.h"

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

static std::vector<std::string> createTableSqlLists = {
    PhotoAlbumColumns::CREATE_TABLE,
    PhotoUpgrade::CREATE_PHOTO_TABLE,
};

static std::vector<std::string> testTables = {
    PhotoAlbumColumns::TABLE,
    PhotoColumn::PHOTOS_TABLE,
};

void AssetChangeQueryByStoragePathTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start AssetChangeQueryByStoragePathTest failed, can not get g_rdbStore");
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
    g_fileManagerAlbumAssetId = builder.CreateAssetWithStoragePath(g_fileManagerAlbumId, "filemanager_asset",
        "/storage/media/local/files/Docs/test/filemanager_asset.jpg");
    ASSERT_GT(g_userAlbumAssetId, 0);
    ASSERT_GT(g_sourceAlbumAssetId, 0);
    ASSERT_GT(g_fileManagerAlbumAssetId, 0);

    MEDIA_INFO_LOG("SetUpTestCase: userAlbumId=%{public}d, sourceAlbumId=%{public}d, fileManagerAlbumId=%{public}d",
        g_userAlbumId, g_sourceAlbumId, g_fileManagerAlbumId);
    MEDIA_INFO_LOG("SetUpTestCase: userAssetId=%{public}d, sourceAssetId=%{public}d, fileManagerAssetId=%{public}d",
        g_userAlbumAssetId, g_sourceAlbumAssetId, g_fileManagerAlbumAssetId);
}

void AssetChangeQueryByStoragePathTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, false);
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void AssetChangeQueryByStoragePathTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void AssetChangeQueryByStoragePathTest::TearDown(void)
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

HWTEST_F(AssetChangeQueryByStoragePathTest, QueryAssetByStoragePath_EmptyPath_ReturnsNull, TestSize.Level0)
{
    MEDIA_INFO_LOG("QueryAssetByStoragePath_EmptyPath_ReturnsNull enter");
    MediaAssetsRdbOperations rdbOperations;
    auto resultSet = rdbOperations.QueryAssetByStoragePath("");
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("QueryAssetByStoragePath_EmptyPath_ReturnsNull end");
}

HWTEST_F(AssetChangeQueryByStoragePathTest, QueryAssetByStoragePath_ExistingAsset, TestSize.Level0)
{
    MEDIA_INFO_LOG("QueryAssetByStoragePath_ExistingAsset enter");
    string path = "/storage/media/local/files/Docs/test/filemanager_asset.jpg";
    MediaAssetsRdbOperations rdbOperations;
    auto resultSet = rdbOperations.QueryAssetByStoragePath(path);
    ASSERT_NE(resultSet, nullptr);
    int32_t count = 0;
    EXPECT_EQ(resultSet->GetRowCount(count), NativeRdb::E_OK);
    EXPECT_GT(count, 0);
    MEDIA_INFO_LOG("QueryAssetByStoragePath_ExistingAsset end");
}
}  // namespace OHOS::Media
