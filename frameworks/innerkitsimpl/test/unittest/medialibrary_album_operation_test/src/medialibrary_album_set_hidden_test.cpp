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
#define MLOG_TAG "MediaLibraryAlbumOperationTest"

#include "medialibrary_album_operation_test.h"
#include "medialibrary_album_set_hidden_test.h"
#include "datashare_result_set.h"
#include "photo_album_column.h"
#include "get_self_permissions.h"
#include "location_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_album_operations.h"
#include "medialibrary_errno.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "test_data_builder.h"
#include "result_set_utils.h"
#include "uri.h"
#include "vision_db_sqls_more.h"
#include "vision_portrait_nickname_column.h"
#include "album_operation_uri.h"
#include "asset_accurate_refresh.h"
#include "media_upgrade.h"
#include "analysis_album_operation_data_utils.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media::AccurateRefresh;

namespace OHOS {
namespace Media {
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
constexpr int32_t WAIT_TIME = 3;

void CovCleanTestTables()
{
    vector<string> cleanTableList = {
        PhotoColumn::PHOTOS_TABLE,
        ANALYSIS_ALBUM_TABLE,
        ANALYSIS_PHOTO_MAP_TABLE,
        ANALYSIS_NICK_NAME_TABLE,
    };
    for (auto &cleanTable : cleanTableList) {
        string deleteSql = "DELETE FROM " + cleanTable + ";";
        int32_t ret = g_rdbStore->ExecuteSql(deleteSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Delete %{public}s table failed", cleanTable.c_str());
            return;
        }
        string seqSql = "UPDATE sqlite_sequence SET seq = 0 WHERE name = '" + cleanTable + "';";
        int32_t seqRet = g_rdbStore->ExecuteSql(seqSql);
        if (seqRet != NativeRdb::E_OK) {
            MEDIA_DEBUG_LOG("Reset %{public}s sqlite_sequence failed, ret=%{public}d", cleanTable.c_str(), seqRet);
        }
        MEDIA_DEBUG_LOG("Delete %{public}s table success", cleanTable.c_str());
    }
}

void CovSetTables()
{
    g_rdbStore->ExecuteSql("DROP TABLE IF EXISTS " + PhotoAlbumColumns::TABLE + ";");
    vector<string> createTableSqlList = {
        PhotoAlbumColumns::CREATE_TABLE,
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        CREATE_ANALYSIS_ALBUM_FOR_ONCREATE,
        CREATE_ANALYSIS_ALBUM_MAP,
        CREATE_ANALYSIS_NICK_NAME_TABLE,
        CREATE_ANALYSIS_NICK_NAME_UNIQUE_INDEX,
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

void CovClearAndRestart()
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
    CovCleanTestTables();
    CovSetTables();
}

void CovClearAnalysisAlbum()
{
    auto rdbStore = MediaLibraryDataManager::GetInstance()->rdbStore_;
    NativeRdb::AbsRdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    predicates.NotEqualTo(ALBUM_SUBTYPE, PhotoAlbumSubType::SHOOTING_MODE);
    int32_t deletedRows = -1;
    auto ret = rdbStore->Delete(deletedRows, predicates);
    MEDIA_INFO_LOG("CovClearAnalysisAlbum Delete retVal: %{public}d, deletedRows: %{public}d", ret, deletedRows);
}

void MediaLibraryAlbumSetHiddenTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("AlbumOperationSetHidden::Start");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryAlbumSetHiddenTest failed, can not get rdbstore");
        exit(1);
    }
    vector<string> perms = { "ohos.permission.MEDIA_LOCATION" };
    uint64_t tokenId = 0;
    PermissionUtilsUnitTest::SetAccessTokenPermission("MediaLibraryAlbumSetHiddenTest", perms, tokenId);
    ASSERT_TRUE(tokenId != 0);
    CovClearAnalysisAlbum();
    CovClearAndRestart();
}

void MediaLibraryAlbumSetHiddenTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("AlbumOperationSetHidden::End");
    std::this_thread::sleep_for(std::chrono::seconds(WAIT_TIME));
}

void MediaLibraryAlbumSetHiddenTest::SetUp(void)
{
    MEDIA_INFO_LOG("SetUp");
    MediaLibraryUnitTestUtils::CleanTestFiles();
    MediaLibraryUnitTestUtils::CleanBundlePermission();
    MediaLibraryUnitTestUtils::InitRootDirs();
    MediaLibraryUnitTestUtils::Init();
    CovClearAnalysisAlbum();
    CovClearAndRestart();
}

void MediaLibraryAlbumSetHiddenTest::TearDown(void) {}

