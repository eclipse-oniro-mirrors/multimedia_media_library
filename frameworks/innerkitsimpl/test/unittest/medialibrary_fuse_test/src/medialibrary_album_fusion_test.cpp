/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "FuseUnitTest"

#include "medialibrary_album_fusion_test.h"

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <fstream>

#include "media_fuse_manager.h"
#include "medialibrary_unittest_utils.h"
#include "mimetype_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_app_uri_sensitive_operations.h"
#include "datashare_predicates_objects.h"
#include "medialibrary_appstate_observer.h"
#include "medialibrary_rdb_transaction.h"
#include "media_file_uri.h"
#include "rdb_utils.h"
#include "datashare_predicates.h"
#include "abs_rdb_predicates.h"
#include "fetch_result.h"
#include "file_asset.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_command.h"
#include "medialibrary_common_utils.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_db_const.h"
#include "medialibrary_inotify.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "thumbnail_const.h"
#include "uri.h"
#include "userfile_manager_types.h"
#include "values_bucket.h"
#include "photo_album_column.h"
#include "medialibrary_app_uri_permission_operations.h"
#include "permission_utils.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_object_utils.h"
#include "parameter.h"
#include "heif_transcoding_check_utils.h"
#include "media_upgrade.h"
#include "media_cloud_permission_check.h"
#include "medialibrary_album_fusion_utils.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using OHOS::DataShare::DataShareValuesBucket;
using OHOS::DataShare::DataSharePredicates;
using namespace OHOS::RdbDataShareAdapter;
using namespace OHOS::Security::AccessToken;
namespace OHOS {
namespace Media {

static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

static shared_ptr<MediaLibraryRdbStore> g_rdbStoreFusion;
unordered_map<string, bool> fuseTestPermsMapFusion = {
    { PERM_READ_IMAGEVIDEO, 1 },
    { PERM_WRITE_IMAGEVIDEO, 1 }
};

void CleanTestTablesFusion()
{
    vector<string> cleanTableList = {
        PhotoColumn::PHOTOS_TABLE,
        AppUriPermissionColumn::APP_URI_PERMISSION_TABLE,
        AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE,
    };
    for (auto &cleanTable : cleanTableList) {
        string deleteSql = "DELETE FROM " + cleanTable + ";";
        int32_t ret = g_rdbStoreFusion->ExecuteSql(deleteSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Delete %{public}s table failed", cleanTable.c_str());
            return;
        }
        string seqSql = "UPDATE sqlite_sequence SET seq = 0 WHERE name = '" + cleanTable + "';";
        int32_t seqRet = g_rdbStoreFusion->ExecuteSql(seqSql);
        if (seqRet != NativeRdb::E_OK) {
            MEDIA_DEBUG_LOG("Reset %{public}s sqlite_sequence failed, ret=%{public}d",
                cleanTable.c_str(), seqRet);
        }
        MEDIA_DEBUG_LOG("Delete %{public}s table success", cleanTable.c_str());
    }
}

struct UniqueMemberValuesBucket {
    string assetMediaType;
    int32_t startNumber;
};

void PrepareUniqueNumberTableFusion()
{
    if (g_rdbStoreFusion == nullptr) {
        MEDIA_ERR_LOG("can not get g_rdbstore");
        return;
    }
    auto store = g_rdbStoreFusion;
    if (store == nullptr) {
        MEDIA_ERR_LOG("can not get store");
        return;
    }
    string queryRowSql = "SELECT COUNT(*) as count FROM " + ASSET_UNIQUE_NUMBER_TABLE;
    auto resultSet = store->QuerySql(queryRowSql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get AssetUniqueNumberTable count");
        return;
    }
    if (GetInt32Val("count", resultSet) != 0) {
        MEDIA_DEBUG_LOG("AssetUniqueNumberTable is already inited");
        return;
    }

    UniqueMemberValuesBucket imageBucket = { CONST_IMAGE_ASSET_TYPE, 1 };
    UniqueMemberValuesBucket videoBucket = { CONST_VIDEO_ASSET_TYPE, 1 };
    UniqueMemberValuesBucket audioBucket = { CONST_AUDIO_ASSET_TYPE, 1 };

    vector<UniqueMemberValuesBucket> uniqueNumberValueBuckets = {
        imageBucket, videoBucket, audioBucket
    };

    for (const auto& uniqueNumberValueBucket : uniqueNumberValueBuckets) {
        ValuesBucket valuesBucket;
        valuesBucket.PutString(ASSET_MEDIA_TYPE, uniqueNumberValueBucket.assetMediaType);
        valuesBucket.PutInt(UNIQUE_NUMBER, uniqueNumberValueBucket.startNumber);
        int64_t outRowId = -1;
        int32_t insertResult = store->Insert(outRowId, ASSET_UNIQUE_NUMBER_TABLE, valuesBucket);
        if (insertResult != NativeRdb::E_OK || outRowId <= 0) {
            MEDIA_ERR_LOG("Prepare smartAlbum failed");
        }
    }
}

void SetTablesFusion()
{
    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        AppUriPermissionColumn::CREATE_APP_URI_PERMISSION_TABLE,
        AppUriSensitiveColumn::CREATE_APP_URI_SENSITIVE_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        if (g_rdbStoreFusion == nullptr) {
            MEDIA_ERR_LOG("can not get g_rdbstore");
            return;
        }
        int32_t ret = g_rdbStoreFusion->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
    PrepareUniqueNumberTableFusion();
}

void ClearAndRestartFusion()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }
    system("rm -rf /storage/cloud/files/*");
    system("rm -rf /storage/cloud/files/.thumbs");
    system("rm -rf /storage/cloud/files/.editData");
    system("rm -rf /storage/cloud/files/.cache");
    for (const auto &dir : TEST_ROOT_DIRS) {
        string ROOT_PATH = "/storage/cloud/100/files/";
        bool ret = MediaFileUtils::CreateDirectory(ROOT_PATH + dir + "/");
        CHECK_AND_PRINT_LOG(ret, "make %{public}s dir failed, ret=%{public}d", dir.c_str(), ret);
    }
    CleanTestTablesFusion();
    SetTablesFusion();
}

