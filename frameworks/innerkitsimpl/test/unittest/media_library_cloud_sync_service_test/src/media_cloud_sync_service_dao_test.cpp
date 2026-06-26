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

#define MLOG_TAG "MediaCloudSync"

#include "media_cloud_sync_service_dao_test.h"

#include "media_log.h"
#include "result_set_utils.h"
#include "media_file_utils.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_photo_operations.h"
#include "fetch_result.h"

#define private public
#include "cloud_media_album_dao.h"
#include "cloud_media_data_dao.h"
#include "cloud_media_photos_dao.h"
#undef private
#include "cloud_media_common_dao.h"
#include "media_cloud_sync_test_utils.h"

#include <iostream>

namespace OHOS::Media::CloudSync {
using namespace testing::ext;
using namespace OHOS::NativeRdb;

static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;

void CloudMediaSyncServiceDaoTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "SetUpTestCase";
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        GTEST_LOG_(ERROR) << "Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore";
        exit(1);
    }
    SetTestTables(g_rdbStore);
}

void CloudMediaSyncServiceDaoTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "TearDownTestCase";
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    ClearAndRestart(g_rdbStore);
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
}

// SetUp:Execute before each test case
void CloudMediaSyncServiceDaoTest::SetUp()
{
    if (g_rdbStore == nullptr) {
        GTEST_LOG_(ERROR) << "Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore";
        exit(1);
    }
    ClearAndRestart(g_rdbStore);
}

void CloudMediaSyncServiceDaoTest::TearDown()
{}

HWTEST_F(CloudMediaSyncServiceDaoTest, CloudMediaAlbumDao_HandleLPathAndAlbumType_Test_001, TestSize.Level1)
{
    CloudMediaAlbumDao albumDao;
    PhotoAlbumDto record;
    int32_t ret = albumDao.HandleLPathAndAlbumType(record);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, CloudMediaAlbumDao_HandleLPathAndAlbumType_Test_002, TestSize.Level1)
{
    PhotoAlbumDto record1 = {
        .albumId = 15,
        .albumType = 0,
        .albumSubType = 2,
        .albumName = "test1",
        .lPath = "test2",
        .bundleName = "test3",
        .priority = 0,
        .cloudId = "10",
        .newCloudId = "20",
        .localLanguage = "test4",
        .albumDateCreated = 50,
        .albumDateAdded = 100,
        .albumDateModified = 150,
        .isDelete = false,
        .isSuccess = true,
    };
    CloudMediaAlbumDao albumDao;
    auto albumRefreshHandle = make_shared<AccurateRefresh::AlbumAccurateRefresh>();
    int32_t ret1 = albumDao.InsertAlbums(record1, albumRefreshHandle);
    std::cout << "ret1: " << ret1 << std::endl;

    PhotoAlbumDto record2;
    int32_t ret2 = albumDao.HandleLPathAndAlbumType(record2);
    std::cout << "ret2: " << ret2 << std::endl;
}

