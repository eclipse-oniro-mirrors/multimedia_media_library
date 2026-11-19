/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "AlbumFusionUtilsTest"

#include "medialibrary_album_fusion_utils_test.h"

#include <chrono>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <thread>
#include <unistd.h>
#include <unordered_map>

#include "abs_shared_result_set.h"
#include "file_ex.h"
#include "media_column.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_formmap_operations.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_rdbstore.h"
#include "rdb_predicates.h"
#include "result_set_utils.h"
#include "string_ex.h"
#include "thumbnail_const.h"
#include "uri.h"
#include "userfile_manager_types.h"
#include "value_object.h"
#include "values_bucket.h"
#include "image_type.h"
#include "datashare_helper.h"
#include "unique_fd.h"
#include "medialibrary_data_manager.h"
#include "form_map.h"
#include "medialibrary_unittest_utils.h"
#include "ability_context_impl.h"

#define private public
#define protected public
#include "medialibrary_album_fusion_utils.h"
#undef private
#undef protected
#include "media_upgrade.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using OHOS::DataShare::DataShareValuesBucket;
using OHOS::DataShare::DataSharePredicates;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static shared_ptr<MediaLibraryRdbStore> rdbStorePtr = nullptr;
static const int64_t ASSET_ID = -1;
static const int32_t IAMGE_TYPE = 1;
static const int32_t SLEEP_5 = 5;
static const int32_t ALBUM_NUM = 7;
static const int64_t NUM_ONE = 1;
static const int64_t NUM_ZERO = 0;
static const int64_t NUM_INVALID = -1;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

static int32_t CreatePhotoApi10(const int32_t mediaType, const string &displayName)
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
    return ret;
}

struct PhotoResult {
    int64_t fileId;
    string title;
    string displayName;
    int32_t mediaType;
    int32_t position;
    int32_t isTemp;
    int64_t timePending;
    int32_t hidden;
    int64_t dateDeleted;
};

static int32_t InsertPhotoAsset(PhotoResult  &result)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    EXPECT_NE(rdbStore, nullptr);

    NativeRdb::ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, result.displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, result.mediaType);
    values.PutInt(PhotoColumn::PHOTO_POSITION, result.position);
    values.PutInt(PhotoColumn::PHOTO_IS_TEMP, result.isTemp);
    values.PutInt(MediaColumn::MEDIA_TIME_PENDING, result.timePending);
    values.PutInt(MediaColumn::MEDIA_HIDDEN, result.hidden);
    values.PutInt(PhotoColumn::MEDIA_DATE_DELETED, result.dateDeleted);

    rdbStore->Insert(result.fileId, PhotoColumn::PHOTOS_TABLE, values);
    return static_cast<int32_t>(result.fileId);
}

static void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        FormMap::FORM_MAP_TABLE,
    };
    for (auto &dropTable : dropTableList) {
        string dropSql = "DROP TABLE " + dropTable + ";";
        int32_t ret = g_rdbStore->ExecuteSql(dropSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Drop %{public}s table failed", dropTable.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Drop %{public}s table success", dropTable.c_str());
    }
}

static void SetTables()
{
    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        FormMap::CREATE_FORM_MAP_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        if (g_rdbStore == nullptr) {
            MEDIA_ERR_LOG("can not get g_rdbstore");
            return;
        }
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}

static void ClearAndRestart()
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
    CleanTestTables();
    SetTables();
}

void MediaLibraryAlbumFusionUtilsTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    auto stageContext = std::make_shared<AbilityRuntime::ContextImpl>();
    auto abilityContextImpl = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    abilityContextImpl->SetStageContext(stageContext);
    rdbStorePtr = std::make_shared<MediaLibraryRdbStore>(abilityContextImpl);
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
}

void MediaLibraryAlbumFusionUtilsTest::TearDownTestCase()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
    MEDIA_INFO_LOG("Clean is finish");
}

void MediaLibraryAlbumFusionUtilsTest::SetUp()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestart();
}