void MediaLibraryAlbumFusionTest::SetUpTestCase()
{
    MediaFuseManager::GetInstance();
    MediaLibraryUnitTestUtils::Init();
    g_rdbStoreFusion = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStoreFusion == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryAlbumFusionTest failed, can not get rdbstore");
        exit(1);
    }
    SetTablesFusion();
}

void MediaLibraryAlbumFusionTest::TearDownTestCase()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    ClearAndRestartFusion();
    g_rdbStoreFusion = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
    MEDIA_INFO_LOG("Clean is finish");
}

void MediaLibraryAlbumFusionTest::SetUp()
{
    if (g_rdbStoreFusion == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryAlbumFusionTest failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestartFusion();
}

void MediaLibraryAlbumFusionTest::TearDown() {}

string GetFilePathFusion(int fileId)
{
    if (fileId < 0) {
        MEDIA_ERR_LOG("this file id %{private}d is invalid", fileId);
        return "";
    }

    vector<string> columns = { PhotoColumn::MEDIA_FILE_PATH };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    if (g_rdbStoreFusion == nullptr) {
        MEDIA_ERR_LOG("can not get rdbstore");
        return "";
    }
    auto resultSet = g_rdbStoreFusion->Query(cmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get file Path");
        return "";
    }
    string path = GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet);
    return path;
}

int32_t MakePhotoUnpendingFusion(int fileId, bool isMovingPhoto = false)
{
    if (fileId < 0) {
        MEDIA_ERR_LOG("this file id %{private}d is invalid", fileId);
        return E_INVALID_FILEID;
    }

    string path = GetFilePathFusion(fileId);
    if (path.empty()) {
        MEDIA_ERR_LOG("Get path failed");
        return E_INVALID_VALUES;
    }
    int32_t errCode = MediaFileUtils::CreateAsset(path);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("Can not create asset");
        return errCode;
    }

    if (isMovingPhoto) {
        string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(path);
        errCode = MediaFileUtils::CreateAsset(videoPath);
        if (errCode != E_OK) {
            MEDIA_ERR_LOG("Can not create video asset");
            return errCode;
        }
    }

    if (g_rdbStoreFusion == nullptr) {
        MEDIA_ERR_LOG("can not get rdbstore");
        return E_HAS_DB_ERROR;
    }
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    ValuesBucket values;
    values.PutLong(PhotoColumn::MEDIA_TIME_PENDING, 0);
    cmd.SetValueBucket(values);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    int32_t changedRows = -1;
    errCode = g_rdbStoreFusion->Update(cmd, changedRows);
    if (errCode != E_OK || changedRows <= 0) {
        MEDIA_ERR_LOG("Update pending failed, errCode = %{public}d, changeRows = %{public}d",
            errCode, changedRows);
        return errCode;
    }

    return E_OK;
}

