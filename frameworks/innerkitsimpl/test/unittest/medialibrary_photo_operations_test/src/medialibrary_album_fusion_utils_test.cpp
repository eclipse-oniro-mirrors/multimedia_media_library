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
    
    MediaLibraryAlbumFusionUtils::CopyCloudSingleFile(upgradeStore, assetId, ALBUM_NUM,
        resultSet, assetId);
    MediaLibraryAlbumFusionUtils::CopyCloudSingleFile(rdbStorePtr, assetId, ALBUM_NUM,
        resultSet, assetId);
    
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
}
}