static void DropPhotoAlbumTableIfExists()
{
    if (g_rdbStore == nullptr) {
        return;
    }
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, { PhotoAlbumColumns::CREATE_TABLE });
    string deleteSql = "DELETE FROM " + PhotoAlbumColumns::TABLE + ";";
    int32_t ret = g_rdbStore->ExecuteSql(deleteSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Delete %{public}s table failed", PhotoAlbumColumns::TABLE.c_str());
        return;
    }
    string seqSql = "UPDATE sqlite_sequence SET seq = 0 WHERE name = '" + PhotoAlbumColumns::TABLE + "';";
    int32_t seqRet = g_rdbStore->ExecuteSql(seqSql);
    if (seqRet != NativeRdb::E_OK) {
        MEDIA_DEBUG_LOG("Reset %{public}s sqlite_sequence failed, ret=%{public}d",
            PhotoAlbumColumns::TABLE.c_str(), seqRet);
    }
}

static void EnsurePhotoAlbumTable()
{
    DropPhotoAlbumTableIfExists();
}

static int32_t GetAlbumFileHiddenAttr(int32_t albumId)
{
    string sql = "SELECT " + string(PhotoAlbumColumns::ALBUM_FILE_HIDDEN) + " FROM " +
        PhotoAlbumColumns::TABLE + " WHERE " + PhotoAlbumColumns::ALBUM_ID + " = ?";
    vector<string> args = { to_string(albumId) };
    auto resultSet = g_rdbStore->QuerySql(sql, args);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return -1;
    }
    return GetInt32Val(PhotoAlbumColumns::ALBUM_FILE_HIDDEN, resultSet);
}

static int32_t GetAssetFileHiddenAttr(int32_t assetId)
{
    string sql = "SELECT " + string(PhotoColumn::PHOTO_FILE_HIDDEN) + " FROM " +
        PhotoColumn::PHOTOS_TABLE + " WHERE " + MediaColumn::MEDIA_ID + " = ?";
    vector<string> args = { to_string(assetId) };
    auto resultSet = g_rdbStore->QuerySql(sql, args);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return -1;
    }
    return GetInt32Val(PhotoColumn::PHOTO_FILE_HIDDEN, resultSet);
}

HWTEST_F(MediaLibraryAlbumSetHiddenTest, AlbumChangeSetHiddenAttribute_AlbumIdZero, TestSize.Level1)
{
    MEDIA_INFO_LOG("AlbumChangeSetHiddenAttribute_AlbumIdZero::Start");
    int32_t ret = MediaLibraryAlbumOperations::AlbumChangeSetHiddenAttribute(0, true, false);
    EXPECT_EQ(ret, NativeRdb::E_INVALID_ARGS);
    MEDIA_INFO_LOG("AlbumChangeSetHiddenAttribute_AlbumIdZero End");
}

HWTEST_F(MediaLibraryAlbumSetHiddenTest, AlbumChangeSetHiddenAttribute_AlbumIdNegative, TestSize.Level1)
{
    MEDIA_INFO_LOG("AlbumChangeSetHiddenAttribute_AlbumIdNegative::Start");
    int32_t ret = MediaLibraryAlbumOperations::AlbumChangeSetHiddenAttribute(-1, true, true);
    EXPECT_EQ(ret, NativeRdb::E_INVALID_ARGS);
    MEDIA_INFO_LOG("AlbumChangeSetHiddenAttribute_AlbumIdNegative End");
}

HWTEST_F(MediaLibraryAlbumSetHiddenTest, AlbumChangeSetHiddenAttribute_AlbumNotExist, TestSize.Level1)
{
    MEDIA_INFO_LOG("AlbumChangeSetHiddenAttribute_AlbumNotExist::Start");
    DropPhotoAlbumTableIfExists();
    int32_t ret = MediaLibraryAlbumOperations::AlbumChangeSetHiddenAttribute(1, true, false);
    EXPECT_EQ(ret, E_OK);
    EnsurePhotoAlbumTable();
    MEDIA_INFO_LOG("AlbumChangeSetHiddenAttribute_AlbumNotExist End");
}