int32_t CreatePhotoApi10Fusion(int mediaType, const string &displayName)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, mediaType);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryPhotoOperations::Create(cmd);
    if (ret < 0) {
        MEDIA_ERR_LOG("Create Photo failed, errCode=%{public}d", ret);
        return ret;
    }

    int32_t errCode = MakePhotoUnpendingFusion(ret);
    if (errCode != E_OK) {
        return errCode;
    }
    return ret;
}

static shared_ptr<NativeRdb::ResultSet> QueryFusionAssetResultSetFusion(int32_t fileId)
{
    if (g_rdbStoreFusion == nullptr) {
        return nullptr;
    }
    MediaLibraryCommand queryCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    queryCmd.SetDataSharePred(predicates);
    vector<string> columns = {
        MediaColumn::MEDIA_ID, MediaColumn::MEDIA_NAME, MediaColumn::MEDIA_TYPE, MediaColumn::MEDIA_FILE_PATH,
        PhotoColumn::PHOTO_OWNER_ALBUM_ID, PhotoColumn::PHOTO_POSITION, PhotoColumn::PHOTO_IS_TEMP,
        MediaColumn::MEDIA_TIME_PENDING, MediaColumn::MEDIA_HIDDEN, MediaColumn::MEDIA_DATE_TRASHED,
        MediaColumn::MEDIA_DATE_DELETED, PhotoColumn::MEDIA_SIZE
    };
    return g_rdbStoreFusion->Query(queryCmd, columns);
}

static int32_t InsertBasePhotoFusion(const string &displayName)
{
    return CreatePhotoApi10Fusion(static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE), displayName);
}

static void SetPhotoFieldFusion(int32_t fileId, const string &column, int64_t value)
{
    if (g_rdbStoreFusion == nullptr || fileId < 0) {
        MEDIA_ERR_LOG("SetPhotoField invalid param");
        return;
    }
    string sql = "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + column + " = " + to_string(value) +
        " WHERE " + PhotoColumn::MEDIA_ID + " = " + to_string(fileId) + ";";
    int32_t ret = g_rdbStoreFusion->ExecuteSql(sql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("SetPhotoField failed, sql=%{private}s", sql.c_str());
    }
}

HWTEST_F(MediaLibraryAlbumFusionTest, MediaLibrary_AlbumFusion_CheckBatchAssets_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_AlbumFusion_CheckBatchAssets_Test_001");
    EXPECT_EQ(MediaLibraryAlbumFusionUtils::CheckBatchAssets(nullptr), E_DB_FAIL);
}

HWTEST_F(MediaLibraryAlbumFusionTest, MediaLibrary_AlbumFusion_CheckBatchAssets_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_AlbumFusion_CheckBatchAssets_Test_002");
    auto resultSet = QueryFusionAssetResultSetFusion(-99999);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(MediaLibraryAlbumFusionUtils::CheckBatchAssets(resultSet), E_OK);
}

HWTEST_F(MediaLibraryAlbumFusionTest, MediaLibrary_AlbumFusion_CheckBatchAssets_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_AlbumFusion_CheckBatchAssets_Test_003");
    int32_t fileId = InsertBasePhotoFusion("fuse_normal.jpg");
    ASSERT_GT(fileId, 0);
    auto resultSet = QueryFusionAssetResultSetFusion(fileId);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(MediaLibraryAlbumFusionUtils::CheckBatchAssets(resultSet), E_OK);
}

HWTEST_F(MediaLibraryAlbumFusionTest, MediaLibrary_AlbumFusion_CheckBatchAssets_Test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_AlbumFusion_CheckBatchAssets_Test_004");
    int32_t fileId = InsertBasePhotoFusion("fuse_cloud.jpg");
    ASSERT_GT(fileId, 0);
    SetPhotoFieldFusion(fileId, PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::CLOUD));
    auto resultSet = QueryFusionAssetResultSetFusion(fileId);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(MediaLibraryAlbumFusionUtils::CheckBatchAssets(resultSet), E_SCENE_IS_CLOUD);
}