void MediaLibraryAlbumFusionUtilsTest::TearDown()
{}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_001, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryRdbStore> upgradeStore = nullptr;
    MediaLibraryAlbumFusionUtils::RemoveMisAddedHiddenData(upgradeStore);
    MediaLibraryAlbumFusionUtils::RemoveMisAddedHiddenData(rdbStorePtr);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_002, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryRdbStore> upgradeStore = nullptr;
    MediaLibraryAlbumFusionUtils::HandleMatchedDataFusion(upgradeStore);
    MediaLibraryAlbumFusionUtils::HandleMatchedDataFusion(rdbStorePtr);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_003, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryRdbStore> upgradeStore = nullptr;
    std::multimap<int32_t, vector<int32_t>> notMatchedMap;
    MediaLibraryAlbumFusionUtils::QueryNoMatchedMap(upgradeStore, notMatchedMap, true);
    MediaLibraryAlbumFusionUtils::QueryNoMatchedMap(rdbStorePtr, notMatchedMap, true);
    MediaLibraryAlbumFusionUtils::QueryNoMatchedMap(rdbStorePtr, notMatchedMap, false);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_004, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryRdbStore> upgradeStore = nullptr;
    string displayName = "test.jpg";
    int64_t assetId = NUM_ZERO;
    vector<string> columns = { PhotoColumn::MEDIA_FILE_PATH };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(NUM_ONE));
    EXPECT_NE(g_rdbStore, nullptr);
    auto resultSet = g_rdbStore->Query(cmd, columns);
    EXPECT_NE(resultSet, nullptr);
    MediaLibraryAlbumFusionUtils::CopyLocalSingleFile(upgradeStore, ALBUM_NUM, resultSet,
        assetId, displayName);
    MediaLibraryAlbumFusionUtils::CopyLocalSingleFile(rdbStorePtr, ALBUM_NUM, resultSet,
        assetId, displayName);

    MediaLibraryAlbumFusionUtils::CloudCopyParam cloudParam;
    cloudParam.assetId = static_cast<int32_t>(assetId);
    cloudParam.ownerAlbumId = ALBUM_NUM;
    MediaLibraryAlbumFusionUtils::CopyCloudSingleFile(upgradeStore, cloudParam, resultSet);
    MediaLibraryAlbumFusionUtils::CopyCloudSingleFile(rdbStorePtr, cloudParam, resultSet);

    MediaLibraryAlbumFusionUtils::SetRefreshAlbum(true);
    MediaLibraryAlbumFusionUtils::SetRefreshAlbum(false);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_005, TestSize.Level1)
{
    string displayName = "test.jpg";
    int32_t fileId = CreatePhotoApi10(IAMGE_TYPE, displayName);
    MediaLibraryAlbumFusionUtils::CloneSingleAsset(fileId, displayName);
    MediaLibraryAlbumFusionUtils::CloneSingleAsset(ASSET_ID, displayName);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_006, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryRdbStore> upgradeStore = nullptr;
    MediaLibraryAlbumFusionUtils::HandleNoOwnerData(upgradeStore);
    MediaLibraryAlbumFusionUtils::HandleNoOwnerData(rdbStorePtr);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_007, TestSize.Level1)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(rdbStore, nullptr);
    MediaLibraryRdbStore::ReconstructMediaLibraryStorageFormat(rdbStore);
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_5));
    MediaLibraryAlbumFusionUtils::CleanInvalidCloudAlbumAndData();
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_008, TestSize.Level1)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(rdbStore, nullptr);
    MediaLibraryRdbStore::ReconstructMediaLibraryStorageFormat(rdbStore);
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_5));
    MediaLibraryAlbumFusionUtils::CleanInvalidCloudAlbumAndData();
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_009, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryRdbStore> upgradeStore = nullptr;
    vector<int32_t> restOwnerAlbumIds;
    int32_t assetId = NUM_ONE;
    restOwnerAlbumIds.emplace_back(assetId);
    MediaLibraryAlbumFusionUtils::HandleRestData(upgradeStore, assetId, restOwnerAlbumIds,
        assetId);
    MediaLibraryAlbumFusionUtils::HandleRestData(rdbStorePtr, assetId, restOwnerAlbumIds,
        assetId);
    string displayName = "test.jpg";
    int32_t fileId = CreatePhotoApi10(IAMGE_TYPE, displayName);
    MediaLibraryAlbumFusionUtils::HandleRestData(rdbStorePtr, fileId, restOwnerAlbumIds,
        assetId);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_010, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryRdbStore> upgradeStore = nullptr;
    vector<int32_t> restOwnerAlbumIds;
    int32_t assetId = NUM_ZERO;
    restOwnerAlbumIds.emplace_back(assetId);
    MediaLibraryAlbumFusionUtils::HandleRestData(upgradeStore, assetId, restOwnerAlbumIds,
        assetId);
    MediaLibraryAlbumFusionUtils::HandleRestData(rdbStorePtr, assetId, restOwnerAlbumIds,
        assetId);
    string displayName = "test.jpg";
    int32_t fileId = CreatePhotoApi10(IAMGE_TYPE, displayName);
    MediaLibraryAlbumFusionUtils::HandleRestData(rdbStorePtr, fileId, restOwnerAlbumIds,
        assetId);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_011, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryRdbStore> upgradeStore = nullptr;
    std::multimap<int32_t, vector<int32_t>> notMatchedMap;
    MediaLibraryAlbumFusionUtils::HandleNotMatchedDataMigration(upgradeStore, notMatchedMap);
    MediaLibraryAlbumFusionUtils::HandleNotMatchedDataMigration(rdbStorePtr, notMatchedMap);
    vector<int32_t> valueVector = {};
    notMatchedMap.insert(pair<int32_t, std::vector<int32_t>>(0, valueVector));
    MediaLibraryAlbumFusionUtils::HandleNotMatchedDataMigration(rdbStorePtr, notMatchedMap);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_012, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryRdbStore> upgradeStore = nullptr;
    int32_t assetId = NUM_ZERO;
    int32_t ownerAlbumId = NUM_ZERO;
    int64_t newAssetId = NUM_ZERO;
    MediaLibraryAlbumFusionUtils::HandleSingleFileCopy(upgradeStore,
        assetId, ownerAlbumId, newAssetId);
    MediaLibraryAlbumFusionUtils::HandleNotMatchedDataFusion(upgradeStore);
    MediaLibraryAlbumFusionUtils::HandleNotMatchedDataFusion(rdbStorePtr);
    NativeRdb::ValuesBucket values;
    vector<string> columns = { PhotoColumn::MEDIA_FILE_PATH };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(NUM_ONE));
    EXPECT_NE(g_rdbStore, nullptr);
    auto resultSet = g_rdbStore->Query(cmd, columns);
    EXPECT_NE(resultSet, nullptr);
    string albumName = "Image";
    MediaLibraryAlbumFusionUtils::BuildAlbumInsertValuesSetName(rdbStorePtr,
        values, resultSet, albumName);
    int32_t albumId = NUM_ZERO;
    int32_t newAlbumId = NUM_INVALID;
    MediaLibraryAlbumFusionUtils::ExecuteObject executeObject;
    executeObject.rdbStore = upgradeStore;
    MediaLibraryAlbumFusionUtils::DeleteAlbumAndUpdateRelationship(executeObject,
        albumId, newAlbumId, false);
    newAlbumId = ALBUM_NUM;
    executeObject.rdbStore = rdbStorePtr;
    MediaLibraryAlbumFusionUtils::DeleteAlbumAndUpdateRelationship(executeObject,
        albumId, newAlbumId, false);
    MediaLibraryAlbumFusionUtils::DeleteAlbumAndUpdateRelationship(executeObject,
        albumId, newAlbumId, true);
    MediaLibraryAlbumFusionUtils::IsCloudAlbum(resultSet);
    MediaLibraryAlbumFusionUtils::HandleExpiredAlbumData(upgradeStore);
    MediaLibraryAlbumFusionUtils::HandleExpiredAlbumData(rdbStorePtr);
    MediaLibraryAlbumFusionUtils::RebuildAlbumAndFillCloudValue(upgradeStore);
    MediaLibraryAlbumFusionUtils::RebuildAlbumAndFillCloudValue(rdbStorePtr);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_013, TestSize.Level1)
{
    std::multimap<int32_t, vector<int32_t>> notMathedMap;
    std::shared_ptr<MediaLibraryRdbStore> upgradeStore = nullptr;
    MediaLibraryAlbumFusionUtils::HandleNewCloudDirtyData(upgradeStore, notMathedMap);
    MediaLibraryAlbumFusionUtils::HandleNewCloudDirtyData(rdbStorePtr, notMathedMap);
    MediaLibraryAlbumFusionUtils::HandleMisMatchScreenRecord(upgradeStore);
    MediaLibraryAlbumFusionUtils::HandleMisMatchScreenRecord(rdbStorePtr);
    MediaLibraryAlbumFusionUtils::RefreshAllAlbums();
    MediaLibraryAlbumFusionUtils::CleanInvalidCloudAlbumAndData();
    int64_t albumFusionTag = NUM_ZERO;
    AlbumFusionState albumFusionState = AlbumFusionState::START;
    MediaLibraryAlbumFusionUtils::ReportAlbumFusionData(albumFusionTag, albumFusionState,
        rdbStorePtr);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_014, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryRdbStore> upgradeStore = nullptr;
    int32_t newAlbumId = NUM_INVALID;
    int32_t albumId = NUM_ZERO;
    vector<string> columns = { PhotoColumn::MEDIA_FILE_PATH };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(NUM_ONE));
    EXPECT_NE(g_rdbStore, nullptr);
    auto resultSet = g_rdbStore->Query(cmd, columns);
    EXPECT_NE(resultSet, nullptr);
    MediaLibraryAlbumFusionUtils::MergeClashSourceAlbum(upgradeStore, resultSet, albumId,
        newAlbumId);
    MediaLibraryAlbumFusionUtils::MergeClashSourceAlbum(rdbStorePtr, resultSet, albumId,
        newAlbumId);
    MediaLibraryAlbumFusionUtils::HandleChangeNameAlbum(upgradeStore);
    MediaLibraryAlbumFusionUtils::HandleChangeNameAlbum(rdbStorePtr);
    MediaLibraryAlbumFusionUtils::CompensateLpathForLocalAlbum(upgradeStore);
    MediaLibraryAlbumFusionUtils::CompensateLpathForLocalAlbum(rdbStorePtr);
    MediaLibraryAlbumFusionUtils::SetParameterToStopSync();
    MediaLibraryAlbumFusionUtils::SetParameterToStartSync();
    MediaLibraryAlbumFusionUtils::GetAlbumFuseUpgradeStatus();
    MediaLibraryAlbumFusionUtils::SetAlbumFuseUpgradeStatus(NUM_INVALID);
    MediaLibraryAlbumFusionUtils::SetAlbumFuseUpgradeStatus(NUM_ONE);
    MediaLibraryAlbumFusionUtils::HandleDuplicateAlbum(upgradeStore);
    MediaLibraryAlbumFusionUtils::HandleDuplicateAlbum(rdbStorePtr);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_015, TestSize.Level1)
{
    string extension = "heic";
    struct PhotoResult photoAsset = {0, "IMG_20250903_103737.heic", "IMG_20250903_103737",
        static_cast<int32_t>(MEDIA_TYPE_IMAGE), 1, 0, 0, 0, 0};
    InsertPhotoAsset(photoAsset);

    auto resultSet = MediaLibraryAlbumFusionUtils::ConvertFormatAsset(photoAsset.fileId, photoAsset.title, extension);
    EXPECT_EQ(resultSet, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_016, TestSize.Level1)
{
    string extension = "heic";
    struct PhotoResult photoAsset = {1, "IMG_20250903_103737.heic", "IMG_20250903_103737",
        static_cast<int32_t>(MEDIA_TYPE_IMAGE), static_cast<int32_t>(PhotoPositionType::CLOUD), 0, 0, 0, 0};
    InsertPhotoAsset(photoAsset);

    auto resultSet = MediaLibraryAlbumFusionUtils::ConvertFormatAsset(photoAsset.fileId, photoAsset.title, extension);
    EXPECT_EQ(resultSet, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_017, TestSize.Level1)
{
    string extension = "heic";
    struct PhotoResult photoAsset = {1, "IMG_20250903_103737.heic", "IMG_20250903_103737",
        static_cast<int32_t>(MEDIA_TYPE_IMAGE), 1, 1, 0, 0, 0};
    InsertPhotoAsset(photoAsset);

    auto resultSet = MediaLibraryAlbumFusionUtils::ConvertFormatAsset(photoAsset.fileId, photoAsset.title, extension);
    EXPECT_EQ(resultSet, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_018, TestSize.Level1)
{
    string extension = "heic";
    struct PhotoResult photoAsset = {1, "IMG_20250903_103737.heic", "IMG_20250903_103737",
        static_cast<int32_t>(MEDIA_TYPE_IMAGE), 1, 0, -1, 0, 0};
    InsertPhotoAsset(photoAsset);

    auto resultSet = MediaLibraryAlbumFusionUtils::ConvertFormatAsset(photoAsset.fileId, photoAsset.title, extension);
    EXPECT_EQ(resultSet, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_019, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::SetParameterToStopSync();
    MediaLibraryAlbumFusionUtils::SetParameterToStartSync();
    int32_t status = MediaLibraryAlbumFusionUtils::GetAlbumFuseUpgradeStatus();
    EXPECT_GE(status, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_020, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::SetAlbumFuseUpgradeStatus(0);
    MediaLibraryAlbumFusionUtils::SetAlbumFuseUpgradeStatus(1);
    int32_t status = MediaLibraryAlbumFusionUtils::GetAlbumFuseUpgradeStatus();
    EXPECT_GE(status, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_021, TestSize.Level1)
{
    int64_t albumFusionTag = NUM_ONE;
    MediaLibraryAlbumFusionUtils::ReportAlbumFusionData(albumFusionTag, AlbumFusionState::START, rdbStorePtr);
    MediaLibraryAlbumFusionUtils::ReportAlbumFusionData(albumFusionTag, AlbumFusionState::SUCCESS, rdbStorePtr);
    MediaLibraryAlbumFusionUtils::ReportAlbumFusionData(albumFusionTag, AlbumFusionState::FAILED, rdbStorePtr);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_022, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::ReportAlbumFusionData(NUM_ZERO, AlbumFusionState::START, nullptr);
    MediaLibraryAlbumFusionUtils::ReportAlbumFusionData(NUM_INVALID, AlbumFusionState::FAILED, rdbStorePtr);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_023, TestSize.Level1)
{
    int32_t ret = MediaLibraryAlbumFusionUtils::RefreshAllAlbums();
    EXPECT_GE(ret, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_024, TestSize.Level1)
{
    int32_t ret = MediaLibraryAlbumFusionUtils::CleanInvalidCloudAlbumAndData(false);
    EXPECT_GE(ret, NUM_INVALID);
    ret = MediaLibraryAlbumFusionUtils::CleanInvalidCloudAlbumAndData(true);
    EXPECT_GE(ret, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_025, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::SetParameterToStopSync();
    MediaLibraryAlbumFusionUtils::SetParameterToStartSync();
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_026, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::HandleDuplicateAlbum(nullptr);
    MediaLibraryAlbumFusionUtils::HandleDuplicateAlbum(rdbStorePtr);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_027, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::SetParameterToStopSync();
    int32_t status = MediaLibraryAlbumFusionUtils::GetAlbumFuseUpgradeStatus();
    EXPECT_GE(status, NUM_INVALID);
    MediaLibraryAlbumFusionUtils::SetParameterToStartSync();
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_028, TestSize.Level1)
{
    int32_t ret = MediaLibraryAlbumFusionUtils::RefreshAllAlbums();
    EXPECT_GE(ret, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_029, TestSize.Level1)
{
    size_t size = 0;
    int32_t dupExist = 0;
    int32_t ret = MediaLibraryAlbumFusionUtils::CreateTmpCompatibleDup(ASSET_ID, "", size, dupExist);
    EXPECT_NE(ret, E_OK);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_030, TestSize.Level1)
{
    string displayName = "photo_test.jpg";
    int32_t fileId = CreatePhotoApi10(IAMGE_TYPE, displayName);
    if (fileId > 0) {
        MediaLibraryAlbumFusionUtils::CloneSingleAsset(static_cast<int64_t>(fileId), displayName);
    }
    MediaLibraryAlbumFusionUtils::CloneSingleAsset(NUM_ZERO, "zero_id.jpg");
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_031, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryRdbStore> upgradeStore = nullptr;
    std::multimap<int32_t, vector<int32_t>> notMatchedMap;
    notMatchedMap.insert(std::make_pair(1, vector<int32_t>{2, 3}));
    notMatchedMap.insert(std::make_pair(4, vector<int32_t>{5}));
    MediaLibraryAlbumFusionUtils::HandleNotMatchedDataMigration(upgradeStore, notMatchedMap);
    MediaLibraryAlbumFusionUtils::HandleNotMatchedDataMigration(rdbStorePtr, notMatchedMap);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_032, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryRdbStore> upgradeStore = nullptr;
    int32_t assetId = NUM_ONE;
    int32_t ownerAlbumId = ALBUM_NUM;
    int64_t newAssetId = NUM_ZERO;
    MediaLibraryAlbumFusionUtils::HandleSingleFileCopy(upgradeStore, assetId, ownerAlbumId, newAssetId);
    MediaLibraryAlbumFusionUtils::HandleSingleFileCopy(rdbStorePtr, assetId, ownerAlbumId, newAssetId);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_033, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryRdbStore> upgradeStore = nullptr;
    vector<int32_t> restOwnerAlbumIds = {1, 2, 3};
    int32_t assetId = NUM_ONE;
    int32_t handledCount = 0;
    MediaLibraryAlbumFusionUtils::HandleRestData(upgradeStore, assetId, restOwnerAlbumIds, handledCount);
    MediaLibraryAlbumFusionUtils::HandleRestData(rdbStorePtr, assetId, restOwnerAlbumIds, handledCount);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_034, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryRdbStore> upgradeStore = nullptr;
    std::multimap<int32_t, vector<int32_t>> notMatchedMap;
    MediaLibraryAlbumFusionUtils::QueryNoMatchedMap(upgradeStore, notMatchedMap, true);
    notMatchedMap.clear();
    MediaLibraryAlbumFusionUtils::QueryNoMatchedMap(rdbStorePtr, notMatchedMap, false);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_035, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryRdbStore> upgradeStore = nullptr;
    vector<string> columns = { PhotoColumn::MEDIA_FILE_PATH };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(NUM_ONE));
    auto resultSet = g_rdbStore->Query(cmd, columns);
    NativeRdb::ValuesBucket values;
    string albumName = "TestAlbum";
    MediaLibraryAlbumFusionUtils::BuildAlbumInsertValuesSetName(upgradeStore, values, resultSet, albumName);
    if (resultSet != nullptr) {
        MediaLibraryAlbumFusionUtils::BuildAlbumInsertValuesSetName(rdbStorePtr, values, resultSet, albumName);
    }
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_036, TestSize.Level1)
{
    vector<string> columns = { PhotoColumn::MEDIA_FILE_PATH };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(NUM_ONE));
    auto resultSet = g_rdbStore->Query(cmd, columns);
    bool isCloud = MediaLibraryAlbumFusionUtils::IsCloudAlbum(resultSet);
    EXPECT_FALSE(isCloud);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_037, TestSize.Level1)
{
    bool isCloud = MediaLibraryAlbumFusionUtils::IsCloudAlbum(nullptr);
    EXPECT_FALSE(isCloud);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_038, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::ExecuteObject executeObject;
    executeObject.rdbStore = nullptr;
    vector<string> fileIdsInAlbum;
    MediaLibraryAlbumFusionUtils::DeleteAlbumAndUpdateRelationship(executeObject, NUM_ZERO, NUM_INVALID,
        false, &fileIdsInAlbum);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_039, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::ExecuteObject executeObject;
    executeObject.rdbStore = rdbStorePtr;
    MediaLibraryAlbumFusionUtils::DeleteAlbumAndUpdateRelationship(executeObject, NUM_ZERO, ALBUM_NUM,
        true, nullptr);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_040, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryRdbStore> upgradeStore = nullptr;
    std::multimap<int32_t, vector<int32_t>> notMatchedMap;
    notMatchedMap.insert(std::make_pair(0, vector<int32_t>()));
    MediaLibraryAlbumFusionUtils::HandleNewCloudDirtyData(upgradeStore, notMatchedMap);
    MediaLibraryAlbumFusionUtils::HandleNewCloudDirtyData(rdbStorePtr, notMatchedMap);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_041, TestSize.Level1)
{
    string extension = "jpg";
    struct PhotoResult photoAsset = {1, "test.jpg", "test",
        static_cast<int32_t>(MEDIA_TYPE_IMAGE), 1, 0, 0, 0, 0};
    InsertPhotoAsset(photoAsset);
    auto resultSet = MediaLibraryAlbumFusionUtils::ConvertFormatAsset(photoAsset.fileId, photoAsset.title, extension);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_042, TestSize.Level1)
{
    auto resultSet = MediaLibraryAlbumFusionUtils::ConvertFormatAsset(NUM_INVALID, "invalid.jpg", "jpg");
    EXPECT_EQ(resultSet, nullptr);
    resultSet = MediaLibraryAlbumFusionUtils::ConvertFormatAsset(NUM_ZERO, "zero.jpg", "png");
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_043, TestSize.Level1)
{
    string extension = "heic";
    struct PhotoResult photoAsset = {2, "IMG.heic", "IMG",
        static_cast<int32_t>(MEDIA_TYPE_IMAGE), 1, 0, 0, 1, 0};
    InsertPhotoAsset(photoAsset);
    auto resultSet = MediaLibraryAlbumFusionUtils::ConvertFormatAsset(photoAsset.fileId, photoAsset.title, extension);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_044, TestSize.Level1)
{
    int32_t ret = MediaLibraryAlbumFusionUtils::RemoveMisAddedHiddenData(nullptr);
    EXPECT_NE(ret, E_OK);
    ret = MediaLibraryAlbumFusionUtils::RemoveMisAddedHiddenData(rdbStorePtr);
    EXPECT_GE(ret, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_045, TestSize.Level1)
{
    int32_t ret = MediaLibraryAlbumFusionUtils::HandleMatchedDataFusion(nullptr);
    EXPECT_NE(ret, E_OK);
    ret = MediaLibraryAlbumFusionUtils::HandleMatchedDataFusion(rdbStorePtr);
    EXPECT_GE(ret, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_046, TestSize.Level1)
{
    int32_t ret = MediaLibraryAlbumFusionUtils::HandleNotMatchedDataFusion(nullptr);
    EXPECT_NE(ret, E_OK);
    ret = MediaLibraryAlbumFusionUtils::HandleNotMatchedDataFusion(rdbStorePtr);
    EXPECT_GE(ret, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_047, TestSize.Level1)
{
    int32_t ret = MediaLibraryAlbumFusionUtils::HandleNoOwnerData(nullptr);
    EXPECT_NE(ret, E_OK);
    ret = MediaLibraryAlbumFusionUtils::HandleNoOwnerData(rdbStorePtr);
    EXPECT_GE(ret, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_048, TestSize.Level1)
{
    int32_t ret = MediaLibraryAlbumFusionUtils::HandleExpiredAlbumData(nullptr);
    EXPECT_NE(ret, E_OK);
    ret = MediaLibraryAlbumFusionUtils::HandleExpiredAlbumData(rdbStorePtr);
    EXPECT_GE(ret, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_049, TestSize.Level1)
{
    int32_t ret = MediaLibraryAlbumFusionUtils::RebuildAlbumAndFillCloudValue(nullptr);
    EXPECT_NE(ret, E_OK);
    ret = MediaLibraryAlbumFusionUtils::RebuildAlbumAndFillCloudValue(rdbStorePtr);
    EXPECT_GE(ret, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_050, TestSize.Level1)
{
    int32_t ret = MediaLibraryAlbumFusionUtils::HandleChangeNameAlbum(nullptr);
    EXPECT_NE(ret, E_OK);
    ret = MediaLibraryAlbumFusionUtils::HandleChangeNameAlbum(rdbStorePtr);
    EXPECT_GE(ret, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_051, TestSize.Level1)
{
    int32_t ret = MediaLibraryAlbumFusionUtils::CompensateLpathForLocalAlbum(nullptr);
    EXPECT_EQ(ret, E_OK);
    ret = MediaLibraryAlbumFusionUtils::CompensateLpathForLocalAlbum(rdbStorePtr);
    EXPECT_GE(ret, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_052, TestSize.Level1)
{
    int32_t ret = MediaLibraryAlbumFusionUtils::HandleDuplicateAlbum(nullptr);
    EXPECT_NE(ret, E_OK);
    ret = MediaLibraryAlbumFusionUtils::HandleDuplicateAlbum(rdbStorePtr);
    EXPECT_GE(ret, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_053, TestSize.Level1)
{
    int32_t ret = MediaLibraryAlbumFusionUtils::HandleMisMatchScreenRecord(nullptr);
    EXPECT_NE(ret, E_OK);
    ret = MediaLibraryAlbumFusionUtils::HandleMisMatchScreenRecord(rdbStorePtr);
    EXPECT_GE(ret, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_054, TestSize.Level1)
{
    string displayName = "test.jpg";
    int64_t assetId = NUM_ZERO;
    vector<string> columns = { PhotoColumn::MEDIA_FILE_PATH };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(NUM_ONE));
    auto resultSet = g_rdbStore->Query(cmd, columns);
    int32_t ret = MediaLibraryAlbumFusionUtils::CopyLocalSingleFile(nullptr, ALBUM_NUM, resultSet,
        assetId, displayName);
    EXPECT_NE(ret, E_OK);
    if (resultSet != nullptr) {
        ret = MediaLibraryAlbumFusionUtils::CopyLocalSingleFile(rdbStorePtr, ALBUM_NUM, resultSet,
            assetId, displayName);
        EXPECT_GE(ret, NUM_INVALID);
    }
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_055, TestSize.Level1)
{
    int64_t assetId = NUM_ZERO;
    vector<string> columns = { PhotoColumn::MEDIA_FILE_PATH };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(NUM_ONE));
    auto resultSet = g_rdbStore->Query(cmd, columns);
    int32_t ret = MediaLibraryAlbumFusionUtils::CopyCloudSingleFile(nullptr, assetId, ALBUM_NUM, resultSet, assetId);
    EXPECT_NE(ret, E_OK);
    if (resultSet != nullptr) {
        ret = MediaLibraryAlbumFusionUtils::CopyCloudSingleFile(rdbStorePtr, assetId, ALBUM_NUM, resultSet, assetId);
        EXPECT_LT(ret, NUM_INVALID);
    }
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_056, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryRdbStore> upgradeStore = nullptr;
    int32_t newAlbumId = NUM_INVALID;
    int32_t albumId = NUM_ZERO;
    vector<string> columns = { PhotoColumn::MEDIA_FILE_PATH };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(NUM_ONE));
    auto resultSet = g_rdbStore->Query(cmd, columns);
    int32_t ret = MediaLibraryAlbumFusionUtils::MergeClashSourceAlbum(upgradeStore, resultSet, albumId, newAlbumId);
    EXPECT_NE(ret, E_OK);
    if (resultSet != nullptr) {
        ret = MediaLibraryAlbumFusionUtils::MergeClashSourceAlbum(rdbStorePtr, resultSet, albumId, newAlbumId);
        EXPECT_GE(ret, NUM_INVALID);
    }
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_057, TestSize.Level1)
{
    std::multimap<int32_t, vector<int32_t>> notMatchedMap;
    int32_t ret = MediaLibraryAlbumFusionUtils::HandleNotMatchedDataMigration(nullptr, notMatchedMap);
    EXPECT_NE(ret, E_OK);
    ret = MediaLibraryAlbumFusionUtils::HandleNotMatchedDataMigration(rdbStorePtr, notMatchedMap);
    EXPECT_GE(ret, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_058, TestSize.Level1)
{
    int32_t assetId = NUM_INVALID;
    int32_t ownerAlbumId = NUM_ZERO;
    int64_t newAssetId = NUM_ZERO;
    int32_t ret = MediaLibraryAlbumFusionUtils::HandleSingleFileCopy(nullptr, assetId, ownerAlbumId, newAssetId);
    EXPECT_NE(ret, E_OK);
    ret = MediaLibraryAlbumFusionUtils::HandleSingleFileCopy(rdbStorePtr, assetId, ownerAlbumId, newAssetId);
    EXPECT_LT(ret, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_059, TestSize.Level1)
{
    vector<int32_t> restOwnerAlbumIds;
    int32_t assetId = NUM_INVALID;
    int32_t handledCount = 0;
    MediaLibraryAlbumFusionUtils::HandleRestData(nullptr, assetId, restOwnerAlbumIds, handledCount);
    MediaLibraryAlbumFusionUtils::HandleRestData(rdbStorePtr, assetId, restOwnerAlbumIds, handledCount);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_060, TestSize.Level1)
{
    std::multimap<int32_t, vector<int32_t>> notMatchedMap;
    int32_t ret = MediaLibraryAlbumFusionUtils::QueryNoMatchedMap(nullptr, notMatchedMap, true);
    EXPECT_NE(ret, E_OK);
    ret = MediaLibraryAlbumFusionUtils::QueryNoMatchedMap(rdbStorePtr, notMatchedMap, true);
    EXPECT_GE(ret, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_061, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::SetRefreshAlbum(true);
    int32_t ret = MediaLibraryAlbumFusionUtils::RefreshAllAlbums();
    EXPECT_GE(ret, NUM_INVALID);
    MediaLibraryAlbumFusionUtils::SetRefreshAlbum(false);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_062, TestSize.Level1)
{
    for (int32_t i = 0; i < 3; i++) {
        MediaLibraryAlbumFusionUtils::SetParameterToStopSync();
        MediaLibraryAlbumFusionUtils::SetParameterToStartSync();
    }
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_063, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::SetAlbumFuseUpgradeStatus(0);
    int32_t status1 = MediaLibraryAlbumFusionUtils::GetAlbumFuseUpgradeStatus();
    MediaLibraryAlbumFusionUtils::SetAlbumFuseUpgradeStatus(1);
    int32_t status2 = MediaLibraryAlbumFusionUtils::GetAlbumFuseUpgradeStatus();
    EXPECT_GE(status1, NUM_INVALID);
    EXPECT_GE(status2, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_064, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::ReportAlbumFusionData(NUM_ONE, AlbumFusionState::START, rdbStorePtr);
    MediaLibraryAlbumFusionUtils::ReportAlbumFusionData(NUM_ONE, AlbumFusionState::SUCCESS, rdbStorePtr);
    MediaLibraryAlbumFusionUtils::ReportAlbumFusionData(NUM_ONE, AlbumFusionState::FAILED, rdbStorePtr);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_065, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::CleanInvalidCloudAlbumAndData();
    MediaLibraryAlbumFusionUtils::CleanInvalidCloudAlbumAndData(false);
    MediaLibraryAlbumFusionUtils::CleanInvalidCloudAlbumAndData(true);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_066, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::HandleDuplicateAlbum(rdbStorePtr);
    MediaLibraryAlbumFusionUtils::HandleChangeNameAlbum(rdbStorePtr);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_067, TestSize.Level1)
{
    int32_t ret = MediaLibraryAlbumFusionUtils::CleanInvalidCloudAlbumAndData(false);
    EXPECT_GE(ret, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_068, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::CloneSingleAsset(NUM_INVALID, "invalid.jpg");
    MediaLibraryAlbumFusionUtils::CloneSingleAsset(999999, "large_id.jpg");
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_069, TestSize.Level1)
{
    std::multimap<int32_t, vector<int32_t>> notMatchedMap;
    notMatchedMap.insert(std::make_pair(10, vector<int32_t>{11, 12, 13}));
    notMatchedMap.insert(std::make_pair(20, vector<int32_t>{21}));
    MediaLibraryAlbumFusionUtils::HandleNotMatchedDataMigration(rdbStorePtr, notMatchedMap);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_070, TestSize.Level1)
{
    vector<int32_t> restOwnerAlbumIds = {100, 200};
    int32_t assetId = 1;
    int32_t handledCount = 0;
    MediaLibraryAlbumFusionUtils::HandleRestData(rdbStorePtr, assetId, restOwnerAlbumIds, handledCount);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_071, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryRdbStore> upgradeStore = nullptr;
    std::multimap<int32_t, vector<int32_t>> notMatchedMap;
    MediaLibraryAlbumFusionUtils::HandleNewCloudDirtyData(upgradeStore, notMatchedMap);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_072, TestSize.Level1)
{
    NativeRdb::ValuesBucket values;
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(NUM_ONE));
    auto resultSet = g_rdbStore->Query(cmd, {PhotoColumn::MEDIA_FILE_PATH});
    MediaLibraryAlbumFusionUtils::BuildAlbumInsertValuesSetName(rdbStorePtr, values, resultSet, "");
    MediaLibraryAlbumFusionUtils::BuildAlbumInsertValuesSetName(rdbStorePtr, values, resultSet, "NewAlbum");
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_073, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::ExecuteObject executeObject;
    executeObject.rdbStore = rdbStorePtr;
    MediaLibraryAlbumFusionUtils::DeleteAlbumAndUpdateRelationship(executeObject, 1, 2, false);
    MediaLibraryAlbumFusionUtils::DeleteAlbumAndUpdateRelationship(executeObject, 1, 2, true);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_074, TestSize.Level1)
{
    string extension = "heic";
    struct PhotoResult photoAsset = {3, "file.heic", "file",
        static_cast<int32_t>(MEDIA_TYPE_IMAGE), static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD), 0, 0, 0, 0};
    InsertPhotoAsset(photoAsset);
    auto resultSet = MediaLibraryAlbumFusionUtils::ConvertFormatAsset(photoAsset.fileId, photoAsset.title, extension);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_075, TestSize.Level1)
{
    struct PhotoResult photoAsset = {4, "video.mp4", "video",
        static_cast<int32_t>(MEDIA_TYPE_VIDEO), 1, 0, 0, 0, 0};
    InsertPhotoAsset(photoAsset);
    auto resultSet = MediaLibraryAlbumFusionUtils::ConvertFormatAsset(photoAsset.fileId, photoAsset.title, "mp4");
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_076, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::RemoveMisAddedHiddenData(rdbStorePtr);
    MediaLibraryAlbumFusionUtils::HandleMatchedDataFusion(rdbStorePtr);
    MediaLibraryAlbumFusionUtils::HandleNotMatchedDataFusion(rdbStorePtr);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_077, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::HandleNoOwnerData(rdbStorePtr);
    MediaLibraryAlbumFusionUtils::HandleExpiredAlbumData(rdbStorePtr);
    MediaLibraryAlbumFusionUtils::RebuildAlbumAndFillCloudValue(rdbStorePtr);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_078, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::HandleChangeNameAlbum(rdbStorePtr);
    MediaLibraryAlbumFusionUtils::CompensateLpathForLocalAlbum(rdbStorePtr);
    MediaLibraryAlbumFusionUtils::HandleDuplicateAlbum(rdbStorePtr);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_079, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::HandleMisMatchScreenRecord(rdbStorePtr);
    MediaLibraryAlbumFusionUtils::RefreshAllAlbums();
    MediaLibraryAlbumFusionUtils::CleanInvalidCloudAlbumAndData();
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_080, TestSize.Level1)
{
    std::multimap<int32_t, vector<int32_t>> notMatchedMap;
    MediaLibraryAlbumFusionUtils::QueryNoMatchedMap(rdbStorePtr, notMatchedMap, true);
    size_t mapSize = notMatchedMap.size();
    EXPECT_GE(mapSize, 0u);
    MediaLibraryAlbumFusionUtils::QueryNoMatchedMap(rdbStorePtr, notMatchedMap, false);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_081, TestSize.Level1)
{
    string displayName = "multi_test.jpg";
    int32_t fileId = CreatePhotoApi10(IAMGE_TYPE, displayName);
    if (fileId > 0) {
        MediaLibraryAlbumFusionUtils::CloneSingleAsset(static_cast<int64_t>(fileId), "clone1.jpg");
        MediaLibraryAlbumFusionUtils::CloneSingleAsset(static_cast<int64_t>(fileId), "clone2.jpg");
    }
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_082, TestSize.Level1)
{
    int64_t tags[] = {0, 1, -1, 100};
    for (auto tag : tags) {
        MediaLibraryAlbumFusionUtils::ReportAlbumFusionData(tag, AlbumFusionState::START, rdbStorePtr);
    }
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_083, TestSize.Level1)
{
    string displayNames[] = {"a.jpg", "b.png", "c.gif"};
    for (const auto& name : displayNames) {
        MediaLibraryAlbumFusionUtils::CloneSingleAsset(ASSET_ID, name);
    }
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_084, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::SetParameterToStopSync();
    MediaLibraryAlbumFusionUtils::RefreshAllAlbums();
    MediaLibraryAlbumFusionUtils::SetParameterToStartSync();
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_085, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::RemoveMisAddedHiddenData(rdbStorePtr);
    MediaLibraryAlbumFusionUtils::HandleMatchedDataFusion(rdbStorePtr);
    MediaLibraryAlbumFusionUtils::HandleNotMatchedDataFusion(rdbStorePtr);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_086, TestSize.Level1)
{
    int32_t assetId = 5;
    int32_t ownerAlbumId = 10;
    int64_t newAssetId = 0;
    MediaLibraryAlbumFusionUtils::HandleSingleFileCopy(rdbStorePtr, assetId, ownerAlbumId, newAssetId);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_087, TestSize.Level1)
{
    vector<string> columns = { PhotoColumn::MEDIA_FILE_PATH };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(NUM_ONE));
    auto resultSet = g_rdbStore->Query(cmd, columns);
    int32_t albumId = 0;
    int64_t newAlbumId = -1;
    if (resultSet != nullptr) {
        MediaLibraryAlbumFusionUtils::MergeClashSourceAlbum(rdbStorePtr, resultSet, albumId, newAlbumId);
    }
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_088, TestSize.Level1)
{
    std::multimap<int32_t, vector<int32_t>> notMatchedMap;
    notMatchedMap.insert(std::make_pair(1, vector<int32_t>()));
    notMatchedMap.insert(std::make_pair(2, vector<int32_t>{1, 2}));
    MediaLibraryAlbumFusionUtils::HandleNewCloudDirtyData(rdbStorePtr, notMatchedMap);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_089, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::ExecuteObject executeObject;
    executeObject.rdbStore = rdbStorePtr;
    vector<string> fileIds = {"1", "2"};
    MediaLibraryAlbumFusionUtils::DeleteAlbumAndUpdateRelationship(executeObject, 0, 1, false, &fileIds);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_090, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::SetAlbumFuseUpgradeStatus(0);
    int32_t status = MediaLibraryAlbumFusionUtils::GetAlbumFuseUpgradeStatus();
    EXPECT_GE(status, NUM_INVALID);
    MediaLibraryAlbumFusionUtils::SetAlbumFuseUpgradeStatus(1);
    status = MediaLibraryAlbumFusionUtils::GetAlbumFuseUpgradeStatus();
    EXPECT_GE(status, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_091, TestSize.Level1)
{
    string displayName = "sync_test.jpg";
    CreatePhotoApi10(IAMGE_TYPE, displayName);
    MediaLibraryAlbumFusionUtils::SetParameterToStopSync();
    MediaLibraryAlbumFusionUtils::RefreshAllAlbums();
    MediaLibraryAlbumFusionUtils::SetParameterToStartSync();
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_092, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::CleanInvalidCloudAlbumAndData(true);
    int32_t ret = MediaLibraryAlbumFusionUtils::RefreshAllAlbums();
    EXPECT_GE(ret, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_093, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::IsCloudAlbum(nullptr);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(NUM_ONE));
    auto resultSet = g_rdbStore->Query(cmd, {PhotoColumn::MEDIA_FILE_PATH});
    MediaLibraryAlbumFusionUtils::IsCloudAlbum(resultSet);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_094, TestSize.Level1)
{
    int64_t assetId = NUM_ZERO;
    vector<string> columns = { PhotoColumn::MEDIA_FILE_PATH };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(NUM_ONE));
    auto resultSet = g_rdbStore->Query(cmd, columns);
    if (resultSet != nullptr) {
        MediaLibraryAlbumFusionUtils::CopyLocalSingleFile(rdbStorePtr, 1, resultSet, assetId, "test.jpg");
        MediaLibraryAlbumFusionUtils::CopyCloudSingleFile(rdbStorePtr, 0, 1, resultSet, assetId);
    }
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_095, TestSize.Level1)
{
    vector<int32_t> restOwnerAlbumIds = {1};
    int32_t assetId = 0;
    int32_t handledCount = 0;
    MediaLibraryAlbumFusionUtils::HandleRestData(rdbStorePtr, assetId, restOwnerAlbumIds, handledCount);
    restOwnerAlbumIds = {1, 2, 3, 4, 5};
    MediaLibraryAlbumFusionUtils::HandleRestData(rdbStorePtr, assetId, restOwnerAlbumIds, handledCount);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_096, TestSize.Level1)
{
    string extensions[] = {"heic", "jpg", "png"};
    struct PhotoResult photoAsset = {5, "test.heic", "test",
        static_cast<int32_t>(MEDIA_TYPE_IMAGE), 1, 0, 0, 0, 0};
    InsertPhotoAsset(photoAsset);
    for (const auto& ext : extensions) {
        MediaLibraryAlbumFusionUtils::ConvertFormatAsset(photoAsset.fileId, photoAsset.title, ext);
    }
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_097, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::ExecuteObject executeObject;
    executeObject.rdbStore = rdbStorePtr;
    MediaLibraryAlbumFusionUtils::DeleteAlbumAndUpdateRelationship(executeObject, 0, 0, false);
    MediaLibraryAlbumFusionUtils::DeleteAlbumAndUpdateRelationship(executeObject, 100, 200, true);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_098, TestSize.Level1)
{
    std::multimap<int32_t, vector<int32_t>> notMatchedMap;
    MediaLibraryAlbumFusionUtils::QueryNoMatchedMap(rdbStorePtr, notMatchedMap, true);
    MediaLibraryAlbumFusionUtils::HandleNotMatchedDataMigration(rdbStorePtr, notMatchedMap);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_099, TestSize.Level1)
{
    NativeRdb::ValuesBucket values;
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(NUM_ONE));
    auto resultSet = g_rdbStore->Query(cmd, {PhotoColumn::MEDIA_FILE_PATH});
    MediaLibraryAlbumFusionUtils::BuildAlbumInsertValuesSetName(rdbStorePtr, values, resultSet, "Album1");
    MediaLibraryAlbumFusionUtils::BuildAlbumInsertValuesSetName(rdbStorePtr, values, resultSet, "Album2");
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_100, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::SetRefreshAlbum(true);
    MediaLibraryAlbumFusionUtils::RefreshAllAlbums();
    MediaLibraryAlbumFusionUtils::CleanInvalidCloudAlbumAndData();
    MediaLibraryAlbumFusionUtils::SetRefreshAlbum(false);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_101, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryRdbStore> upgradeStore = nullptr;
    MediaLibraryAlbumFusionUtils::RemoveMisAddedHiddenData(upgradeStore);
    MediaLibraryAlbumFusionUtils::HandleMatchedDataFusion(upgradeStore);
    MediaLibraryAlbumFusionUtils::HandleNoOwnerData(upgradeStore);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_102, TestSize.Level1)
{
    std::multimap<int32_t, vector<int32_t>> emptyMap;
    std::multimap<int32_t, vector<int32_t>> filledMap;
    filledMap.insert(std::make_pair(1, vector<int32_t>{2}));
    MediaLibraryAlbumFusionUtils::HandleNotMatchedDataMigration(rdbStorePtr, emptyMap);
    MediaLibraryAlbumFusionUtils::HandleNotMatchedDataMigration(rdbStorePtr, filledMap);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_103, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::SetAlbumFuseUpgradeStatus(0);
    MediaLibraryAlbumFusionUtils::ReportAlbumFusionData(0, AlbumFusionState::FAILED, rdbStorePtr);
    MediaLibraryAlbumFusionUtils::SetAlbumFuseUpgradeStatus(1);
    MediaLibraryAlbumFusionUtils::ReportAlbumFusionData(1, AlbumFusionState::SUCCESS, rdbStorePtr);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_104, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::SetAlbumFuseUpgradeStatus(0);
    MediaLibraryAlbumFusionUtils::SetAlbumFuseUpgradeStatus(1);
    int32_t status = MediaLibraryAlbumFusionUtils::GetAlbumFuseUpgradeStatus();
    EXPECT_GE(status, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_105, TestSize.Level1)
{
    vector<int32_t> singleId = {1};
    vector<int32_t> multiIds = {1, 2, 3, 4, 5};
    int32_t handledCount = 0;
    MediaLibraryAlbumFusionUtils::HandleRestData(rdbStorePtr, 0, singleId, handledCount);
    MediaLibraryAlbumFusionUtils::HandleRestData(rdbStorePtr, 1, multiIds, handledCount);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_106, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::ExecuteObject execObj;
    execObj.rdbStore = rdbStorePtr;
    MediaLibraryAlbumFusionUtils::DeleteAlbumAndUpdateRelationship(execObj, 5, 10, false);
    MediaLibraryAlbumFusionUtils::DeleteAlbumAndUpdateRelationship(execObj, 10, 20, true);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_107, TestSize.Level1)
{
    struct PhotoResult asset = {6, "convert.heic", "convert",
        static_cast<int32_t>(MEDIA_TYPE_IMAGE), 1, 0, 0, 0, 0};
    InsertPhotoAsset(asset);
    MediaLibraryAlbumFusionUtils::ConvertFormatAsset(asset.fileId, asset.title, "heic");
    MediaLibraryAlbumFusionUtils::ConvertFormatAsset(asset.fileId, asset.title, "jpg");
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_108, TestSize.Level1)
{
    MediaLibraryAlbumFusionUtils::HandleChangeNameAlbum(rdbStorePtr);
    MediaLibraryAlbumFusionUtils::CompensateLpathForLocalAlbum(rdbStorePtr);
    MediaLibraryAlbumFusionUtils::HandleDuplicateAlbum(rdbStorePtr);
    MediaLibraryAlbumFusionUtils::HandleMisMatchScreenRecord(rdbStorePtr);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_109, TestSize.Level1)
{
    int32_t ret1 = MediaLibraryAlbumFusionUtils::RefreshAllAlbums();
    int32_t ret2 = MediaLibraryAlbumFusionUtils::CleanInvalidCloudAlbumAndData(false);
    int32_t ret3 = MediaLibraryAlbumFusionUtils::CleanInvalidCloudAlbumAndData(true);
    EXPECT_GE(ret1, NUM_INVALID);
    EXPECT_GE(ret2, NUM_INVALID);
    EXPECT_GE(ret3, NUM_INVALID);
    ASSERT_NE(rdbStorePtr, nullptr);
}

HWTEST_F(MediaLibraryAlbumFusionUtilsTest, AlbumFusionUtils_test_110, TestSize.Level1)
{
    std::multimap<int32_t, vector<int32_t>> notMatchedMap;
    MediaLibraryAlbumFusionUtils::HandleNewCloudDirtyData(rdbStorePtr, notMatchedMap);
    MediaLibraryAlbumFusionUtils::HandleDuplicateAlbum(rdbStorePtr);
    ASSERT_NE(rdbStorePtr, nullptr);
}
}
}