HWTEST_F(MediaLibraryAlbumSetHiddenTest, InheritedFalse_HiddenTrue, TestSize.Level1)
{
    MEDIA_INFO_LOG("AlbumChangeSetHiddenAttribute_InheritedFalse_HiddenTrue::Start");
    EnsurePhotoAlbumTable();
    auto& builder = TestDataBuilder::GetInstance();
    builder.Init(g_rdbStore);
    builder.ClearAllTables();
    int32_t albumId = builder.CreateAlbum(TestAlbumType::USER_ALBUM, "HiddenAttrUserAlbum");
    ASSERT_GT(albumId, 0);
    int32_t assetId = builder.CreateAsset(albumId, "hidden_attr_asset");
    ASSERT_GT(assetId, 0);

    int32_t ret = MediaLibraryAlbumOperations::AlbumChangeSetHiddenAttribute(albumId, true, false);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(GetAlbumFileHiddenAttr(albumId), 1);
    EXPECT_EQ(GetAssetFileHiddenAttr(assetId), 0);
    DropPhotoAlbumTableIfExists();
    MEDIA_INFO_LOG("AlbumChangeSetHiddenAttribute_InheritedFalse_HiddenTrue End");
}

HWTEST_F(MediaLibraryAlbumSetHiddenTest, InheritedFalse_HiddenFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("AlbumChangeSetHiddenAttribute_InheritedFalse_HiddenFalse::Start");
    EnsurePhotoAlbumTable();
    auto& builder = TestDataBuilder::GetInstance();
    builder.Init(g_rdbStore);
    builder.ClearAllTables();
    int32_t albumId = builder.CreateAlbum(TestAlbumType::USER_ALBUM, "HiddenAttrUserAlbum2");
    ASSERT_GT(albumId, 0);
    int32_t assetId = builder.CreateAsset(albumId, "hidden_attr_asset2");
    ASSERT_GT(assetId, 0);

    int32_t ret = MediaLibraryAlbumOperations::AlbumChangeSetHiddenAttribute(albumId, false, false);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(GetAlbumFileHiddenAttr(albumId), 0);
    EXPECT_EQ(GetAssetFileHiddenAttr(assetId), 0);
    DropPhotoAlbumTableIfExists();
    MEDIA_INFO_LOG("AlbumChangeSetHiddenAttribute_InheritedFalse_HiddenFalse End");
}

HWTEST_F(MediaLibraryAlbumSetHiddenTest, InheritedTrue_HiddenTrue, TestSize.Level1)
{
    MEDIA_INFO_LOG("AlbumChangeSetHiddenAttribute_InheritedTrue_HiddenTrue::Start");
    EnsurePhotoAlbumTable();
    auto& builder = TestDataBuilder::GetInstance();
    builder.Init(g_rdbStore);
    builder.ClearAllTables();
    int32_t albumId = builder.CreateAlbum(TestAlbumType::USER_ALBUM, "HiddenAttrUserAlbum3");
    ASSERT_GT(albumId, 0);
    int32_t assetId = builder.CreateAsset(albumId, "hidden_attr_asset3");
    ASSERT_GT(assetId, 0);

    int32_t ret = MediaLibraryAlbumOperations::AlbumChangeSetHiddenAttribute(albumId, true, true);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(GetAlbumFileHiddenAttr(albumId), 1);
    EXPECT_EQ(GetAssetFileHiddenAttr(assetId), 1);
    DropPhotoAlbumTableIfExists();
    MEDIA_INFO_LOG("AlbumChangeSetHiddenAttribute_InheritedTrue_HiddenTrue End");
}

HWTEST_F(MediaLibraryAlbumSetHiddenTest, InheritedTrue_HiddenFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("AlbumChangeSetHiddenAttribute_InheritedTrue_HiddenFalse::Start");
    EnsurePhotoAlbumTable();
    auto& builder = TestDataBuilder::GetInstance();
    builder.Init(g_rdbStore);
    builder.ClearAllTables();
    int32_t albumId = builder.CreateAlbum(TestAlbumType::USER_ALBUM, "HiddenAttrUserAlbum4");
    ASSERT_GT(albumId, 0);
    int32_t assetId = builder.CreateAsset(albumId, "hidden_attr_asset4");
    ASSERT_GT(assetId, 0);

    int32_t ret = MediaLibraryAlbumOperations::AlbumChangeSetHiddenAttribute(albumId, true, true);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(GetAlbumFileHiddenAttr(albumId), 1);
    EXPECT_EQ(GetAssetFileHiddenAttr(assetId), 1);
    ret = MediaLibraryAlbumOperations::AlbumChangeSetHiddenAttribute(albumId, false, true);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(GetAlbumFileHiddenAttr(albumId), 0);
    EXPECT_EQ(GetAssetFileHiddenAttr(assetId), 0);
    DropPhotoAlbumTableIfExists();
    MEDIA_INFO_LOG("AlbumChangeSetHiddenAttribute_InheritedTrue_HiddenFalse End");
}
}  // namespace Media
}  // namespace OHOS