HWTEST_F(MediaLibraryAlbumFusionTest, MediaLibrary_AlbumFusion_CheckBatchAssets_Test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_AlbumFusion_CheckBatchAssets_Test_005");
    int32_t fileId = InsertBasePhotoFusion("fuse_temp.jpg");
    ASSERT_GT(fileId, 0);
    SetPhotoFieldFusion(fileId, PhotoColumn::PHOTO_IS_TEMP, 1);
    auto resultSet = QueryFusionAssetResultSetFusion(fileId);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(MediaLibraryAlbumFusionUtils::CheckBatchAssets(resultSet), E_SCENE_HAS_DELETED);
}

HWTEST_F(MediaLibraryAlbumFusionTest, MediaLibrary_AlbumFusion_CheckBatchAssets_Test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_AlbumFusion_CheckBatchAssets_Test_006");
    int32_t fileId = InsertBasePhotoFusion("fuse_pending.jpg");
    ASSERT_GT(fileId, 0);
    SetPhotoFieldFusion(fileId, MediaColumn::MEDIA_TIME_PENDING, 1);
    auto resultSet = QueryFusionAssetResultSetFusion(fileId);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(MediaLibraryAlbumFusionUtils::CheckBatchAssets(resultSet), E_SCENE_IS_HIDDEN);
}

HWTEST_F(MediaLibraryAlbumFusionTest, MediaLibrary_AlbumFusion_CheckBatchAssets_Test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_AlbumFusion_CheckBatchAssets_Test_007");
    int32_t fileId = InsertBasePhotoFusion("fuse_hidden.jpg");
    ASSERT_GT(fileId, 0);
    SetPhotoFieldFusion(fileId, MediaColumn::MEDIA_HIDDEN, 1);
    auto resultSet = QueryFusionAssetResultSetFusion(fileId);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(MediaLibraryAlbumFusionUtils::CheckBatchAssets(resultSet), E_SCENE_IS_HIDDEN);
}

HWTEST_F(MediaLibraryAlbumFusionTest, MediaLibrary_AlbumFusion_CheckBatchAssets_Test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_AlbumFusion_CheckBatchAssets_Test_008");
    int32_t fileId = InsertBasePhotoFusion("fuse_trashed.jpg");
    ASSERT_GT(fileId, 0);
    SetPhotoFieldFusion(fileId, MediaColumn::MEDIA_DATE_TRASHED, 1);
    auto resultSet = QueryFusionAssetResultSetFusion(fileId);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(MediaLibraryAlbumFusionUtils::CheckBatchAssets(resultSet), E_SCENE_HAS_DELETED);
}

HWTEST_F(MediaLibraryAlbumFusionTest, MediaLibrary_AlbumFusion_CopyLocalSingleFile_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_AlbumFusion_CopyLocalSingleFile_Test_001");
    int64_t newAssetId = -1;
    std::shared_ptr<NativeRdb::ResultSet> resultSet = nullptr;
    EXPECT_EQ(MediaLibraryAlbumFusionUtils::CopyLocalSingleFile(nullptr, 0, resultSet, newAssetId), E_DB_FAIL);
}

HWTEST_F(MediaLibraryAlbumFusionTest, MediaLibrary_AlbumFusion_CloneSingleAsset_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_AlbumFusion_CloneSingleAsset_Test_001");
    EXPECT_EQ(MediaLibraryAlbumFusionUtils::CloneSingleAsset(999999, "cloned_title"), E_DB_FAIL);
}

HWTEST_F(MediaLibraryAlbumFusionTest, MediaLibrary_AlbumFusion_CloneProgressAsset_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_AlbumFusion_CloneProgressAsset_Test_001");
    CloneAssetInfo cloneAssetInfo;
    cloneAssetInfo.fileId = 999999;
    cloneAssetInfo.targetDisplayName = "cloned_title.jpg";
    cloneAssetInfo.targetFilePath = "";
    int32_t targetAlbumId = 0;
    string newAssetIds;
    auto cb = [](uint64_t) {};
    EXPECT_EQ(MediaLibraryAlbumFusionUtils::CloneProgressAsset(cloneAssetInfo, targetAlbumId,
        newAssetIds, cb, CloneCallbackType::URI), E_DB_FAIL);
}

} // namespace Media
} // namespace OHOS