HWTEST_F(CloudMediaSyncServiceDaoTest, CloudMediaAlbumDao_HandleLPathAndAlbumType_Test_003, TestSize.Level1)
{
    CloudMediaAlbumDao albumDao;
    PhotoAlbumDto record;
    record.lPath = "/FromDocs/Documents";
    record.albumType = static_cast<int32_t>(PhotoAlbumType::INVALID);
    int32_t ret = albumDao.HandleLPathAndAlbumType(record);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(record.albumType, PhotoAlbumType::SOURCE);
    EXPECT_EQ(record.albumSubType, PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, CloudMediaAlbumDao_ReplaceCoverUriCondition_Test_001, TestSize.Level1)
{
    CloudMediaAlbumDao albumDao;
    PhotoAlbumDto record;
    std::string coverUri = "file://media/Photo/93/VID_1754566736_008/SVID_20250807_193716_1.jpg";
    std::string lPath = "/Pictures/Screenrecords";
    bool ret = albumDao.ReplaceCoverUriCondition(coverUri, lPath);
    EXPECT_EQ(ret, false);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, CloudMediaAlbumDao_ReplaceCoverUriCondition_Test_002, TestSize.Level1)
{
    CloudMediaAlbumDao albumDao;
    PhotoAlbumDto record;
    std::string coverUri = "file://media/Photo/93/VID_1754566736_008/SVID_20250807_193716_1.mp4";
    std::string lPath = "/Pictures/Screenshots";
    bool ret = albumDao.ReplaceCoverUriCondition(coverUri, lPath);
    EXPECT_EQ(ret, false);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, CloudMediaAlbumDao_ReplaceCoverUriCondition_Test_003, TestSize.Level1)
{
    CloudMediaAlbumDao albumDao;
    PhotoAlbumDto record;
    std::string coverUri = "file://media/Photo/93/VID_1754566736_008/SVID_20250807_193716_1.mp4";
    std::string lPath = "/Pictures/Screenrecords";
    bool ret = albumDao.ReplaceCoverUriCondition(coverUri, lPath);
    EXPECT_EQ(ret, true);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, CloudMediaAlbumDao_ReplaceCoverUriCondition_Test_004, TestSize.Level1)
{
    CloudMediaAlbumDao albumDao;
    PhotoAlbumDto record;
    std::string coverUri = "file://media/Photo/93/VID_1754566736_008/SVID_20250807_193716_1.jpg";
    std::string lPath = "/Pictures/Screenshots";
    bool ret = albumDao.ReplaceCoverUriCondition(coverUri, lPath);
    EXPECT_EQ(ret, true);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, BatchInsertFile_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::map<std::string, int> recordAnalysisAlbumMaps;
    std::map<std::string, std::set<int>> recordAlbumMaps;
    std::vector<NativeRdb::ValuesBucket> insertFiles;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    int32_t ret = photosDao.BatchInsertFile(recordAnalysisAlbumMaps, recordAlbumMaps, insertFiles, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, BatchInsertFile_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::map<std::string, int> recordAnalysisAlbumMaps = {{"cloud_id_1", 1}};
    std::map<std::string, std::set<int>> recordAlbumMaps = {{"cloud_id_2", {1, 2}}};
    std::vector<NativeRdb::ValuesBucket> insertFiles;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    int32_t ret = photosDao.BatchInsertFile(recordAnalysisAlbumMaps, recordAlbumMaps, insertFiles, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, BatchInsertFile_Test_003, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::map<std::string, int> recordAnalysisAlbumMaps;
    std::map<std::string, std::set<int>> recordAlbumMaps;
    std::vector<NativeRdb::ValuesBucket> insertFiles;
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::MEDIA_ID, 1);
    values.PutString(PhotoColumn::MEDIA_NAME, "test.jpg");
    insertFiles.push_back(values);
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    int32_t ret = photosDao.BatchInsertFile(recordAnalysisAlbumMaps, recordAlbumMaps, insertFiles, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, BatchInsertAssetMaps_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::map<std::string, std::set<int32_t>> recordAlbumMaps;
    int32_t ret = photosDao.BatchInsertAssetMaps(recordAlbumMaps);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, BatchInsertAssetMaps_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::map<std::string, std::set<int32_t>> recordAlbumMaps = {{"cloud_id_1", {1, 2, 3}}};
    int32_t ret = photosDao.BatchInsertAssetMaps(recordAlbumMaps);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, BatchInsertAssetAnalysisMaps_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::map<std::string, int32_t> recordAnalysisAlbumMaps;
    int32_t ret = photosDao.BatchInsertAssetAnalysisMaps(recordAnalysisAlbumMaps);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, BatchInsertAssetAnalysisMaps_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::map<std::string, int32_t> recordAnalysisAlbumMaps = {{"cloud_id_1", 1}};
    int32_t ret = photosDao.BatchInsertAssetAnalysisMaps(recordAnalysisAlbumMaps);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, BatchInsertQuick_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    int64_t outRowId = 0;
    std::vector<NativeRdb::ValuesBucket> initialBatchValues;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    int32_t ret = photosDao.BatchInsertQuick(outRowId, PhotoColumn::PHOTOS_TABLE, initialBatchValues, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, BatchInsertQuick_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    int64_t outRowId = 0;
    std::vector<NativeRdb::ValuesBucket> initialBatchValues;
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::MEDIA_ID, 1);
    values.PutString(PhotoColumn::MEDIA_NAME, "test.jpg");
    initialBatchValues.push_back(values);
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    int32_t ret = photosDao.BatchInsertQuick(outRowId, PhotoColumn::PHOTOS_TABLE, initialBatchValues, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateAssetInPhotoMap_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    int32_t fileId = 1;
    std::set<int> cloudMapIds;
    int32_t ret = photosDao.UpdateAssetInPhotoMap(fileId, cloudMapIds);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateAssetInPhotoMap_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    int32_t fileId = 1;
    std::set<int> cloudMapIds = {1, 2, 3};
    int32_t ret = photosDao.UpdateAssetInPhotoMap(fileId, cloudMapIds);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdatePhotosSynced_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_CLOUD_ID, "test_cloud_id");
    int32_t dirtyValue = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    int32_t ret = photosDao.UpdatePhotosSynced(predicates, dirtyValue);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdatePhotosSynced_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::string table = PhotoColumn::PHOTOS_TABLE;
    std::string whereClause = "cloud_id = ?";
    std::vector<std::string> args = {"test_cloud_id"};
    int32_t dirtyValue = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    int32_t ret = photosDao.UpdatePhotosSynced(table, whereClause, args, dirtyValue);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetFieldIntValue_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    NativeRdb::ValuesBucket values;
    std::string fieldName = PhotoColumn::PHOTO_DIRTY;
    int32_t defaultFieldValue = 0;
    int32_t ret = photosDao.GetFieldIntValue(values, fieldName, defaultFieldValue);
    EXPECT_EQ(ret, defaultFieldValue);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetFieldIntValue_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_DIRTY, 1);
    std::string fieldName = PhotoColumn::PHOTO_DIRTY;
    int32_t defaultFieldValue = 0;
    int32_t ret = photosDao.GetFieldIntValue(values, fieldName, defaultFieldValue);
    EXPECT_EQ(ret, 1);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, IsNeededFix_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.attributesFixVersion = -1;
    bool ret = photosDao.IsNeededFix(pullData);
    EXPECT_EQ(ret, true);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, IsNeededFix_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.attributesFixVersion = 1;
    bool ret = photosDao.IsNeededFix(pullData);
    EXPECT_EQ(ret, false);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, IsHiddenAsset_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.attributesSrcAlbumIds = {};
    bool ret = photosDao.IsHiddenAsset(pullData);
    EXPECT_EQ(ret, false);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, IsHiddenAsset_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.attributesSrcAlbumIds = {"hidden_album_id"};
    bool ret = photosDao.IsHiddenAsset(pullData);
    EXPECT_EQ(ret, false);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetRetryRecords_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::vector<std::string> cloudIds;
    int32_t ret = photosDao.GetRetryRecords(cloudIds);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetCheckRecords_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::vector<std::string> cloudIds;
    std::vector<PhotosPo> result = photosDao.GetCheckRecords(cloudIds);
    EXPECT_EQ(result.size(), 0);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetCheckRecords_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::vector<std::string> cloudIds = {"cloud_id_1", "cloud_id_2"};
    std::vector<PhotosPo> result = photosDao.GetCheckRecords(cloudIds);
    EXPECT_EQ(result.size(), 0);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetCreatedRecords_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    int32_t size = 10;
    std::vector<PhotosPo> createdRecords;
    int32_t ret = photosDao.GetCreatedRecords(size, createdRecords);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetMetaModifiedRecords_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    int32_t size = 10;
    std::vector<PhotosPo> cloudRecordPoList;
    int32_t ret = photosDao.GetMetaModifiedRecords(size, cloudRecordPoList);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetFileModifiedRecords_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    int32_t size = 10;
    std::vector<PhotosPo> cloudRecordPoList;
    int32_t ret = photosDao.GetFileModifiedRecords(size, cloudRecordPoList);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetDeletedRecordsAsset_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    int32_t size = 10;
    std::vector<PhotosPo> cloudRecordPoList;
    int32_t ret = photosDao.GetDeletedRecordsAsset(size, cloudRecordPoList);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetCopyRecords_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    int32_t size = 10;
    std::vector<PhotosPo> copyRecords;
    int32_t ret = photosDao.GetCopyRecords(size, copyRecords);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateLocalAlbumMap_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::string cloudId = "test_cloud_id";
    int32_t ret = photosDao.UpdateLocalAlbumMap(cloudId);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, SetRetry_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::string cloudId = "test_cloud_id";
    int32_t ret = photosDao.SetRetry(cloudId);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, ClearPhotoFailedRecords_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    int32_t ret = photosDao.ClearPhotoFailedRecords();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdatePhotoVisible_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    int32_t ret = photosDao.UpdatePhotoVisible();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateAlbumReplacedSignal_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::vector<std::string> albumIdVector = {"1", "2", "3"};
    int32_t ret = photosDao.UpdateAlbumReplacedSignal(albumIdVector);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, BatchQueryLocal_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::vector<CloudMediaPullDataDto> datas;
    std::vector<std::string> columns = {PhotoColumn::MEDIA_ID, PhotoColumn::MEDIA_NAME};
    int32_t rowCount = 0;
    auto resultSet = photosDao.BatchQueryLocal(datas, columns, rowCount, CleanType::TYPE_NOT_CLEAN);
    EXPECT_EQ(resultSet, nullptr);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetAllSysAlbums_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::vector<std::string> subtypes;
    std::vector<std::string> columns = {PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_NAME};
    auto resultSet = photosDao.GetAllSysAlbums(subtypes, columns);
    EXPECT_NE(resultSet, nullptr);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetAllSysAlbums_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::vector<std::string> subtypes = {"1", "2"};
    std::vector<std::string> columns = {PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_NAME};
    auto resultSet = photosDao.GetAllSysAlbums(subtypes, columns);
    EXPECT_NE(resultSet, nullptr);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, JudgeConflict_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    KeyData localKeyData;
    KeyData cloudKeyData;
    localKeyData.displayName = "test.jpg";
    cloudKeyData.displayName = "test.jpg";
    localKeyData.isize = 1000;
    cloudKeyData.isize = 1000;
    localKeyData.exifRotateValue = 0;
    cloudKeyData.exifRotateValue = 0;
    localKeyData.sourceAlbum = "";
    localKeyData.lPath = "/Pictures/test";
    cloudKeyData.lPath = "/Pictures/test";
    bool ret = photosDao.JudgeConflict(pullData, localKeyData, cloudKeyData);
    EXPECT_EQ(ret, true);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, JudgeConflict_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    KeyData localKeyData;
    KeyData cloudKeyData;
    localKeyData.displayName = "test.jpg";
    cloudKeyData.displayName = "test2.jpg";
    bool ret = photosDao.JudgeConflict(pullData, localKeyData, cloudKeyData);
    EXPECT_EQ(ret, false);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, JudgeConflict_Test_003, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    KeyData localKeyData;
    KeyData cloudKeyData;
    localKeyData.displayName = "test.jpg";
    cloudKeyData.displayName = "test.jpg";
    localKeyData.isize = 1000;
    cloudKeyData.isize = 2000;
    bool ret = photosDao.JudgeConflict(pullData, localKeyData, cloudKeyData);
    EXPECT_EQ(ret, false);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateRecordToDatabase_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localFileId = 1;
    pullData.basicCloudVersion = 1;
    pullData.localDisplayName = "test.jpg";
    pullData.localPath = "/storage/test.jpg";
    pullData.localDirty = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    pullData.basicCreatedTime = 1000;
    pullData.dateTaken = 1000;
    pullData.localExifRotate = 0;
    pullData.exifRotate = 0;
    pullData.basicFileType = FILE_TYPE_IMAGE;
    pullData.attributesSrcAlbumIds = {};
    pullData.propertiesSourcePath = "";
    pullData.basicRecycledTime = 0;
    pullData.localFileId = 1;
    pullData.attributesFileId = 1;
    
    bool isLocal = true;
    bool mtimeChanged = true;
    std::set<std::string> refreshAlbums;
    std::vector<int32_t> stats(StatsIndex::DELETE_RECORDS_COUNT + 1);
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.UpdateRecordToDatabase(pullData, isLocal, mtimeChanged,
        refreshAlbums, stats, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateRecordToDatabase_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localFileId = 1;
    pullData.basicCloudVersion = 1;
    pullData.localDisplayName = "test.jpg";
    pullData.localPath = "/storage/test.jpg";
    pullData.localDirty = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    pullData.basicCreatedTime = 1000;
    pullData.dateTaken = 1000;
    pullData.localExifRotate = 0;
    pullData.exifRotate = 0;
    pullData.basicFileType = FILE_TYPE_IMAGE;
    pullData.attributesSrcAlbumIds = {};
    pullData.propertiesSourcePath = "";
    pullData.basicRecycledTime = 0;
    pullData.localFileId = 1;
    pullData.attributesFileId = 1;
    
    bool isLocal = false;
    bool mtimeChanged = false;
    std::set<std::string> refreshAlbums;
    std::vector<int32_t> stats(StatsIndex::DELETE_RECORDS_COUNT + 1);
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.UpdateRecordToDatabase(pullData, isLocal, mtimeChanged,
        refreshAlbums, stats, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, ConflictDataMerge_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicCloudVersion = 1;
    pullData.localDisplayName = "test.jpg";
    pullData.localPath = "/storage/test.jpg";
    pullData.basicFileType = FILE_TYPE_IMAGE;
    pullData.attributesSrcAlbumIds = {};
    pullData.propertiesSourcePath = "";
    pullData.basicRecycledTime = 0;
    
    std::string fullPath = "/storage/test.jpg";
    bool cloudStd = true;
    std::set<int32_t> albumIds;
    std::set<std::string> refreshAlbums;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.ConflictDataMerge(pullData, fullPath, cloudStd,
        albumIds, refreshAlbums, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, ConflictDataMerge_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicCloudVersion = 1;
    pullData.localDisplayName = "test.jpg";
    pullData.localPath = "/storage/test.jpg";
    pullData.basicFileType = FILE_TYPE_IMAGE;
    pullData.attributesSrcAlbumIds = {};
    pullData.propertiesSourcePath = "";
    pullData.basicRecycledTime = 0;
    
    std::string fullPath = "/storage/test.jpg";
    bool cloudStd = false;
    std::set<int32_t> albumIds;
    std::set<std::string> refreshAlbums;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.ConflictDataMerge(pullData, fullPath, cloudStd,
        albumIds, refreshAlbums, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetInsertParams_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicCloudVersion = 1;
    pullData.basicDisplayName = "test.jpg";
    pullData.localPath = "/storage/test.jpg";
    pullData.basicFileType = FILE_TYPE_IMAGE;
    pullData.attributesSrcAlbumIds = {};
    pullData.propertiesSourcePath = "";
    pullData.basicRecycledTime = 0;
    
    std::map<std::string, int> recordAnalysisAlbumMaps;
    std::map<std::string, std::set<int>> recordAlbumMaps;
    std::set<std::string> refreshAlbums;
    std::vector<NativeRdb::ValuesBucket> insertFiles;
    
    int32_t ret = photosDao.GetInsertParams(pullData, recordAnalysisAlbumMaps,
        recordAlbumMaps, refreshAlbums, insertFiles);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateFixDB_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicCloudVersion = 1;
    pullData.basicDisplayName = "test.jpg";
    pullData.localPath = "/storage/test.jpg";
    pullData.basicFileType = FILE_TYPE_IMAGE;
    pullData.attributesSrcAlbumIds = {};
    pullData.propertiesSourcePath = "";
    pullData.basicRecycledTime = 0;
    pullData.attributesFixVersion = 0;
    
    NativeRdb::ValuesBucket values;
    int32_t albumId = 0;
    std::set<int32_t> albumIds;
    std::set<std::string> refreshAlbums;
    
    int32_t ret = photosDao.UpdateFixDB(pullData, values, albumId, albumIds, refreshAlbums);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateFixDB_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicCloudVersion = 1;
    pullData.basicDisplayName = "test.jpg";
    pullData.localPath = "/storage/test.jpg";
    pullData.basicFileType = FILE_TYPE_IMAGE;
    pullData.attributesSrcAlbumIds = {};
    pullData.propertiesSourcePath = "";
    pullData.basicRecycledTime = 1000;
    pullData.attributesFixVersion = -1;
    
    NativeRdb::ValuesBucket values;
    int32_t albumId = 0;
    std::set<int32_t> albumIds;
    std::set<std::string> refreshAlbums;
    
    int32_t ret = photosDao.UpdateFixDB(pullData, values, albumId, albumIds, refreshAlbums);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, HandleExifRotateDownloadAsset_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto data;
    data.exifRotate = 0;
    data.propertiesRotate = 0;
    data.attributesMediaType = static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE);
    
    NativeRdb::ValuesBucket valuebucket;
    photosDao.HandleExifRotateDownloadAsset(data, valuebucket);
    EXPECT_EQ(data.exifRotate, 0);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, HandleExifRotateDownloadAsset_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto data;
    data.exifRotate = 1;
    data.propertiesRotate = 90;
    data.attributesMediaType = static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE);
    
    NativeRdb::ValuesBucket valuebucket;
    photosDao.HandleExifRotateDownloadAsset(data, valuebucket);
    EXPECT_EQ(data.exifRotate, 1);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, HandleExifRotateDownloadAsset_Test_003, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto data;
    data.exifRotate = 0;
    data.propertiesRotate = 0;
    data.attributesMediaType = static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO);
    
    NativeRdb::ValuesBucket valuebucket;
    photosDao.HandleExifRotateDownloadAsset(data, valuebucket);
    EXPECT_EQ(data.exifRotate, 0);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, HandleExifRotateDownloadMetaData_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto data;
    data.localExifRotate = 0;
    data.exifRotate = 0;
    data.attributesMediaType = static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE);
    
    NativeRdb::ValuesBucket valuebucket;
    photosDao.HandleExifRotateDownloadMetaData(data, valuebucket);
    EXPECT_EQ(data.exifRotate, 0);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateFileRecordsInTransaction_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::vector<NativeRdb::ValuesBucket> updateFiles;
    std::vector<int32_t> cloudFileIdlist;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.UpdateFileRecordsInTransaction(updateFiles, cloudFileIdlist, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateFileRecordsInTransaction_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::vector<NativeRdb::ValuesBucket> updateFiles;
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::MEDIA_ID, 1);
    updateFiles.push_back(values);
    
    std::vector<int32_t> cloudFileIdlist = {1};
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.UpdateFileRecordsInTransaction(updateFiles,
        cloudFileIdlist, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, BatchUpdateFile_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::map<std::string, int> recordAnalysisAlbumMaps;
    std::map<std::string, std::set<int>> recordAlbumMaps;
    std::vector<NativeRdb::ValuesBucket> updateFiles;
    std::vector<int32_t> cloudFileIdlist;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.BatchUpdateFile(recordAnalysisAlbumMaps, recordAlbumMaps,
        updateFiles, photoRefresh, cloudFileIdlist);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, BatchUpdateFile_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::map<std::string, int> recordAnalysisAlbumMaps = {{"cloud_id_1", 1}};
    std::map<std::string, std::set<int>> recordAlbumMaps = {{"cloud_id_2", {1, 2}}};
    std::vector<NativeRdb::ValuesBucket> updateFiles;
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::MEDIA_ID, 1);
    updateFiles.push_back(values);
    std::vector<int32_t> cloudFileIdlist = {1};
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.BatchUpdateFile(recordAnalysisAlbumMaps, recordAlbumMaps,
        updateFiles, photoRefresh, cloudFileIdlist);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, OnModifyPhotoRecord_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    PhotosDto record;
    record.cloudId = "test_cloud_id";
    record.version = 1;
    record.metaDateModified = 1000;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.OnModifyPhotoRecord(record, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, OnModifyPhotoRecord_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    PhotosDto record;
    record.cloudId = "";
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.OnModifyPhotoRecord(record, photoRefresh);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateFdirtyVersion_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    PhotosDto record;
    record.cloudId = "test_cloud_id";
    record.version = 1;
    record.metaDateModified = 1000;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.UpdateFdirtyVersion(record, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, OnDeleteRecordsAsset_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    PhotosDto record;
    record.dkRecordId = "test_cloud_id";
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.OnDeleteRecordsAsset(record, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, OnCopyPhotoRecord_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    PhotosDto record;
    record.fileId = 1;
    record.path = "/storage/test.jpg";
    record.cloudVersion = 1;
    record.cloudId = "test_cloud_id";
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.OnCopyPhotoRecord(record, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, ClearCloudInfo_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::string cloudId = "test_cloud_id";
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.ClearCloudInfo(cloudId, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, DeleteFileNotExistPhoto_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::string path = "/storage/test.jpg";
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.DeleteFileNotExistPhoto(path, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, HandleNotExistAlbumRecord_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    PhotosDto record;
    record.fileId = 1;
    record.cloudId = "test_cloud_id";
    
    int32_t ret = photosDao.HandleNotExistAlbumRecord(record);
    EXPECT_EQ(ret, E_DATA);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdatePhotoCreatedRecord_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    PhotosDto record;
    record.fileId = 1;
    record.cloudId = "test_cloud_id";
    record.version = 1;
    record.metaDateModified = 1000;
    record.editedTimeMs = 1000;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.UpdatePhotoCreatedRecord(record, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, AddRemoveAlbumCloudId_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    int32_t fileId = 1;
    int32_t ownerAlbumId = 1;
    PhotosPo record;
    
    int32_t ret = photosDao.AddRemoveAlbumCloudId(rdbStore, fileId, ownerAlbumId, record);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateFailRecordsCloudId_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    PhotosDto record;
    record.fileId = 1;
    record.cloudId = "test_cloud_id";
    record.serverErrorCode = static_cast<int32_t>(ServerErrorCode::NETWORK_ERROR);
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.UpdateFailRecordsCloudId(record, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateFailRecordsCloudId_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    PhotosDto record;
    record.fileId = 1;
    record.cloudId = "test_cloud_id";
    record.serverErrorCode = static_cast<int32_t>(ServerErrorCode::RENEW_RESOURCE);
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.UpdateFailRecordsCloudId(record, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, RepushDuplicatedPhoto_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    PhotosDto photo;
    photo.fileId = 1;
    
    int32_t ret = photosDao.RepushDuplicatedPhoto(photo);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, QueryAnalysisAlbum_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::string cloudId = "test_cloud_id";
    std::vector<std::string> analysisAlbumIds;
    
    int32_t ret = photosDao.QueryAnalysisAlbum(cloudId, analysisAlbumIds);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, QueryAnalysisAlbum_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::string cloudId = "";
    std::vector<std::string> analysisAlbumIds;
    
    int32_t ret = photosDao.QueryAnalysisAlbum(cloudId, analysisAlbumIds);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, IsAlbumCloud_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    bool isUpload = true;
    PhotoAlbumPo albumInfo;
    albumInfo.dirty = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    
    bool ret = photosDao.IsAlbumCloud(isUpload, albumInfo);
    EXPECT_EQ(ret, true);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, IsAlbumCloud_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    bool isUpload = false;
    PhotoAlbumPo albumInfo;
    
    bool ret = photosDao.IsAlbumCloud(isUpload, albumInfo);
    EXPECT_EQ(ret, true);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, IsAlbumCloud_Test_003, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    bool isUpload = true;
    PhotoAlbumPo albumInfo;
    albumInfo.dirty = static_cast<int32_t>(DirtyType::TYPE_NEW);
    
    bool ret = photosDao.IsAlbumCloud(isUpload, albumInfo);
    EXPECT_EQ(ret, false);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdatePhoto_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::string whereClause = "file_id = ?";
    std::vector<std::string> whereArgs = {"1"};
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_DIRTY, 0);
    int32_t changedRows = 0;
    
    int32_t ret = photosDao.UpdatePhoto(whereClause, whereArgs, values, changedRows);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, FixAlbumIdToBeOtherAlbumId_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    int32_t albumId = 0;
    
    int32_t ret = photosDao.FixAlbumIdToBeOtherAlbumId(albumId);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetSourceAlbumFromPath_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localPath = "/storage/test.jpg";
    
    int32_t albumId = 0;
    std::set<int32_t> cloudMapIds;
    SafeMap<std::string, std::pair<int32_t, std::string>> lpathToIdMap;
    
    photosDao.GetSourceAlbumFromPath(pullData, albumId, cloudMapIds, lpathToIdMap);
    EXPECT_EQ(pullData.cloudId, "test_cloud_id");
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetSourceAlbumForMerge_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.attributesSrcAlbumIds = {};
    
    std::vector<std::string> albumCloudIds;
    SafeMap<std::string, std::pair<int32_t, std::string>> lpathToIdMap;
    
    int32_t ret = photosDao.GetSourceAlbumForMerge(pullData, albumCloudIds, lpathToIdMap);
    EXPECT_EQ(ret, E_INVALID_VALUES);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetSourceAlbum_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.attributesSrcAlbumIds = {};
    pullData.basicFileType = FILE_TYPE_IMAGE;
    
    int32_t albumId = 0;
    std::set<int32_t> cloudMapIds;
    bool isHidden = false;
    SafeMap<std::string, int32_t> cloudToLocalMap;
    
    int32_t ret = photosDao.GetSourceAlbum(pullData, albumId, cloudMapIds, isHidden, cloudToLocalMap);
    EXPECT_EQ(ret, E_INVALID_VALUES);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateRecordValues_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicCloudVersion = 1;
    pullData.basicCreatedTime = 1000;
    
    NativeRdb::ValuesBucket values;
    photosDao.GetUpdateRecordValues(pullData, values);
    EXPECT_EQ(pullData.cloudId, "test_cloud_id");
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateRecordToDatabasePrepare_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localDisplayName = "test.jpg";
    pullData.localPath = "/storage/test.jpg";
    pullData.exifRotate = 0;
    pullData.propertiesRotate = 0;
    pullData.basicFileType = FILE_TYPE_IMAGE;
    
    bool isLocal = true;
    bool mtimeChanged = true;
    NativeRdb::ValuesBucket values;
    
    photosDao.UpdateRecordToDatabasePrepare(pullData, isLocal, mtimeChanged, values);
    EXPECT_EQ(pullData.cloudId, "test_cloud_id");
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateRecordToDatabasePrepare_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localDisplayName = "test.jpg";
    pullData.localPath = "/storage/test.jpg";
    pullData.exifRotate = 0;
    pullData.propertiesRotate = 0;
    pullData.basicFileType = FILE_TYPE_IMAGE;
    
    bool isLocal = false;
    bool mtimeChanged = false;
    NativeRdb::ValuesBucket values;
    
    photosDao.UpdateRecordToDatabasePrepare(pullData, isLocal, mtimeChanged, values);
    EXPECT_EQ(pullData.cloudId, "test_cloud_id");
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateProxy_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    int changedRows = 0;
    NativeRdb::ValuesBucket row;
    row.PutInt(PhotoColumn::PHOTO_DIRTY, 0);
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_CLOUD_ID, "test_cloud_id");
    std::string cloudId = "test_cloud_id";
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int ret = photosDao.UpdateProxy(changedRows, row, predicates, cloudId, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateProxy_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    int changedRows = 0;
    std::string table = PhotoColumn::PHOTOS_TABLE;
    NativeRdb::ValuesBucket row;
    row.Put(PhotoColumn::PHOTO_DIRTY, 0);
    std::string whereClause = "cloud_id = ?";
    std::vector<std::string> args = {"test_cloud_id"};
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int ret = photosDao.UpdateProxy(changedRows, table,
        row, whereClause, args, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, JudgeConflict_Test_004, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.attributesSrcAlbumIds = {"album_cloud_id_1"};
    KeyData localKeyData;
    KeyData cloudKeyData;
    localKeyData.displayName = "test.jpg";
    cloudKeyData.displayName = "test.jpg";
    localKeyData.isize = 1000;
    cloudKeyData.isize = 1000;
    localKeyData.exifRotateValue = 0;
    cloudKeyData.exifRotateValue = 0;
    localKeyData.sourceAlbum = "album_cloud_id_1";
    localKeyData.lPath = "/Pictures/test";
    cloudKeyData.lPath = "/Pictures/test";
    bool ret = photosDao.JudgeConflict(pullData, localKeyData, cloudKeyData);
    EXPECT_EQ(ret, true);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, JudgeConflict_Test_006, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.attributesSrcAlbumIds = {"album_cloud_id_1"};
    KeyData localKeyData;
    KeyData cloudKeyData;
    cloudKeyData.displayName = "test.jpg";
    localKeyData.isize = 1000;
    cloudKeyData.isize = 1000;
    localKeyData.exifRotateValue = 0;
    cloudKeyData.exifRotateValue = 0;
    localKeyData.sourceAlbum = "album_cloud_id_2";
    localKeyData.lPath = "/Pictures/test1";
    cloudKeyData.lPath = "/Pictures/test2";
    bool ret = photosDao.JudgeConflict(pullData, localKeyData, cloudKeyData);
    EXPECT_EQ(ret, false);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateFixDB_Test_003, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicCloudVersion = 1;
    pullData.basicDisplayName = "test.jpg";
    pullData.localPath = "/storage/test.jpg";
    pullData.basicFileType = FILE_TYPE_IMAGE;
    pullData.attributesSrcAlbumIds = {};
    pullData.propertiesSourcePath = "/Pictures/test";
    pullData.basicRecycledTime = 0;
    pullData.attributesFixVersion = 0;
    
    NativeRdb::ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_HIDDEN, 1);
    int32_t albumId = 0;
    std::set<int32_t> albumIds;
    std::set<std::string> refreshAlbums;
    
    int32_t ret = photosDao.UpdateFixDB(pullData, values,
        albumId, albumIds, refreshAlbums);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateFixDB_Test_004, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicCloudVersion = 1;
    pullData.basicDisplayName = "test.jpg";
    pullData.localPath = "/storage/test.jpg";
    pullData.basicFileType = FILE_TYPE_IMAGE;
    pullData.attributesSrcAlbumIds = {};
    pullData.propertiesSourcePath = "";
    pullData.basicRecycledTime = 0;
    pullData.attributesFixVersion = 0;
    
    NativeRdb::ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_HIDDEN, 1);
    values.Put(PhotoColumn::PHOTO_HIDDEN_TIME, 1000);
    int32_t albumId = 0;
    std::set<int32_t> albumIds;
    std::set<std::string> refreshAlbums;
    
    int32_t ret = photosDao.UpdateFixDB(pullData, values,
        albumId, albumIds, refreshAlbums);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, HandleExifRotateDownloadAsset_Test_004, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto data;
    data.exifRotate = 1;
    data.propertiesRotate = 0;
    data.attributesMediaType = static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE);
    
    NativeRdb::ValuesBucket valuebucket;
    photosDao.HandleExifRotateDownloadAsset(data, valuebucket);
    EXPECT_EQ(data.exifRotate, 1);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, HandleExifRotateDownloadAsset_Test_005, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto data;
    data.exifRotate = 1;
    data.propertiesRotate = 90;
    data.attributesMediaType = static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE);
    
    NativeRdb::ValuesBucket valuebucket;
    photosDao.HandleExifRotateDownloadAsset(data, valuebucket);
    EXPECT_EQ(data.exifRotate, 1);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, HandleExifRotateDownloadMetaData_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto data;
    data.localExifRotate = 0;
    data.exifRotate = 90;
    data.attributesMediaType = static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE);
    
    NativeRdb::ValuesBucket valuebucket;
    photosDao.HandleExifRotateDownloadMetaData(data, valuebucket);
    EXPECT_EQ(data.exifRotate, 90);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetSourceAlbumForMerge_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.attributesSrcAlbumIds = {"album_cloud_id_1", "hidden_album_id"};
    
    std::vector<std::string> albumCloudIds;
    SafeMap<std::string, std::pair<int32_t, std::string>> lpathToIdMap;
    
    int32_t ret = photosDao.GetSourceAlbumForMerge(pullData, albumCloudIds, lpathToIdMap);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetSourceAlbumForMerge_Test_003, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.attributesSrcAlbumIds = {"album_cloud_id_1"};
    
    std::vector<std::string> albumCloudIds;
    SafeMap<std::string, std::pair<int32_t, std::string>> lpathToIdMap;
    lpathToIdMap.EnsureInsert("/pictures/test", std::make_pair(1, "album_cloud_id_1"));
    
    int32_t ret = photosDao.GetSourceAlbumForMerge(pullData, albumCloudIds, lpathToIdMap);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetSourceAlbum_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.attributesSrcAlbumIds = {"album_cloud_id_1"};
    pullData.basicFileType = FILE_TYPE_IMAGE;
    
    int32_t albumId = 0;
    std::set<int32_t> cloudMapIds;
    bool isHidden = false;
    SafeMap<std::string, int32_t> cloudToLocalMap;
    cloudToLocalMap.EnsureInsert("album_cloud_id_1", 1);
    
    int32_t ret = photosDao.GetSourceAlbum(pullData, albumId,
        cloudMapIds, isHidden, cloudToLocalMap);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetSourceAlbum_Test_003, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.attributesSrcAlbumIds = {"album_cloud_id_1", "album_cloud_id_2"};
    pullData.basicFileType = FILE_TYPE_IMAGE;
    
    int32_t albumId = 0;
    std::set<int32_t> cloudMapIds;
    bool isHidden = false;
    SafeMap<std::string, int32_t> cloudToLocalMap;
    cloudToLocalMap.EnsureInsert("album_cloud_id_1", 1);
    cloudToLocalMap.EnsureInsert("album_cloud_id_2", 2);
    
    int32_t ret = photosDao.GetSourceAlbum(pullData, albumId,
        cloudMapIds, isHidden, cloudToLocalMap);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetSourceAlbum_Test_004, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.attributesSrcAlbumIds = {"physical_album_id_1"};
    pullData.basicFileType = FILE_TYPE_IMAGE;
    
    int32_t albumId = 0;
    std::set<int32_t> cloudMapIds;
    bool isHidden = false;
    SafeMap<std::string, int32_t> cloudToLocalMap;
    cloudToLocalMap.EnsureInsert("physical_album_id_1", 1);
    
    int32_t ret = photosDao.GetSourceAlbum(pullData, albumId,
        cloudMapIds, isHidden, cloudToLocalMap);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetSourceAlbum_Test_005, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.attributesSrcAlbumIds = {"screenshot_album_id"};
    pullData.basicFileType = FILE_TYPE_VIDEO;
    
    int32_t albumId = 0;
    std::set<int32_t> cloudMapIds;
    bool isHidden = false;
    SafeMap<std::string, int32_t> cloudToLocalMap;
    cloudToLocalMap.EnsureInsert("screenshot_album_id", 1);
    
    int32_t ret = photosDao.GetSourceAlbum(pullData, albumId, cloudMapIds, isHidden, cloudToLocalMap);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateAlbumReplacedSignal_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::vector<std::string> albumIdVector = {"1", "2", "3"};
    int32_t ret = photosDao.UpdateAlbumReplacedSignal(albumIdVector);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateAlbumReplacedSignal_Test_003, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::vector<std::string> albumIdVector = {};
    int32_t ret = photosDao.UpdateAlbumReplacedSignal(albumIdVector);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateFileRecordsInTransaction_Test_003, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    std::vector<NativeRdb::ValuesBucket> updateFiles;
    NativeRdb::ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_ID, 1);
    updateFiles.push_back(values);
    
    std::vector<int32_t> cloudFileIdlist = {1};
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.UpdateFileRecordsInTransaction(updateFiles,
        cloudFileIdlist, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, OnModifyPhotoRecord_Test_003, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    PhotosDto record;
    record.cloudId = "test_cloud_id";
    record.version = 1;
    record.metaDateModified = 1000;
    record.localInfoOp = PhotosPo();
    record.localInfoOp.value().metaDateModified = 2000;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.OnModifyPhotoRecord(record, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateFdirtyVersion_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    PhotosDto record;
    record.cloudId = "test_cloud_id";
    record.version = 1;
    record.metaDateModified = 1000;
    record.localInfoOp = PhotosPo();
    record.localInfoOp.value().metaDateModified = 2000;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.UpdateFdirtyVersion(record, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, HandleNotExistAlbumRecord_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    PhotosDto record;
    record.fileId = 1;
    record.cloudId = "test_cloud_id";
    record.localInfoOp = PhotosPo();
    record.localInfoOp.value().ownerAlbumId = 1;
    
    int32_t ret = photosDao.HandleNotExistAlbumRecord(record);
    EXPECT_EQ(ret, E_DATA);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateFailRecordsCloudId_Test_003, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    PhotosDto record;
    record.fileId = 1;
    record.cloudId = "test_cloud_id";
    record.serverErrorCode = static_cast<int32_t>(ServerErrorCode::NETWORK_ERROR);
    record.localInfoOp = PhotosPo();
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.UpdateFailRecordsCloudId(record, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdatePhotoCreatedRecord_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    PhotosDto record;
    record.fileId = 1;
    record.cloudId = "test_cloud_id";
    record.version = 1;
    record.metaDateModified = 1000;
    record.editedTimeMs = 2000;
    record.localInfoOp = PhotosPo();
    record.localInfoOp.value().dateModified = 2000;
    record.localInfoOp.value().metaDateModified = 2000;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.UpdatePhotoCreatedRecord(record, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdatePhotoCreatedRecord_Test_003, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    PhotosDto record;
    record.fileId = 1;
    record.cloudId = "test_cloud_id";
    record.version = 1;
    record.metaDateModified = 1000;
    record.editedTimeMs = 1000;
    record.localInfoOp = PhotosPo();
    record.localInfoOp.value().dateModified = 1000;
    record.localInfoOp.value().metaDateModified = 1000;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.UpdatePhotoCreatedRecord(record, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, IsFileTimeChanged_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    PhotosDto record;
    record.cloudId = "test_cloud_id";
    record.editedTimeMs = 1000;
    record.localInfoOp = PhotosPo();
    record.localInfoOp.value().dateModified = 2000;
    
    bool ret = photosDao.IsFileTimeChanged(record);
    EXPECT_EQ(ret, true);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, IsFileTimeChanged_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    PhotosDto record;
    record.cloudId = "test_cloud_id";
    record.editedTimeMs = 1000;
    record.localInfoOp = PhotosPo();
    record.localInfoOp.value().dateModified = 1000;
    
    bool ret = photosDao.IsFileTimeChanged(record);
    EXPECT_EQ(ret, false);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, IsMetaTimeChanged_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    PhotosDto record;
    record.cloudId = "test_cloud_id";
    record.metaDateModified = 1000;
    record.localInfoOp = PhotosPo();
    record.localInfoOp.value().metaDateModified = 2000;
    
    bool ret = photosDao.IsMetaTimeChanged(record);
    EXPECT_EQ(ret, true);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, IsMetaTimeChanged_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    PhotosDto record;
    record.cloudId = "test_cloud_id";
    record.metaDateModified = 1000;
    record.localInfoOp = PhotosPo();
    record.localInfoOp.value().metaDateModified = 1000;
    
    bool ret = photosDao.IsMetaTimeChanged(record);
    EXPECT_EQ(ret, false);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, IsLocalFileExists_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    PhotosDto record;
    record.path = "/storage/test.jpg";
    
    bool ret = photosDao.IsLocalFileExists(record);
    EXPECT_EQ(ret, false);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, OnFdirtyHandlePosition_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    PhotosDto record;
    record.path = "/storage/test.jpg";
    
    NativeRdb::ValuesBucket valuesBucket;
    int32_t ret = photosDao.OnFdirtyHandlePosition(record, valuesBucket);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, FillThumbStatus_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    NativeRdb::ValuesBucket values;
    bool mtimeChanged = true;
    
    int32_t ret = photosDao.FillThumbStatus(values, mtimeChanged);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, FillThumbStatus_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    NativeRdb::ValuesBucket values;
    bool mtimeChanged = false;
    
    int32_t ret = photosDao.FillThumbStatus(values, mtimeChanged);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateRecordToDatabase_Test_003, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localFileId = 1;
    pullData.basicCloudVersion = 1;
    pullData.localDisplayName = "test.heif";
    pullData.localPath = "/storage/test.heif";
    pullData.localDirty = static_cast<int32_t>(DirtyType::TYPE_TDIRTY);
    pullData.basicCreatedTime = 1000;
    pullData.dateTaken = 1000;
    pullData.localExifRotate = 0;
    pullData.exifRotate = 0;
    pullData.basicFileType = FILE_TYPE_IMAGE;
    pullData.attributesSrcAlbumIds = {};
    pullData.propertiesSourcePath = "";
    pullData.basicRecycledTime = 0;
    pullData.localFileId = 1;
    pullData.attributesFileId = 1;
    
    bool isLocal = true;
    bool mtimeChanged = false;
    std::set<std::string> refreshAlbums;
    std::vector<int32_t> stats(StatsIndex::DELETE_RECORDS_COUNT + 1);
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.UpdateRecordToDatabase(pullData, isLocal,
        mtimeChanged, refreshAlbums, stats, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateRecordToDatabase_Test_004, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localFileId = 1;
    pullData.basicCloudVersion = 1;
    pullData.localDisplayName = "test.heic";
    pullData.localPath = "/storage/test.heic";
    pullData.localDirty = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    pullData.basicCreatedTime = 1000;
    pullData.dateTaken = 1000;
    pullData.localExifRotate = 0;
    pullData.exifRotate = 0;
    pullData.basicFileType = FILE_TYPE_IMAGE;
    pullData.attributesSrcAlbumIds = {};
    pullData.propertiesSourcePath = "";
    pullData.basicRecycledTime = 0;
    pullData.localFileId = 1;
    pullData.attributesFileId = 1;
    
    bool isLocal = true;
    bool mtimeChanged = true;
    std::set<std::string> refreshAlbums;
    std::vector<int32_t> stats(StatsIndex::DELETE_RECORDS_COUNT + 1);
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.UpdateRecordToDatabase(pullData, isLocal,
        mtimeChanged, refreshAlbums, stats, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, UpdateRecordToDatabase_Test_005, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localFileId = 1;
    pullData.basicCloudVersion = 1;
    pullData.localDisplayName = "test.jpg";
    pullData.localPath = "/storage/test.jpg";
    pullData.localDirty = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    pullData.basicCreatedTime = 1000;
    pullData.dateTaken = 2000;
    pullData.localExifRotate = 0;
    pullData.exifRotate = 0;
    pullData.basicFileType = FILE_TYPE_IMAGE;
    pullData.attributesSrcAlbumIds = {};
    pullData.propertiesSourcePath = "";
    pullData.basicRecycledTime = 0;
    pullData.localFileId = 1;
    pullData.attributesFileId = 1;
    
    bool isLocal = true;
    bool mtimeChanged = true;
    std::set<std::string> refreshAlbums;
    std::vector<int32_t> stats(StatsIndex::DELETE_RECORDS_COUNT + 1);
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.UpdateRecordToDatabase(pullData, isLocal,
        mtimeChanged, refreshAlbums, stats, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, ConflictDataMerge_Test_003, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicCloudVersion = 1;
    pullData.localDisplayName = "test.jpg";
    pullData.localPath = "/storage/test.jpg";
    pullData.basicFileType = FILE_TYPE_IMAGE;
    pullData.attributesSrcAlbumIds = {};
    pullData.propertiesSourcePath = "";
    pullData.basicRecycledTime = 0;
    pullData.attributesFixVersion = 0;
    
    std::string fullPath = "/storage/test.jpg";
    bool cloudStd = true;
    std::set<int32_t> albumIds;
    std::set<std::string> refreshAlbums;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.ConflictDataMerge(pullData, fullPath, cloudStd,
        albumIds, refreshAlbums, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, ConflictDataMerge_Test_004, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicCloudVersion = 1;
    pullData.localDisplayName = "test.jpg";
    pullData.localPath = "/storage/test.jpg";
    pullData.basicFileType = FILE_TYPE_IMAGE;
    pullData.attributesSrcAlbumIds = {};
    pullData.propertiesSourcePath = "";
    pullData.basicRecycledTime = 0;
    pullData.attributesFixVersion = -1;
    
    std::string fullPath = "/storage/test.jpg";
    bool cloudStd = true;
    std::set<int32_t> albumIds = {1};
    std::set<std::string> refreshAlbums;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    
    int32_t ret = photosDao.ConflictDataMerge(pullData, fullPath, cloudStd,
        albumIds, refreshAlbums, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetInsertParams_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicCloudVersion = 1;
    pullData.basicDisplayName = "test.jpg";
    pullData.localPath = "/storage/test.jpg";
    pullData.basicFileType = FILE_TYPE_IMAGE;
    pullData.attributesSrcAlbumIds = {};
    pullData.propertiesSourcePath = "";
    pullData.basicRecycledTime = 0;
    
    std::map<std::string, int> recordAnalysisAlbumMaps;
    std::map<std::string, std::set<int>> recordAlbumMaps;
    std::set<std::string> refreshAlbums;
    std::vector<NativeRdb::ValuesBucket> insertFiles;
    
    int32_t ret = photosDao.GetInsertParams(pullData, recordAnalysisAlbumMaps,
        recordAlbumMaps, refreshAlbums, insertFiles);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetInsertParams_Test_003, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicCloudVersion = 1;
    pullData.basicDisplayName = "test.jpg";
    pullData.localPath = "/storage/test.jpg";
    pullData.basicFileType = FILE_TYPE_IMAGE;
    pullData.attributesSrcAlbumIds = {};
    pullData.propertiesSourcePath = "";
    pullData.basicRecycledTime = 1000;
    
    std::map<std::string, int> recordAnalysisAlbumMaps;
    std::map<std::string, std::set<int>> recordAlbumMaps;
    std::set<std::string> refreshAlbums;
    std::vector<NativeRdb::ValuesBucket> insertFiles;
    
    int32_t ret = photosDao.GetInsertParams(pullData, recordAnalysisAlbumMaps,
        recordAlbumMaps, refreshAlbums, insertFiles);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, GetInsertParams_Test_004, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicCloudVersion = 1;
    pullData.basicDisplayName = "test.jpg";
    pullData.localPath = "/storage/test.jpg";
    pullData.basicFileType = FILE_TYPE_IMAGE;
    pullData.attributesSrcAlbumIds = {"hidden_album_id"};
    pullData.propertiesSourcePath = "";
    pullData.basicRecycledTime = 0;
    
    std::map<std::string, int> recordAnalysisAlbumMaps;
    std::map<std::string, std::set<int>> recordAlbumMaps;
    std::set<std::string> refreshAlbums;
    std::vector<NativeRdb::ValuesBucket> insertFiles;
    
    int32_t ret = photosDao.GetInsertParams(pullData, recordAnalysisAlbumMaps,
        recordAlbumMaps, refreshAlbums, insertFiles);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, BatchInsert_Test_003, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    int64_t outRowId = 0;
    std::vector<NativeRdb::ValuesBucket> initialBatchValues;
    NativeRdb::ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_DIRTY, 1);
    initialBatchValues.push_back(values);
    
    int32_t ret = photosDao.BatchInsert(outRowId,
        PhotoColumn::PHOTOS_TABLE, initialBatchValues);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, BatchInsert_Test_004, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    int64_t outRowId = 0;
    std::vector<NativeRdb::ValuesBucket> initialBatchValues;
    for (int i = 0; i < 20; i++) {
        NativeRdb::ValuesBucket values;
        values.Put(PhotoColumn::PHOTO_DIRTY, 1);
        initialBatchValues.push_back(values);
    }
    
    int32_t ret = photosDao.BatchInsert(outRowId,
        PhotoColumn::PHOTOS_TABLE, initialBatchValues);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, HandleWatchRelatedFields_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.attributesUniqueId = "";
    
    NativeRdb::ValuesBucket values;
    photosDao.HandleWatchRelatedFields(pullData, values);
    
    NativeRdb::ValueObject val;
    EXPECT_EQ(values.GetObject(PhotoColumn::UNIQUE_ID, val), true);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, HandleWatchRelatedFields_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.attributesUniqueId = "-1";
    
    NativeRdb::ValuesBucket values;
    photosDao.HandleWatchRelatedFields(pullData, values);
    
    NativeRdb::ValueObject val;
    EXPECT_EQ(values.GetObject(PhotoColumn::UNIQUE_ID, val), true);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, HandleWatchRelatedFields_Test_003, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.attributesUniqueId = "valid_unique_id";
    
    NativeRdb::ValuesBucket values;
    photosDao.HandleWatchRelatedFields(pullData, values);
    
    NativeRdb::ValueObject val;
    EXPECT_EQ(values.GetObject(PhotoColumn::UNIQUE_ID, val), false);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, HandleWatchRelatedFields_Test_004, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localPhotosPoOp = PhotosPo();
    pullData.localPhotosPoOp.value().uniqueId = "local_unique_id";
    
    NativeRdb::ValuesBucket values;
    photosDao.HandleWatchRelatedFields(pullData, values);
    
    NativeRdb::ValueObject val;
    EXPECT_EQ(values.GetObject(PhotoColumn::UNIQUE_ID, val), true);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, HandleWatchRelatedFieldsForUpdate_Test_001, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    
    NativeRdb::ValuesBucket values;
    photosDao.HandleWatchRelatedFieldsForUpdate(pullData, values);
    
    NativeRdb::ValueObject val;
    EXPECT_EQ(values.GetObject(PhotoColumn::UNIQUE_ID, val), false);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, HandleWatchRelatedFieldsForUpdate_Test_002, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.attributesUniqueId = "";
    pullData.localPhotosPoOp = PhotosPo();
    pullData.localPhotosPoOp.value().uniqueId = "";
    
    NativeRdb::ValuesBucket values;
    photosDao.HandleWatchRelatedFieldsForUpdate(pullData, values);
    
    NativeRdb::ValueObject val;
    EXPECT_EQ(values.GetObject(PhotoColumn::UNIQUE_ID, val), true);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, HandleWatchRelatedFieldsForUpdate_Test_003, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.attributesUniqueId = "";
    pullData.localPhotosPoOp = PhotosPo();
    pullData.localPhotosPoOp.value().uniqueId = "-1";
    
    NativeRdb::ValuesBucket values;
    photosDao.HandleWatchRelatedFieldsForUpdate(pullData, values);
    
    NativeRdb::ValueObject val;
    EXPECT_EQ(values.GetObject(PhotoColumn::UNIQUE_ID, val), true);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, HandleWatchRelatedFieldsForUpdate_Test_004, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.attributesUniqueId = "";
    pullData.localPhotosPoOp = PhotosPo();
    pullData.localPhotosPoOp.value().uniqueId = "valid_local_unique_id";
    
    NativeRdb::ValuesBucket values;
    photosDao.HandleWatchRelatedFieldsForUpdate(pullData, values);
    
    NativeRdb::ValueObject val;
    std::string uniqueId;
    EXPECT_EQ(values.GetObject(PhotoColumn::UNIQUE_ID, val), true);
    EXPECT_EQ(val.GetString(uniqueId), E_OK);
    EXPECT_EQ(uniqueId, "valid_local_unique_id");
}

HWTEST_F(CloudMediaSyncServiceDaoTest, HandleWatchRelatedFieldsForUpdate_Test_005, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localPhotosPoOp = PhotosPo();
    pullData.localPhotosPoOp.value().uniqueId = "local_unique_id";
    pullData.localPhotosPoOp.value().photoRiskStatus = 2;
    
    NativeRdb::ValuesBucket values;
    photosDao.HandleWatchRelatedFieldsForUpdate(pullData, values);
    
    NativeRdb::ValueObject val;
    int32_t riskStatus;
    EXPECT_EQ(values.GetObject(PhotoColumn::PHOTO_RISK_STATUS, val), true);
    EXPECT_EQ(val.GetInt(riskStatus), E_OK);
    EXPECT_EQ(riskStatus, 2);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, HandleWatchRelatedFieldsForUpdate_Test_006, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localPhotosPoOp = PhotosPo();
    pullData.localPhotosPoOp.value().uniqueId = "local_unique_id";
    pullData.localPhotosPoOp.value().photoRiskStatus = 1;
    
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_RISK_STATUS, 1);
    photosDao.HandleWatchRelatedFieldsForUpdate(pullData, values);
    
    NativeRdb::ValueObject val;
    int32_t isCritical;
    EXPECT_EQ(values.GetObject(PhotoColumn::PHOTO_IS_CRITICAL, val), true);
    EXPECT_EQ(val.GetInt(isCritical), E_OK);
    EXPECT_EQ(isCritical, 0);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, HandleWatchRelatedFieldsForUpdate_Test_007, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localPhotosPoOp = PhotosPo();
    pullData.localPhotosPoOp.value().uniqueId = "local_unique_id";
    pullData.localPhotosPoOp.value().photoRiskStatus = 2;
    
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_RISK_STATUS, 2);
    photosDao.HandleWatchRelatedFieldsForUpdate(pullData, values);
    
    NativeRdb::ValueObject val;
    int32_t isCritical;
    EXPECT_EQ(values.GetObject(PhotoColumn::PHOTO_IS_CRITICAL, val), true);
    EXPECT_EQ(val.GetInt(isCritical), E_OK);
    EXPECT_EQ(isCritical, 1);
}

HWTEST_F(CloudMediaSyncServiceDaoTest, HandleWatchRelatedFieldsForUpdate_Test_008, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localPhotosPoOp = PhotosPo();
    pullData.localPhotosPoOp.value().uniqueId = "local_unique_id";
    pullData.localPhotosPoOp.value().packageName = "test_package";
    
    NativeRdb::ValuesBucket values;
    photosDao.HandleWatchRelatedFieldsForUpdate(pullData, values);
    
    NativeRdb::ValueObject val;
    std::string packageName;
    EXPECT_EQ(values.GetObject(MediaColumn::MEDIA_PACKAGE_NAME, val), true);
    EXPECT_EQ(val.GetString(packageName), E_OK);
    EXPECT_EQ(packageName, "test_package");
}

HWTEST_F(CloudMediaSyncServiceDaoTest, HandleWatchRelatedFieldsForUpdate_Test_009, TestSize.Level1)
{
    CloudMediaPhotosDao photosDao;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localPhotosPoOp = PhotosPo();
    pullData.localPhotosPoOp.value().uniqueId = "local_unique_id";
    pullData.localPhotosPoOp.value().packageName = "";
    
    NativeRdb::ValuesBucket values;
    photosDao.HandleWatchRelatedFieldsForUpdate(pullData, values);
    
    NativeRdb::ValueObject val;
    EXPECT_EQ(values.GetObject(MediaColumn::MEDIA_PACKAGE_NAME, val), false);
}
} // namespace OHOS::Media::CloudSync