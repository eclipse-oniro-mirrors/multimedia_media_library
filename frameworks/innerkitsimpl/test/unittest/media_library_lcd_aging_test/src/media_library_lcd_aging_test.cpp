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

#include "media_library_lcd_aging_test.h"

#include <fstream>

#include "lcd_aging_manager.h"
#include "lcd_aging_utils.h"
#include "lcd_aging_worker.h"
#include "media_column.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "exif_rotate_utils.h"
#include "userfile_manager_types.h"
#include "parameters.h"

#include "ithumbnail_helper.h"
#include "media_file_utils.h"
#include "media_upgrade.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "thumbnail_source_loading.h"
#include "values_bucket.h"
#include "vision_db_sqls.h"
#include "story_db_sqls.h"
#include "lcd_aging_service.h"

using namespace std;
using namespace OHOS;
using namespace::testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
static shared_ptr<MediaLibraryRdbStore> g_rdbStore = nullptr;
static int64_t g_id;
const int64_t DATE_TAKEN_TEST_VALUE = 1756111539577;
const string TEST_IMAGE_PATH = "/storage/cloud/files/Photo/1/CreateImageThumbnailTest_001.jpg";
const int64_t THUMB_DENTRY_SIZE = 2 * 1024 * 1024;
const std::string FILE_NAME_LCD = "LCD.jpg";
const int64_t THIRTY_DAYS_MS = 30LL * 24 * 60 * 60 * 1000;
constexpr int32_t E_AGING_STOP = 2;
constexpr int32_t E_AGING_INTERRUPT = 3;
const std::string TEST_MEDIA_BACKUP_FLAG = "multimedia.medialibrary.backupFlag";
const std::string TEST_MEDIA_RESTORE_FLAG = "multimedia.medialibrary.restoreFlag";
const std::string TEST_CLOUDSYNC_SWITCH_STATUS_KEY = "persist.kernel.cloudsync.switch_status";
const std::string TEST_CLONE_STATE = "persist.dataclone.state";
const std::string TEST_CLONE_FLAG = "multimedia.medialibrary.cloneFlag";

class TddRdbOpenCallback : public NativeRdb::RdbOpenCallback {
public:
    int OnCreate(NativeRdb::RdbStore &rdbStore) override
    {
        return E_OK;
    }
    int OnUpgrade(NativeRdb::RdbStore &rdbStore, int oldVersion, int newVersion) override
    {
        return E_OK;
    }
};

struct PhotoTestData {
    int64_t dateTaken = DATE_TAKEN_TEST_VALUE - THIRTY_DAYS_MS;
    int32_t position = 2;
    int32_t syncStatus = 0;
    int32_t cleanFlag = 0;
    int64_t timePending = 0;
    int32_t isTemp = 0;
    int32_t isFavorite = 0;
    int32_t thumbStatus = 0;
    int64_t dateTrashed = 0;
    int64_t realLcdVisitTime = DATE_TAKEN_TEST_VALUE - THIRTY_DAYS_MS;
};

static void InitRdbStore()
{
    const string dbPath = "/data/test/medialibrary_thumbnail_rdb_utils_test.db";
    NativeRdb::RdbStoreConfig config(dbPath);
    TddRdbOpenCallback openCallback;

    int32_t ret = MediaLibraryUnitTestUtils::InitUnistore(config, 1, openCallback);
    ASSERT_EQ(ret, E_OK);
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);

    ret = g_rdbStore->ExecuteSql(PhotoUpgrade::CREATE_PHOTO_TABLE);
    ASSERT_EQ(ret, NativeRdb::E_OK);
    
    ret = g_rdbStore->ExecuteSql(PhotoExtUpgrade::CREATE_PHOTO_EXT_TABLE);
    ASSERT_EQ(ret, NativeRdb::E_OK);
    
    ret = g_rdbStore->ExecuteSql(CREATE_ANALYSIS_ALBUM);
    ASSERT_EQ(ret, NativeRdb::E_OK);
    
    ret = g_rdbStore->ExecuteSql(CREATE_ANALYSIS_ALBUM_MAP);
    ASSERT_EQ(ret, NativeRdb::E_OK);
    
    ret = g_rdbStore->ExecuteSql(CREATE_HIGHLIGHT_ALBUM_TABLE);
    ASSERT_EQ(ret, NativeRdb::E_OK);
    
    ret = g_rdbStore->ExecuteSql(CREATE_HIGHLIGHT_COVER_INFO_TABLE);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, 0);
    values.PutLong(MediaColumn::MEDIA_DATE_TAKEN, DATE_TAKEN_TEST_VALUE);
    ret = g_rdbStore->Insert(g_id, PhotoColumn::PHOTOS_TABLE, values);
    ASSERT_EQ(ret, NativeRdb::E_OK);
}

static void DeleteRdbStore()
{
    string dropSql = "DROP TABLE IF EXISTS " + PhotoColumn::PHOTOS_TABLE + ";";
    int32_t ret = g_rdbStore->ExecuteSql(dropSql);
    MEDIA_INFO_LOG("Drop photos table ret: %{public}d", ret == NativeRdb::E_OK);
    
    dropSql = "DROP TABLE IF EXISTS " + PhotoExtColumn::PHOTOS_EXT_TABLE + ";";
    ret = g_rdbStore->ExecuteSql(dropSql);
    MEDIA_INFO_LOG("Drop photos_ext table ret: %{public}d", ret == NativeRdb::E_OK);
    
    dropSql = "DROP TABLE IF EXISTS AnalysisAlbum;";
    ret = g_rdbStore->ExecuteSql(dropSql);
    MEDIA_INFO_LOG("Drop AnalysisAlbum table ret: %{public}d", ret == NativeRdb::E_OK);
    
    dropSql = "DROP TABLE IF EXISTS AnalysisPhotoMap;";
    ret = g_rdbStore->ExecuteSql(dropSql);
    MEDIA_INFO_LOG("Drop AnalysisPhotoMap table ret: %{public}d", ret == NativeRdb::E_OK);
    
    dropSql = "DROP TABLE IF EXISTS tab_highlight_album;";
    ret = g_rdbStore->ExecuteSql(dropSql);
    MEDIA_INFO_LOG("Drop tab_highlight_album table ret: %{public}d", ret == NativeRdb::E_OK);
    
    dropSql = "DROP TABLE IF EXISTS tab_highlight_cover_info;";
    ret = g_rdbStore->ExecuteSql(dropSql);
    MEDIA_INFO_LOG("Drop tab_highlight_cover_info table ret: %{public}d", ret == NativeRdb::E_OK);
    
    MediaLibraryUnitTestUtils::StopUnistore();
}

static int32_t InsertPhotoData(int64_t &testId, const PhotoTestData &data)
{
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, 0);
    values.PutLong(MediaColumn::MEDIA_DATE_TAKEN, data.dateTaken);
    values.PutInt(PhotoColumn::PHOTO_POSITION, data.position);
    values.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, data.syncStatus);
    values.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, data.cleanFlag);
    values.PutLong(MediaColumn::MEDIA_TIME_PENDING, data.timePending);
    values.PutInt(PhotoColumn::PHOTO_IS_TEMP, data.isTemp);
    values.PutInt(MediaColumn::MEDIA_IS_FAV, data.isFavorite);
    values.PutInt(PhotoColumn::PHOTO_THUMB_STATUS, data.thumbStatus);
    values.PutLong(MediaColumn::MEDIA_DATE_TRASHED, data.dateTrashed);
    values.PutLong(PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, data.realLcdVisitTime);
    return g_rdbStore->Insert(testId, PhotoColumn::PHOTOS_TABLE, values);
}

static int32_t InsertTrashedPhotoData(int64_t &testId)
{
    PhotoTestData data;
    data.dateTrashed = DATE_TAKEN_TEST_VALUE;
    return InsertPhotoData(testId, data);
}

static int32_t InsertNotTrashedPhotoData(int64_t &testId)
{
    PhotoTestData data;
    return InsertPhotoData(testId, data);
}

static int32_t InsertMultiplePhotoData(vector<int64_t> &testIds, int32_t count, bool isTrashed = false)
{
    for (int32_t i = 0; i < count; i++) {
        PhotoTestData data;
        data.dateTaken = DATE_TAKEN_TEST_VALUE - THIRTY_DAYS_MS + i;
        if (isTrashed) {
            data.dateTrashed = DATE_TAKEN_TEST_VALUE;
        }
        int64_t testId;
        int32_t ret = InsertPhotoData(testId, data);
        if (ret != NativeRdb::E_OK) {
            return ret;
        }
        testIds.push_back(testId);
    }
    return NativeRdb::E_OK;
}

static void DeletePhotoDataById(int64_t testId)
{
    string deleteSql = "DELETE FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE file_id = " + to_string(testId);
    g_rdbStore->ExecuteSql(deleteSql);
}

static void DeleteMultiplePhotoData(const vector<int64_t> &testIds)
{
    for (auto testId : testIds) {
        DeletePhotoDataById(testId);
    }
}

void MediaLibraryLcdAgingTest::SetUpTestCase(void)
{
    InitRdbStore();
}

void MediaLibraryLcdAgingTest::TearDownTestCase(void)
{
    DeleteRdbStore();
}

void MediaLibraryLcdAgingTest::SetUp()
{
}

void MediaLibraryLcdAgingTest::TearDown(void) {}

HWTEST_F(MediaLibraryLcdAgingTest, lcd_aging_ConvertAgingFileToDentryFile_Single_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("lcd_aging_ConvertAgingFileToDentryFile_Single_test_002: test convert single aging file");
    LcdAgingFileInfo agingFileInfo;
    agingFileInfo.fileId = 1;
    agingFileInfo.cloudId = "test_cloud_id_001";
    agingFileInfo.path = "/storage/cloud/files/Photo/test.jpg";
    agingFileInfo.localLcdPath = "/storage/cloud/files/.thumbs/Photo/LCD/test.jpg";
    agingFileInfo.mediaType = static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE);
    agingFileInfo.orientation = 0;
    agingFileInfo.exifRotate = static_cast<int32_t>(ExifRotateType::TOP_LEFT);
    agingFileInfo.dateModified = 1234567890;
    agingFileInfo.lcdFileSize = 102400;
    agingFileInfo.hasExThumbnail = false;
    
    vector<LcdAgingFileInfo> agingFileInfos = {agingFileInfo};
    
    auto dentryFileInfos = LcdAgingUtils::ConvertAgingFileToDentryFile(agingFileInfos);
    EXPECT_EQ(dentryFileInfos.size(), 1);
    EXPECT_EQ(dentryFileInfos[0].cloudId, "test_cloud_id_001");
    EXPECT_EQ(dentryFileInfos[0].path, "/storage/cloud/files/Photo/test.jpg");
    EXPECT_EQ(dentryFileInfos[0].size, THUMB_DENTRY_SIZE);
    EXPECT_EQ(dentryFileInfos[0].modifiedTime, 1234567890);
    EXPECT_EQ(dentryFileInfos[0].fileName, FILE_NAME_LCD);
    EXPECT_EQ(dentryFileInfos[0].fileType, "LCD");
}

HWTEST_F(MediaLibraryLcdAgingTest, lcd_aging_ConvertAgingFileToDentryFile_ExThumbnail_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("lcd_aging_ConvertAgingFileToDentryFile_ExThumbnail_test_003: test convert single aging ex file");
    LcdAgingFileInfo agingFileInfo;
    agingFileInfo.fileId = 2;
    agingFileInfo.cloudId = "test_cloud_id_002";
    agingFileInfo.path = "/storage/cloud/files/Photo/test2.jpg";
    agingFileInfo.localLcdPath = "/storage/cloud/files/.thumbs/Photo/LCD/test2.jpg";
    agingFileInfo.localLcdExPath = "/storage/cloud/files/.thumbs/Photo/LCD_EX/test2.jpg";
    agingFileInfo.mediaType = static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE);
    agingFileInfo.orientation = 90;
    agingFileInfo.exifRotate = static_cast<int32_t>(ExifRotateType::RIGHT_TOP);
    agingFileInfo.dateModified = 1234567891;
    agingFileInfo.lcdFileSize = 204800;
    agingFileInfo.hasExThumbnail = true;
    
    vector<LcdAgingFileInfo> agingFileInfos = {agingFileInfo};
    
    auto dentryFileInfos = LcdAgingUtils::ConvertAgingFileToDentryFile(agingFileInfos);
    EXPECT_EQ(dentryFileInfos.size(), 1);
    EXPECT_EQ(dentryFileInfos[0].cloudId, "test_cloud_id_002");
    EXPECT_EQ(dentryFileInfos[0].path, "/storage/cloud/files/Photo/test2.jpg");
    EXPECT_EQ(dentryFileInfos[0].size, THUMB_DENTRY_SIZE);
    EXPECT_EQ(dentryFileInfos[0].modifiedTime, 1234567891);
    EXPECT_EQ(dentryFileInfos[0].fileName, FILE_NAME_LCD);
    EXPECT_EQ(dentryFileInfos[0].fileType, "THM_EX/LCD");
}

HWTEST_F(MediaLibraryLcdAgingTest, lcd_aging_HasExThumbnail_ImageWithOrientation_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("lcd_aging_HasExThumbnail_ImageWithOrientation_test_001: test has ex thumbnail");
    LcdAgingFileInfo agingFileInfo;
    agingFileInfo.mediaType = static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE);
    agingFileInfo.orientation = 90;
    agingFileInfo.exifRotate = static_cast<int32_t>(ExifRotateType::TOP_LEFT);
    
    bool result = LcdAgingUtils::HasExThumbnail(agingFileInfo);
    EXPECT_TRUE(result);
}

HWTEST_F(MediaLibraryLcdAgingTest, lcd_aging_HasExThumbnail_ImageWithExifRotate_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("lcd_aging_HasExThumbnail_ImageWithExifRotate_test_002: test has ex thumbnail");
    LcdAgingFileInfo agingFileInfo;
    agingFileInfo.mediaType = static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE);
    agingFileInfo.orientation = 0;
    agingFileInfo.exifRotate = static_cast<int32_t>(ExifRotateType::RIGHT_TOP);
    
    bool result = LcdAgingUtils::HasExThumbnail(agingFileInfo);
    EXPECT_TRUE(result);
}

HWTEST_F(MediaLibraryLcdAgingTest, lcd_aging_HasExThumbnail_ImageNoRotate_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("lcd_aging_HasExThumbnail_ImageNoRotate_test_003: test no ex thumbnail for image without rotation");
    LcdAgingFileInfo agingFileInfo;
    agingFileInfo.mediaType = static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE);
    agingFileInfo.orientation = 0;
    agingFileInfo.exifRotate = static_cast<int32_t>(ExifRotateType::TOP_LEFT);
    
    bool result = LcdAgingUtils::HasExThumbnail(agingFileInfo);
    EXPECT_FALSE(result);
}

HWTEST_F(MediaLibraryLcdAgingTest, lcd_aging_HasExThumbnail_Video_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("lcd_aging_HasExThumbnail_Video_test_004: test no ex thumbnail for video type");
    LcdAgingFileInfo agingFileInfo;
    agingFileInfo.mediaType = static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO);
    agingFileInfo.orientation = 90;
    agingFileInfo.exifRotate = static_cast<int32_t>(ExifRotateType::RIGHT_TOP);
    
    bool result = LcdAgingUtils::HasExThumbnail(agingFileInfo);
    EXPECT_FALSE(result);
}

HWTEST_F(MediaLibraryLcdAgingTest, lcd_aging_ConvertAgingFileToDentryFile_AllFields_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("lcd_aging_ConvertAgingFileToDentryFile_AllFields_test_005: test convert with all fields populated");
    LcdAgingFileInfo agingFileInfo;
    agingFileInfo.fileId = 999;
    agingFileInfo.cloudId = "test_cloud_id_full";
    agingFileInfo.path = "/storage/cloud/files/Photo/full_test.jpg";
    agingFileInfo.localLcdPath = "/storage/cloud/files/.thumbs/Photo/LCD/full_test.jpg";
    agingFileInfo.localLcdExPath = "/storage/cloud/files/.thumbs/Photo/LCD_EX/full_test.jpg";
    agingFileInfo.mediaType = static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE);
    agingFileInfo.orientation = 90;
    agingFileInfo.exifRotate = static_cast<int32_t>(ExifRotateType::RIGHT_TOP);
    agingFileInfo.thumbnailReady = 1;
    agingFileInfo.dateModified = 9999999999;
    agingFileInfo.lcdFileSize = 999999;
    agingFileInfo.needFixLcdFileSize = true;
    agingFileInfo.hasExThumbnail = true;
    
    vector<LcdAgingFileInfo> agingFileInfos = {agingFileInfo};
    
    auto dentryFileInfos = LcdAgingUtils::ConvertAgingFileToDentryFile(agingFileInfos);
    EXPECT_EQ(dentryFileInfos.size(), 1);
    EXPECT_EQ(dentryFileInfos[0].cloudId, "test_cloud_id_full");
    EXPECT_EQ(dentryFileInfos[0].path, "/storage/cloud/files/Photo/full_test.jpg");
    EXPECT_EQ(dentryFileInfos[0].size, THUMB_DENTRY_SIZE);
    EXPECT_EQ(dentryFileInfos[0].modifiedTime, 9999999999);
    EXPECT_EQ(dentryFileInfos[0].fileName, FILE_NAME_LCD);
    EXPECT_EQ(dentryFileInfos[0].fileType, "THM_EX/LCD");
}

HWTEST_F(MediaLibraryLcdAgingTest, lcd_aging_manager_GetFileIdFromAgingFiles_Single_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("lcd_aging_manager_GetFileIdFromAgingFiles_Single_test_002: test get file ids");
    auto& manager = LcdAgingManager::GetInstance();
    LcdAgingFileInfo agingFileInfo;
    agingFileInfo.fileId = 100;
    
    vector<LcdAgingFileInfo> agingFileInfos = {agingFileInfo};
    auto fileIds = manager.GetFileIdFromAgingFiles(agingFileInfos);
    
    EXPECT_EQ(fileIds.size(), 1);
    EXPECT_EQ(fileIds[0], "100");
}

HWTEST_F(MediaLibraryLcdAgingTest, GetFileIdFromAgingFiles_EmptyInput_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileIdFromAgingFiles_EmptyInput_test_001: test with empty input returns empty vector");
    auto& manager = LcdAgingManager::GetInstance();
    
    vector<LcdAgingFileInfo> agingFileInfos;
    auto fileIds = manager.GetFileIdFromAgingFiles(agingFileInfos);
    
    EXPECT_EQ(fileIds.size(), 0);
    EXPECT_TRUE(fileIds.empty());
}

HWTEST_F(MediaLibraryLcdAgingTest, GetFileIdFromAgingFiles_MultipleFiles_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileIdFromAgingFiles_MultipleFiles_test_003: test with multiple LcdAgingFileInfo");
    auto& manager = LcdAgingManager::GetInstance();
    
    vector<LcdAgingFileInfo> agingFileInfos;
    for (int i = 0; i < 5; i++) {
        LcdAgingFileInfo info;
        info.fileId = 100 + i;
        agingFileInfos.push_back(info);
    }
    
    auto fileIds = manager.GetFileIdFromAgingFiles(agingFileInfos);
    
    EXPECT_EQ(fileIds.size(), 5);
    EXPECT_EQ(fileIds[0], "100");
    EXPECT_EQ(fileIds[1], "101");
    EXPECT_EQ(fileIds[2], "102");
    EXPECT_EQ(fileIds[3], "103");
    EXPECT_EQ(fileIds[4], "104");
}

HWTEST_F(MediaLibraryLcdAgingTest, GetFailedFileIds_EmptyInput_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFailedFileIds_EmptyInput_test_001: test with empty agingFileInfos returns empty vector");
    auto& manager = LcdAgingManager::GetInstance();
    
    vector<LcdAgingFileInfo> agingFileInfos;
    vector<string> failCloudIds = {"cloud_id_1"};
    
    auto failFileIds = manager.GetFailedFileIds(agingFileInfos, failCloudIds);
    
    EXPECT_EQ(failFileIds.size(), 0);
    EXPECT_TRUE(failFileIds.empty());
}

HWTEST_F(MediaLibraryLcdAgingTest, GetFailedFileIds_EmptyFailCloudIds_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFailedFileIds_EmptyFailCloudIds_test_002: test with empty failCloudIds returns empty vector");
    auto& manager = LcdAgingManager::GetInstance();
    
    vector<LcdAgingFileInfo> agingFileInfos;
    LcdAgingFileInfo info;
    info.fileId = 100;
    info.cloudId = "cloud_id_1";
    agingFileInfos.push_back(info);
    
    vector<string> failCloudIds;
    
    auto failFileIds = manager.GetFailedFileIds(agingFileInfos, failCloudIds);
    
    EXPECT_EQ(failFileIds.size(), 0);
    EXPECT_TRUE(failFileIds.empty());
}

HWTEST_F(MediaLibraryLcdAgingTest, GetFailedFileIds_SingleMatch_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFailedFileIds_SingleMatch_test_003: test with single matching cloudId");
    auto& manager = LcdAgingManager::GetInstance();
    
    vector<LcdAgingFileInfo> agingFileInfos;
    LcdAgingFileInfo info;
    info.fileId = 100;
    info.cloudId = "cloud_id_1";
    agingFileInfos.push_back(info);
    
    vector<string> failCloudIds = {"cloud_id_1"};
    
    auto failFileIds = manager.GetFailedFileIds(agingFileInfos, failCloudIds);
    
    EXPECT_EQ(failFileIds.size(), 1);
    EXPECT_EQ(failFileIds[0], "100");
}

HWTEST_F(MediaLibraryLcdAgingTest, GetFailedFileIds_NoMatch_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFailedFileIds_NoMatch_test_004: test with no matching cloudId");
    auto& manager = LcdAgingManager::GetInstance();
    
    vector<LcdAgingFileInfo> agingFileInfos;
    LcdAgingFileInfo info;
    info.fileId = 100;
    info.cloudId = "cloud_id_1";
    agingFileInfos.push_back(info);
    
    vector<string> failCloudIds = {"cloud_id_2", "cloud_id_3"};
    
    auto failFileIds = manager.GetFailedFileIds(agingFileInfos, failCloudIds);
    
    EXPECT_EQ(failFileIds.size(), 0);
    EXPECT_TRUE(failFileIds.empty());
}

HWTEST_F(MediaLibraryLcdAgingTest, GetFailedFileIds_MultipleMatches_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFailedFileIds_MultipleMatches_test_005: test with multiple matching cloudIds");
    auto& manager = LcdAgingManager::GetInstance();
    
    vector<LcdAgingFileInfo> agingFileInfos;
    for (int i = 0; i < 3; i++) {
        LcdAgingFileInfo info;
        info.fileId = 100 + i;
        info.cloudId = "cloud_id_" + to_string(i);
        agingFileInfos.push_back(info);
    }
    
    vector<string> failCloudIds = {"cloud_id_0", "cloud_id_2"};
    
    auto failFileIds = manager.GetFailedFileIds(agingFileInfos, failCloudIds);
    
    EXPECT_EQ(failFileIds.size(), 2);
    EXPECT_EQ(failFileIds[0], "100");
    EXPECT_EQ(failFileIds[1], "102");
}

HWTEST_F(MediaLibraryLcdAgingTest, GetFailedFileIds_PartialMatch_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFailedFileIds_PartialMatch_test_006: test with partial matching cloudIds");
    auto& manager = LcdAgingManager::GetInstance();
    
    vector<LcdAgingFileInfo> agingFileInfos;
    for (int i = 0; i < 5; i++) {
        LcdAgingFileInfo info;
        info.fileId = 100 + i;
        info.cloudId = "cloud_id_" + to_string(i);
        agingFileInfos.push_back(info);
    }
    
    vector<string> failCloudIds = {"cloud_id_1", "cloud_id_3"};
    
    auto failFileIds = manager.GetFailedFileIds(agingFileInfos, failCloudIds);
    
    EXPECT_EQ(failFileIds.size(), 2);
    EXPECT_EQ(failFileIds[0], "101");
    EXPECT_EQ(failFileIds[1], "103");
}

HWTEST_F(MediaLibraryLcdAgingTest, GetFailedFileIds_AllMatch_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFailedFileIds_AllMatch_test_007: test with all cloudIds matching");
    auto& manager = LcdAgingManager::GetInstance();
    
    vector<LcdAgingFileInfo> agingFileInfos;
    for (int i = 0; i < 3; i++) {
        LcdAgingFileInfo info;
        info.fileId = 100 + i;
        info.cloudId = "cloud_id_" + to_string(i);
        agingFileInfos.push_back(info);
    }
    
    vector<string> failCloudIds = {"cloud_id_0", "cloud_id_1", "cloud_id_2"};
    
    auto failFileIds = manager.GetFailedFileIds(agingFileInfos, failCloudIds);
    
    EXPECT_EQ(failFileIds.size(), 3);
    EXPECT_EQ(failFileIds[0], "100");
    EXPECT_EQ(failFileIds[1], "101");
    EXPECT_EQ(failFileIds[2], "102");
}

HWTEST_F(MediaLibraryLcdAgingTest, GetFailedFileIds_LargeCloudId_test_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFailedFileIds_LargeCloudId_test_010: test with long cloudId string");
    auto& manager = LcdAgingManager::GetInstance();
    
    vector<LcdAgingFileInfo> agingFileInfos;
    LcdAgingFileInfo info;
    info.fileId = 100;
    info.cloudId = "test-cloud-id";
    agingFileInfos.push_back(info);
    
    vector<string> failCloudIds = {"test-cloud-id"};
    
    auto failFileIds = manager.GetFailedFileIds(agingFileInfos, failCloudIds);
    
    EXPECT_EQ(failFileIds.size(), 1);
    EXPECT_EQ(failFileIds[0], "100");
}

HWTEST_F(MediaLibraryLcdAgingTest, lcd_aging_manager_DeleteLocalFile_EmptyPath_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("lcd_aging_manager_DeleteLocalFile_EmptyPath_test_001: test delete file with empty path");
    auto& manager = LcdAgingManager::GetInstance();
    int32_t ret = manager.DeleteLocalFile("");
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibraryLcdAgingTest, lcd_aging_manager_DeleteLocalFile_NonExistent_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("lcd_aging_manager_DeleteLocalFile_NonExistent_test_002: test delete non-existent file");
    auto& manager = LcdAgingManager::GetInstance();
    int32_t ret = manager.DeleteLocalFile("/data/test/non_existent_file.jpg");
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibraryLcdAgingTest, DeleteLocalFile_CreateAndDelete_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteLocalFile_CreateAndDelete_test_003: test create file then delete successfully");
    auto& manager = LcdAgingManager::GetInstance();
    const string testFilePath = "/data/test/lcd_aging_test_file_003.jpg";
    
    ofstream outFile(testFilePath);
    ASSERT_TRUE(outFile.is_open());
    outFile << "test content for lcd aging";
    outFile.close();
    
    ifstream inFile(testFilePath);
    ASSERT_TRUE(inFile.is_open());
    inFile.close();
    
    int32_t ret = manager.DeleteLocalFile(testFilePath);
    EXPECT_EQ(ret, E_OK);
    
    ifstream checkFile(testFilePath);
    EXPECT_FALSE(checkFile.is_open());
}

HWTEST_F(MediaLibraryLcdAgingTest, DeleteLocalFile_DeleteAlreadyDeleted_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteLocalFile_DeleteAlreadyDeleted_test_006: test delete same file twice");
    auto& manager = LcdAgingManager::GetInstance();
    const string testFilePath = "/data/test/lcd_aging_test_file_006.jpg";
    
    ofstream outFile(testFilePath);
    ASSERT_TRUE(outFile.is_open());
    outFile << "test content";
    outFile.close();
    
    int32_t ret1 = manager.DeleteLocalFile(testFilePath);
    EXPECT_EQ(ret1, E_OK);
    
    int32_t ret2 = manager.DeleteLocalFile(testFilePath);
    EXPECT_EQ(ret2, E_ERR);
}

HWTEST_F(MediaLibraryLcdAgingTest, lcd_aging_manager_FinishAgingTask_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("lcd_aging_manager_FinishAgingTask_test_001: test finish aging task clears all state");
    auto& manager = LcdAgingManager::GetInstance();
    manager.hasAgingLcdNumber_ = 1000;
    manager.notAgingFileIds_.push_back("1");
    manager.notAgingFileIds_.push_back("2");
    
    int32_t ret = manager.FinishAgingTask();
    
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(manager.hasAgingLcdNumber_, 0);
    EXPECT_TRUE(manager.notAgingFileIds_.empty());
}

HWTEST_F(MediaLibraryLcdAgingTest, lcd_aging_manager_DoBatchAgingLcdFile_EmptyInput_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("lcd_aging_manager_DoBatchAgingLcdFile_EmptyInput_test_001: test with empty input returns E_ERR");
    auto& manager = LcdAgingManager::GetInstance();
    std::vector<LcdAgingFileInfo> lcdAgingFileInfoList;
    std::atomic<bool> shouldStop(false);
    
    int32_t ret = manager.DoBatchAgingLcdFile(lcdAgingFileInfoList, shouldStop);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibraryLcdAgingTest, DoBatchAgingLcdFile_SingleFileInfo_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("DoBatchAgingLcdFile_SingleFileInfo_test_002: test with single LcdAgingFileInfo");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(false);
    
    LcdAgingFileInfo info;
    info.fileId = 1;
    info.path = "/storage/cloud/files/Photo/test.jpg";
    info.lcdFileSize = 102400;
    std::vector<LcdAgingFileInfo> list = {info};
    
    manager.hasAgingLcdNumber_ = 0;
    
    int32_t ret = manager.DoBatchAgingLcdFile(list, shouldStop);
    EXPECT_TRUE(ret == E_OK);
}

HWTEST_F(MediaLibraryLcdAgingTest, DoBatchAgingLcdFile_ShouldStop_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("DoBatchAgingLcdFile_ShouldStop_test_003: test with shouldStop=true returns E_AGING_STOP");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(true);
    
    LcdAgingFileInfo info;
    info.fileId = 1;
    info.path = "/storage/cloud/files/Photo/test.jpg";
    std::vector<LcdAgingFileInfo> list = {info};
    
    int32_t ret = manager.DoBatchAgingLcdFile(list, shouldStop);
    EXPECT_EQ(ret, E_AGING_STOP);
}

HWTEST_F(MediaLibraryLcdAgingTest, DoBatchAgingLcdFile_MultipleFileInfo_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("DoBatchAgingLcdFile_MultipleFileInfo_test_004: test with multiple LcdAgingFileInfo");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(false);
    
    std::vector<LcdAgingFileInfo> list;
    for (int i = 0; i < 505; i++) {
        LcdAgingFileInfo info;
        info.fileId = i + 1;
        info.path = "/storage/cloud/files/Photo/test" + to_string(i) + ".jpg";
        info.lcdFileSize = 102400 * (i + 1);
        list.push_back(info);
    }
    
    manager.hasAgingLcdNumber_ = 0;
    
    int32_t ret = manager.DoBatchAgingLcdFile(list, shouldStop);
    EXPECT_TRUE(ret == E_OK);
}

HWTEST_F(MediaLibraryLcdAgingTest, lcd_aging_manager_StateReset_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("lcd_aging_manager_StateReset_test_001: test reset manager state after operations");
    auto& manager = LcdAgingManager::GetInstance();
    
    manager.hasAgingLcdNumber_ = 500;
    manager.notAgingFileIds_.push_back("100");
    manager.notAgingFileIds_.push_back("200");
    
    manager.FinishAgingTask();
    
    EXPECT_EQ(manager.hasAgingLcdNumber_, 0);
    EXPECT_TRUE(manager.notAgingFileIds_.empty());
}

HWTEST_F(MediaLibraryLcdAgingTest, BatchAgingLcdFileTask_InitAgingTask_ReturnNoData_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("BatchAgingLcdFileTask_InitAgingTask_ReturnNoData_test_001: test returns E_NO_QUERY_DATA");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(false);

    manager.hasAgingLcdNumber_ = 100;
    manager.totalAgingLcdNumber_ = 200;
    manager.lastAgingProgress_ = 50;
    manager.notAgingFileIds_.push_back("old_id");
    
    int32_t ret = manager.BatchAgingLcdFileTask(shouldStop);
    
    EXPECT_EQ(ret, E_NO_QUERY_DATA);
    EXPECT_EQ(manager.hasAgingLcdNumber_, 0);
    EXPECT_EQ(manager.totalAgingLcdNumber_, 0);
    EXPECT_EQ(manager.lastAgingProgress_, 0);
    EXPECT_TRUE(manager.notAgingFileIds_.empty());
}

HWTEST_F(MediaLibraryLcdAgingTest, InitAgingTask_GetNeedAgingLcdSize_ReturnNoData_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("InitAgingTask_GetNeedAgingLcdSize_ReturnNoData_test_001: test when returns E_NO_QUERY_DATA");
    auto& manager = LcdAgingManager::GetInstance();
    int64_t taskSize = -1;

    manager.hasAgingLcdNumber_ = 100;
    manager.totalAgingLcdNumber_ = 200;
    manager.lastAgingProgress_ = 50;
    manager.notAgingFileIds_.push_back("old_id");
    
    int32_t ret = manager.InitAgingTask(taskSize);
    
    EXPECT_EQ(ret, E_NO_QUERY_DATA);
    EXPECT_EQ(taskSize, -1);
    EXPECT_EQ(manager.hasAgingLcdNumber_, 0);
    EXPECT_EQ(manager.totalAgingLcdNumber_, 0);
    EXPECT_EQ(manager.lastAgingProgress_, 0);
    EXPECT_TRUE(manager.notAgingFileIds_.empty());
}

HWTEST_F(MediaLibraryLcdAgingTest, InitAgingTask_WithDatabaseData_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("InitAgingTask_WithDatabaseData_test_006: test with preset database data, returns E_NO_QUERY_DATA");
    auto& manager = LcdAgingManager::GetInstance();
    int64_t taskSize = -1;

    PhotoTestData data;
    data.position = 1;
    int64_t testId;
    int32_t ret = InsertPhotoData(testId, data);
    ASSERT_EQ(ret, NativeRdb::E_OK);
    
    ret = manager.InitAgingTask(taskSize);
    
    EXPECT_EQ(ret, E_NO_QUERY_DATA);
    EXPECT_EQ(taskSize, -1);
    
    DeletePhotoDataById(testId);
}

HWTEST_F(MediaLibraryLcdAgingTest, GetNeedAgingLcdSize_EmptyDatabase_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetNeedAgingLcdSize_EmptyDatabase_test_001: test with empty database, lcd count < 50000 threshold");
    auto& manager = LcdAgingManager::GetInstance();
    int64_t taskSize = -1;
    
    int32_t ret = manager.GetNeedAgingLcdSize(taskSize);
    
    EXPECT_EQ(ret, E_NO_QUERY_DATA);
    EXPECT_EQ(taskSize, -1);
}

HWTEST_F(MediaLibraryLcdAgingTest, GetNeedAgingLcdSize_SingleRecord_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetNeedAgingLcdSize_SingleRecord_test_002: test with single record, lcd count < 50000");
    auto& manager = LcdAgingManager::GetInstance();
    int64_t taskSize = -1;

    int64_t testId;
    int32_t ret = InsertNotTrashedPhotoData(testId);
    ASSERT_EQ(ret, NativeRdb::E_OK);
    
    ret = manager.GetNeedAgingLcdSize(taskSize);
    
    EXPECT_EQ(ret, E_NO_QUERY_DATA);
    EXPECT_EQ(taskSize, -1);
    
    DeletePhotoDataById(testId);
}

HWTEST_F(MediaLibraryLcdAgingTest, ExecuteAgingLoop_EmptyDb_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ExecuteAgingLoop_EmptyDb_test_001: test with empty db");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(false);

    manager.totalAgingLcdNumber_ = 0;
    manager.hasAgingLcdNumber_ = 0;

    int32_t ret = manager.ExecuteAgingLoop(shouldStop);

    EXPECT_EQ(ret, E_NO_QUERY_DATA);
}

HWTEST_F(MediaLibraryLcdAgingTest, ExecuteAgingLoop_SmallTotal_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("ExecuteAgingLoop_SmallTotal_test_003: test with small totalAgingLcdNumber");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(false);

    manager.totalAgingLcdNumber_ = 10;
    manager.hasAgingLcdNumber_ = 0;

    int32_t ret = manager.ExecuteAgingLoop(shouldStop);

    EXPECT_EQ(ret, E_NO_QUERY_DATA);
}

HWTEST_F(MediaLibraryLcdAgingTest, ExecuteAgingLoop_ShouldStopImmediate_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("ExecuteAgingLoop_ShouldStopImmediate_test_004: test with shouldStop=true");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(true);

    manager.totalAgingLcdNumber_ = 100;
    manager.hasAgingLcdNumber_ = 0;

    int32_t ret = manager.ExecuteAgingLoop(shouldStop);

    EXPECT_EQ(ret, E_AGING_STOP);
}

HWTEST_F(MediaLibraryLcdAgingTest, ExecuteSingleBatch_NoTrashedData_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ExecuteSingleBatch_NoTrashedData_test_001: test when hasTrashedData=true but no trashed data");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(false);
    const int32_t batchSize = 10;
    bool hasTrashedData = true;

    manager.notAgingFileIds_.clear();
    manager.hasAgingLcdNumber_ = 0;

    int32_t ret = manager.ExecuteSingleBatch(batchSize, hasTrashedData, shouldStop);

    EXPECT_TRUE(ret == E_NO_QUERY_DATA);
}

HWTEST_F(MediaLibraryLcdAgingTest, ExecuteSingleBatch_HasTrashedDataFalse_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("ExecuteSingleBatch_HasTrashedDataFalse_test_002: test with hasTrashedData=false");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(false);
    const int32_t batchSize = 10;
    bool hasTrashedData = false;

    manager.notAgingFileIds_.clear();

    int32_t ret = manager.ExecuteSingleBatch(batchSize, hasTrashedData, shouldStop);

    EXPECT_TRUE(ret == E_NO_QUERY_DATA);
}

HWTEST_F(MediaLibraryLcdAgingTest, ExecuteSingleBatch_WithTrashedData_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("ExecuteSingleBatch_WithTrashedData_test_003: test with trashed data preset in db");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(false);
    const int32_t batchSize = 10;
    bool hasTrashedData = true;

    int64_t testId;
    int32_t ret = InsertTrashedPhotoData(testId);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    manager.notAgingFileIds_.clear();
    ret = manager.ExecuteSingleBatch(batchSize, hasTrashedData, shouldStop);

    EXPECT_TRUE(ret == E_NO_QUERY_DATA);
    EXPECT_TRUE(hasTrashedData == false);

    DeletePhotoDataById(testId);
}

HWTEST_F(MediaLibraryLcdAgingTest, ExecuteSingleBatch_WithNotTrashedData_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("ExecuteSingleBatch_WithNotTrashedData_test_004: test with not trashed data preset in db");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(false);
    const int32_t batchSize = 10;
    bool hasTrashedData = false;

    int64_t testId;
    int32_t ret = InsertNotTrashedPhotoData(testId);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    manager.notAgingFileIds_.clear();
    ret = manager.ExecuteSingleBatch(batchSize, hasTrashedData, shouldStop);

    EXPECT_TRUE(ret == E_NO_QUERY_DATA);

    DeletePhotoDataById(testId);
}

HWTEST_F(MediaLibraryLcdAgingTest, ExecuteSingleBatch_BothTrashedAndNotTrashed_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("ExecuteSingleBatch_BothTrashedAndNotTrashed_test_005: test with both trashed and notTrashed data");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(false);
    const int32_t batchSize = 10;
    bool hasTrashedData = true;
    vector<int64_t> testIds;

    int32_t ret = InsertMultiplePhotoData(testIds, 1, true);
    ASSERT_EQ(ret, NativeRdb::E_OK);
    
    ret = InsertMultiplePhotoData(testIds, 1, false);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    manager.notAgingFileIds_.clear();
    ret = manager.ExecuteSingleBatch(batchSize, hasTrashedData, shouldStop);

    EXPECT_TRUE(ret == E_NO_QUERY_DATA);

    DeleteMultiplePhotoData(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, ExecuteSingleBatch_LocalPosition_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("ExecuteSingleBatch_LocalPosition_test_006: test with position=1(local), should not be queried");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(false);
    const int32_t batchSize = 10;
    bool hasTrashedData = true;

    PhotoTestData data;
    data.position = 1;
    int64_t testId;
    int32_t ret = InsertPhotoData(testId, data);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    manager.notAgingFileIds_.clear();
    ret = manager.ExecuteSingleBatch(batchSize, hasTrashedData, shouldStop);

    EXPECT_TRUE(ret == E_NO_QUERY_DATA);

    DeletePhotoDataById(testId);
}

HWTEST_F(MediaLibraryLcdAgingTest, ExecuteSingleBatch_FavoriteData_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("ExecuteSingleBatch_FavoriteData_test_007: test with is_favorite=1, should be excluded");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(false);
    const int32_t batchSize = 10;
    bool hasTrashedData = true;

    PhotoTestData data;
    data.isFavorite = 1;
    int64_t testId;
    int32_t ret = InsertPhotoData(testId, data);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    manager.notAgingFileIds_.clear();
    ret = manager.ExecuteSingleBatch(batchSize, hasTrashedData, shouldStop);

    EXPECT_TRUE(ret == E_NO_QUERY_DATA);

    DeletePhotoDataById(testId);
}

HWTEST_F(MediaLibraryLcdAgingTest, ExecuteSingleBatch_NotAgingFileIds_test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("ExecuteSingleBatch_NotAgingFileIds_test_009: test with notAgingFileIds preset");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(false);
    const int32_t batchSize = 10;
    bool hasTrashedData = true;

    int64_t testId;
    int32_t ret = InsertNotTrashedPhotoData(testId);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    manager.notAgingFileIds_.clear();
    manager.notAgingFileIds_.push_back(to_string(testId));
    ret = manager.ExecuteSingleBatch(batchSize, hasTrashedData, shouldStop);

    EXPECT_TRUE(ret == E_NO_QUERY_DATA);

    DeletePhotoDataById(testId);
}

HWTEST_F(MediaLibraryLcdAgingTest, ExecuteSingleBatch_ShouldStop_test_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("ExecuteSingleBatch_ShouldStop_test_010: test with shouldStop=true");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(true);
    const int32_t batchSize = 10;
    bool hasTrashedData = true;

    int64_t testId;
    int32_t ret = InsertTrashedPhotoData(testId);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    manager.notAgingFileIds_.clear();
    manager.hasAgingLcdNumber_ = 0;

    ret = manager.ExecuteSingleBatch(batchSize, hasTrashedData, shouldStop);

    EXPECT_EQ(ret, E_NO_QUERY_DATA);
    DeletePhotoDataById(testId);
}

HWTEST_F(MediaLibraryLcdAgingTest, DoBatchAgingLcdFileInternal_EmptyInput_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("DoBatchAgingLcdFileInternal_EmptyInput_test_001: test with empty input returns E_ERR");
    auto& manager = LcdAgingManager::GetInstance();
    std::vector<LcdAgingFileInfo> lcdAgingFileInfoList;
    int64_t agingSuccessSize = 0;
    std::vector<std::string> failFileIds;
    
    int32_t ret = manager.DoBatchAgingLcdFileInternal(lcdAgingFileInfoList, agingSuccessSize, failFileIds);
    EXPECT_EQ(ret, E_ERR);
    EXPECT_EQ(agingSuccessSize, 0);
}

HWTEST_F(MediaLibraryLcdAgingTest, DoBatchAgingLcdFileInternal_SingleValidRecord_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("DoBatchAgingLcdFileInternal_SingleValidRecord_test_002: test with single valid database record");
    auto& manager = LcdAgingManager::GetInstance();
    int64_t agingSuccessSize = 0;

    int64_t testId;
    int32_t ret = InsertNotTrashedPhotoData(testId);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    LcdAgingFileInfo info;
    info.fileId = testId;
    info.path = "/storage/cloud/files/Photo/test.jpg";
    info.lcdFileSize = 102400;
    info.cloudId = "test-cloud-id";
    std::vector<LcdAgingFileInfo> list = {info};
    std::vector<std::string> failFileIds;

    ret = manager.DoBatchAgingLcdFileInternal(list, agingSuccessSize, failFileIds);
    EXPECT_EQ(ret, E_ERR);

    DeletePhotoDataById(testId);
}

HWTEST_F(MediaLibraryLcdAgingTest, DoBatchAgingLcdFileInternal_WithOnceFlag_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("DoBatchAgingLcdFileInternal_WithOnceFlag_test_004: test with failFileIds output");
    auto& manager = LcdAgingManager::GetInstance();
    int64_t agingSuccessSize = 0;

    int64_t testId;
    int32_t ret = InsertNotTrashedPhotoData(testId);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    LcdAgingFileInfo info;
    info.fileId = testId;
    info.path = "/storage/cloud/files/Photo/test.jpg";
    info.lcdFileSize = 102400;
    std::vector<LcdAgingFileInfo> list = {info};
    std::vector<std::string> failFileIds;

    manager.notAgingFileIds_.clear();
    ret = manager.DoBatchAgingLcdFileInternal(list, agingSuccessSize, failFileIds);
    EXPECT_EQ(ret, E_ERR);

    DeletePhotoDataById(testId);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingWorker_IsRunning_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingWorker_IsRunning_test_002: test IsRunning returns isThreadRunning_ value");
    auto& worker = LcdAgingWorker::GetInstance();
    
    worker.isThreadRunning_.store(false);
    EXPECT_FALSE(worker.IsRunning());
    
    worker.isThreadRunning_.store(true);
    EXPECT_TRUE(worker.IsRunning());
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingWorker_StopDeepOptimizeSpace_NoTask_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingWorker_StopDeepOptimizeSpace_NoTask_test_003: test stop when no task running");
    auto& worker = LcdAgingWorker::GetInstance();
    
    worker.isThreadRunning_.store(false);
    worker.shouldStop_.store(false);
    
    int32_t ret = worker.StopDeepOptimizeSpace();
    
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingWorker_StopDeepOptimizeSpace_TaskRunning_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingWorker_StopDeepOptimizeSpace_TaskRunning_test_004: test stop sets shouldStop when running");
    auto& worker = LcdAgingWorker::GetInstance();
    
    worker.isThreadRunning_.store(true);
    worker.shouldStop_.store(false);
    
    int32_t ret = worker.StopDeepOptimizeSpace();
    
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(worker.shouldStop_.load());
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingWorker_OnClientDied_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingWorker_OnClientDied_test_005: test OnClientDied clears callback and sets shouldStop");
    auto& worker = LcdAgingWorker::GetInstance();
    
    worker.callbackProxy_ = nullptr;
    worker.shouldStop_.store(false);
    
    worker.OnClientDied();
    
    EXPECT_EQ(worker.callbackProxy_, nullptr);
    EXPECT_TRUE(worker.shouldStop_.load());
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingWorker_StartDeepOptimizeSpace_NullClient_test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingWorker_StartDeepOptimizeSpace_NullClient_test_008: test with null clientRemote");
    auto& worker = LcdAgingWorker::GetInstance();
    
    int32_t ret = worker.StartDeepOptimizeSpace(nullptr, nullptr);
    
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingWorker_CleanupInternal_test_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingWorker_CleanupInternal_test_010: test CleanupInternal clears all members");
    auto& worker = LcdAgingWorker::GetInstance();
    worker.isThreadRunning_.store(true);
    worker.shouldStop_.store(true);
    
    worker.CleanupInternal();
    
    EXPECT_EQ(worker.clientRemote_, nullptr);
    EXPECT_EQ(worker.deathRecipient_, nullptr);
    EXPECT_EQ(worker.callbackProxy_, nullptr);
    EXPECT_FALSE(worker.isThreadRunning_.load());
    EXPECT_FALSE(worker.shouldStop_.load());
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingWorker_Cleanup_test_011, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingWorker_Cleanup_test_011: test Cleanup calls CleanupInternal with mutex");
    auto& worker = LcdAgingWorker::GetInstance();
    
    worker.isThreadRunning_.store(true);
    worker.shouldStop_.store(true);
    
    worker.Cleanup();
    
    EXPECT_FALSE(worker.isThreadRunning_.load());
    EXPECT_FALSE(worker.shouldStop_.load());
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingWorker_ClientDeathRecipient_OnRemoteDied_test_017, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingWorker_ClientDeathRecipient_OnRemoteDied_test_017: test ClientDeathRecipient OnRemoteDied");
    auto& worker = LcdAgingWorker::GetInstance();
    
    worker.shouldStop_.store(false);
    
    LcdAgingWorker::ClientDeathRecipient deathRecipient(&worker);
    wptr<IRemoteObject> remote(nullptr);
    deathRecipient.OnRemoteDied(remote);
    
    EXPECT_TRUE(worker.shouldStop_.load());
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingManager_GetIsActiveLcdAging_FirstLoadTrue_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingManager_GetIsActiveLcdAging_FirstLoadTrue_test_001: first load from prefs returns true");
    auto& manager = LcdAgingManager::GetInstance();
    manager.SetIsActiveLcdAging(true);
    bool result = manager.GetIsActiveLcdAging();
    EXPECT_TRUE(result);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingManager_GetIsActiveLcdAging_FirstLoadFalse_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingManager_GetIsActiveLcdAging_FirstLoadFalse_test_002: first load from prefs returns false");
    auto& manager = LcdAgingManager::GetInstance();
    manager.SetIsActiveLcdAging(false);
    bool result = manager.GetIsActiveLcdAging();
    EXPECT_FALSE(result);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingManager_GetIsActiveLcdAging_CacheValueOne_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingManager_GetIsActiveLcdAging_CacheValueOne_test_003: cached value is 1");
    auto& manager = LcdAgingManager::GetInstance();
    manager.SetIsActiveLcdAging(true);
    bool firstResult = manager.GetIsActiveLcdAging();
    EXPECT_TRUE(firstResult);
    bool secondResult = manager.GetIsActiveLcdAging();
    EXPECT_TRUE(secondResult);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingManager_SetIsActiveLcdAging_AlternateSet_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingManager_SetIsActiveLcdAging_AlternateSet_test_005: alternate set true and false");
    auto& manager = LcdAgingManager::GetInstance();
    
    int32_t ret = manager.SetIsActiveLcdAging(true);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(manager.GetIsActiveLcdAging());

    ret = manager.SetIsActiveLcdAging(false);
    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(manager.GetIsActiveLcdAging());

    ret = manager.SetIsActiveLcdAging(true);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(manager.GetIsActiveLcdAging());
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingManager_AnalysisRemoveCloudLcd_InactiveAging_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingManager_AnalysisRemoveCloudLcd_InactiveAging_test_001: isActiveLcdAging is false");
    auto& manager = LcdAgingManager::GetInstance();
    manager.SetIsActiveLcdAging(false);
    
    vector<int64_t> fileIds = {1, 2, 3};
    int32_t ret = manager.AnalysisRemoveCloudLcd(fileIds);
    EXPECT_EQ(ret, E_OK);
    
    manager.SetIsActiveLcdAging(true);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingManager_AnalysisRemoveCloudLcd_EmptyFileIds_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingManager_AnalysisRemoveCloudLcd_EmptyFileIds_test_002: fileIds is empty");
    auto& manager = LcdAgingManager::GetInstance();
    manager.SetIsActiveLcdAging(true);
    
    vector<int64_t> fileIds;
    int32_t ret = manager.AnalysisRemoveCloudLcd(fileIds);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingManager_AnalysisRemoveCloudLcd_NoMatchingData_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingManager_AnalysisRemoveCloudLcd_NoMatchingData_test_003: no matching data in database");
    auto& manager = LcdAgingManager::GetInstance();
    manager.SetIsActiveLcdAging(true);
    
    PhotoTestData data;
    data.position = 1;
    int64_t testId;
    int32_t ret = InsertPhotoData(testId, data);
    ASSERT_EQ(ret, NativeRdb::E_OK);
    
    vector<int64_t> fileIds = {testId};
    ret = manager.AnalysisRemoveCloudLcd(fileIds);
    EXPECT_EQ(ret, E_OK);
    
    DeletePhotoDataById(testId);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingManager_AnalysisRemoveCloudLcd_MultipleFileIds_test_012, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingManager_AnalysisRemoveCloudLcd_MultipleFileIds_test_012: multiple fileIds input");
    auto& manager = LcdAgingManager::GetInstance();
    manager.SetIsActiveLcdAging(true);
    
    vector<int64_t> testIds;
    int32_t ret = InsertMultiplePhotoData(testIds, 3, false);
    ASSERT_EQ(ret, NativeRdb::E_OK);
    
    vector<int64_t> fileIds = testIds;
    ret = manager.AnalysisRemoveCloudLcd(fileIds);
    EXPECT_EQ(ret, E_ERR);
    
    DeleteMultiplePhotoData(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingManager_AnalysisRemoveCloudLcd_NonexistentFileId_test_013, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingManager_AnalysisRemoveCloudLcd_NonexistentFileId_test_013: nonexistent fileId");
    auto& manager = LcdAgingManager::GetInstance();
    manager.SetIsActiveLcdAging(true);
    
    vector<int64_t> fileIds = {99999999};
    int32_t ret = manager.AnalysisRemoveCloudLcd(fileIds);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingManager_StartDeepOptimizeSpace_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingManager_StartDeepOptimizeSpace_003: isActiveLcdAging not set on null client");
    auto& manager = LcdAgingManager::GetInstance();
    manager.SetIsActiveLcdAging(false);
    EXPECT_FALSE(manager.GetIsActiveLcdAging());

    int32_t ret = manager.StartDeepOptimizeSpace(nullptr, nullptr);
    EXPECT_EQ(ret, E_ERR);

    EXPECT_FALSE(manager.GetIsActiveLcdAging());
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingManager_CheckLcdAgingStatus_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingManager_CheckLcdAgingStatus_001: shouldStop=true returns E_AGING_STOP");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(true);

    int32_t ret = manager.CheckLcdAgingStatus(shouldStop);
    EXPECT_EQ(ret, E_AGING_STOP);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingManager_CheckLcdAgingStatus_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingManager_CheckLcdAgingStatus_002: shouldStop=false returns E_OK");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(false);

    int32_t ret = manager.CheckLcdAgingStatus(shouldStop);

    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingManager_CheckLcdAgingStatus_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingManager_CheckLcdAgingStatus_003: backup flag causes interrupt");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(false);

    system::SetParameter(TEST_MEDIA_BACKUP_FLAG, std::to_string(MediaFileUtils::UTCTimeSeconds()));
    int32_t ret = manager.CheckLcdAgingStatus(shouldStop);
    EXPECT_EQ(ret, E_AGING_INTERRUPT);
    system::SetParameter(TEST_MEDIA_BACKUP_FLAG, "0");
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingManager_CheckLcdAgingStatus_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingManager_CheckLcdAgingStatus_004: restore flag causes interrupt");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(false);

    system::SetParameter(TEST_MEDIA_RESTORE_FLAG, std::to_string(MediaFileUtils::UTCTimeSeconds()));
    int32_t ret = manager.CheckLcdAgingStatus(shouldStop);
    EXPECT_EQ(ret, E_AGING_INTERRUPT);
    system::SetParameter(TEST_MEDIA_RESTORE_FLAG, "0");
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingManager_CheckLcdAgingStatus_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingManager_CheckLcdAgingStatus_005: cloudsync switch causes interrupt");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(false);

    system::SetParameter(TEST_CLOUDSYNC_SWITCH_STATUS_KEY,
        std::to_string(MediaFileUtils::UTCTimeMilliSeconds()));
    int32_t ret = manager.CheckLcdAgingStatus(shouldStop);
    EXPECT_EQ(ret, E_AGING_INTERRUPT);
    system::SetParameter(TEST_CLOUDSYNC_SWITCH_STATUS_KEY, "0");
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingManager_CheckLcdAgingStatus_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingManager_CheckLcdAgingStatus_006: clone state causes interrupt");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(false);

    system::SetParameter(TEST_CLONE_STATE, "1");
    system::SetParameter(TEST_CLONE_FLAG, std::to_string(MediaFileUtils::UTCTimeSeconds()));
    int32_t ret = manager.CheckLcdAgingStatus(shouldStop);
    EXPECT_EQ(ret, E_AGING_INTERRUPT);

    system::SetParameter(TEST_CLONE_STATE, "0");
    system::SetParameter(TEST_CLONE_FLAG, "0");
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingManager_CheckLcdAgingStatus_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingManager_CheckLcdAgingStatus_008: shouldStop takes priority");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(true);

    system::SetParameter(TEST_MEDIA_BACKUP_FLAG, std::to_string(MediaFileUtils::UTCTimeSeconds()));
    int32_t ret = manager.CheckLcdAgingStatus(shouldStop);
    EXPECT_EQ(ret, E_AGING_STOP);
    system::SetParameter(TEST_MEDIA_BACKUP_FLAG, "0");
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingManager_CheckLcdAgingStatus_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingManager_CheckLcdAgingStatus_009: multiple flags set causes interrupt");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(false);

    system::SetParameter(TEST_MEDIA_BACKUP_FLAG, std::to_string(MediaFileUtils::UTCTimeSeconds()));
    system::SetParameter(TEST_MEDIA_RESTORE_FLAG, std::to_string(MediaFileUtils::UTCTimeSeconds()));
    int32_t ret = manager.CheckLcdAgingStatus(shouldStop);
    EXPECT_EQ(ret, E_AGING_INTERRUPT);
    system::SetParameter(TEST_MEDIA_BACKUP_FLAG, "0");
    system::SetParameter(TEST_MEDIA_RESTORE_FLAG, "0");
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingManager_CheckLcdAgingStatus_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingManager_CheckLcdAgingStatus_010: reset flags returns E_OK");
    auto& manager = LcdAgingManager::GetInstance();
    std::atomic<bool> shouldStop(false);

    system::SetParameter(TEST_MEDIA_BACKUP_FLAG, "0");
    system::SetParameter(TEST_MEDIA_RESTORE_FLAG, "0");
    system::SetParameter(TEST_CLOUDSYNC_SWITCH_STATUS_KEY, "0");
    system::SetParameter(TEST_CLONE_STATE, "0");
    system::SetParameter(TEST_CLONE_FLAG, "0");

int32_t ret = manager.CheckLcdAgingStatus(shouldStop);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingManager_StopDeepOptimizeSpace_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingManager_StopDeepOptimizeSpace_test_001: task running sets shouldStop and returns E_OK");
    auto& manager = LcdAgingManager::GetInstance();
    auto& worker = LcdAgingWorker::GetInstance();
    worker.isThreadRunning_.store(true);
    worker.shouldStop_.store(false);

    int32_t ret = manager.StopDeepOptimizeSpace();
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(worker.shouldStop_.load());

    worker.isThreadRunning_.store(false);
    worker.shouldStop_.store(false);
}

HWTEST_F(MediaLibraryLcdAgingTest, CheckLcdAgingTargetReached_EmptyDb_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckLcdAgingTargetReached_EmptyDb_test_001: empty db, LCD count below threshold");
    auto& manager = LcdAgingManager::GetInstance();

    int32_t excessSize = 0;
    int32_t ret = manager.CheckLcdAgingTargetReached(excessSize);

    EXPECT_EQ(ret, E_NO_QUERY_DATA);
}

HWTEST_F(MediaLibraryLcdAgingTest, CheckLcdAgingTargetReached_SingleRecord_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckLcdAgingTargetReached_SingleRecord_test_002: single record below threshold");
    auto& manager = LcdAgingManager::GetInstance();

    int64_t testId;
    int32_t ret = InsertNotTrashedPhotoData(testId);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    int32_t excessSize = 0;
    ret = manager.CheckLcdAgingTargetReached(excessSize);

    EXPECT_EQ(ret, E_NO_QUERY_DATA);

    DeletePhotoDataById(testId);
}

HWTEST_F(MediaLibraryLcdAgingTest, CheckLcdAgingTargetReached_LocalPosition_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckLcdAgingTargetReached_LocalPosition_test_003: position=1, not counted in LCD");
    auto& manager = LcdAgingManager::GetInstance();

    PhotoTestData data;
    data.position = 1;
    int64_t testId;
    int32_t ret = InsertPhotoData(testId, data);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    int32_t excessSize = 0;
    ret = manager.CheckLcdAgingTargetReached(excessSize);

    EXPECT_EQ(ret, E_NO_QUERY_DATA);

    DeletePhotoDataById(testId);
}

HWTEST_F(MediaLibraryLcdAgingTest, HandleAgingProgress_CappedAt99_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("HandleAgingProgress_CappedAt99_test_001: progress capped at 99 when would exceed");
    auto& manager = LcdAgingManager::GetInstance();

    manager.totalAgingLcdNumber_ = 100;
    manager.hasAgingLcdNumber_ = 100;
    manager.lastAgingProgress_ = 0;

    manager.HandleAgingProgress();

    EXPECT_EQ(manager.lastAgingProgress_, 99);
}

HWTEST_F(MediaLibraryLcdAgingTest, HandleAgingProgress_CappedAt99_NearTotal_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("HandleAgingProgress_CappedAt99_NearTotal_test_002: progress 99 when 99/100 aged");
    auto& manager = LcdAgingManager::GetInstance();

    manager.totalAgingLcdNumber_ = 100;
    manager.hasAgingLcdNumber_ = 99;
    manager.lastAgingProgress_ = 0;

    manager.HandleAgingProgress();

    EXPECT_EQ(manager.lastAgingProgress_, 99);
}

HWTEST_F(MediaLibraryLcdAgingTest, HandleAgingProgress_NoRegression_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("HandleAgingProgress_NoRegression_test_003: progress never regresses");
    auto& manager = LcdAgingManager::GetInstance();

    manager.totalAgingLcdNumber_ = 200;
    manager.hasAgingLcdNumber_ = 100;
    manager.lastAgingProgress_ = 60;

    manager.HandleAgingProgress();

    uint32_t progress = manager.lastAgingProgress_;
    EXPECT_GE(progress, 60);
    EXPECT_LE(progress, 99);
}

HWTEST_F(MediaLibraryLcdAgingTest, HandleAgingProgress_LowProgress_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("HandleAgingProgress_LowProgress_test_004: normal low progress not capped");
    auto& manager = LcdAgingManager::GetInstance();

    manager.totalAgingLcdNumber_ = 1000;
    manager.hasAgingLcdNumber_ = 50;
    manager.lastAgingProgress_ = 0;

    manager.HandleAgingProgress();

    EXPECT_EQ(manager.lastAgingProgress_, 5);
}

HWTEST_F(MediaLibraryLcdAgingTest, HandleAgingProgress_ZeroTotal_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("HandleAgingProgress_ZeroTotal_test_005: totalAgingLcdNumber=0, early return");
    auto& manager = LcdAgingManager::GetInstance();

    manager.totalAgingLcdNumber_ = 0;
    manager.hasAgingLcdNumber_ = 50;
    manager.lastAgingProgress_ = 0;

    manager.HandleAgingProgress();

    EXPECT_EQ(manager.lastAgingProgress_, 0);
}

static int32_t InsertLcdPhotoData(int64_t &testId, int32_t position, int64_t lcdFileSize = 0,
    int64_t dateTaken = DATE_TAKEN_TEST_VALUE, int64_t realLcdVisitTime = DATE_TAKEN_TEST_VALUE)
{
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, 0);
    values.PutLong(MediaColumn::MEDIA_DATE_TAKEN, dateTaken);
    values.PutInt(PhotoColumn::PHOTO_POSITION, position);
    values.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, 0);
    values.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, 0);
    values.PutLong(MediaColumn::MEDIA_TIME_PENDING, 0);
    values.PutInt(PhotoColumn::PHOTO_IS_TEMP, 0);
    values.PutInt(MediaColumn::MEDIA_IS_FAV, 0);
    values.PutInt(PhotoColumn::PHOTO_THUMB_STATUS, 0);
    values.PutInt(MediaColumn::MEDIA_DATE_TRASHED, 0);
    values.PutInt(PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, realLcdVisitTime);
    values.PutInt(PhotoColumn::PHOTO_LCD_FILE_SIZE, lcdFileSize);
    values.PutInt(MediaColumn::MEDIA_TYPE, static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    values.PutInt(PhotoColumn::PHOTO_ORIENTATION, 0);
    values.PutInt(PhotoColumn::PHOTO_EXIF_ROTATE, static_cast<int32_t>(ExifRotateType::TOP_LEFT));
    values.PutInt(PhotoColumn::PHOTO_THUMBNAIL_READY, 1);
    values.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, DATE_TAKEN_TEST_VALUE);
    return g_rdbStore->Insert(testId, PhotoColumn::PHOTOS_TABLE, values);
}

static int32_t InsertMultipleLcdPhotos(vector<int64_t> &testIds, int32_t count, int32_t position = 2,
    int64_t lcdFileSize = 0, int32_t daysOffset = 0)
{
    for (int32_t i = 0; i < count; i++) {
        int64_t testId;
        int64_t dateTaken = DATE_TAKEN_TEST_VALUE - daysOffset * 24 * 60 * 60 * 1000 + i;
        int64_t realLcdVisitTime = dateTaken;
        int32_t ret = InsertLcdPhotoData(testId, position, lcdFileSize, dateTaken, realLcdVisitTime);
        if (ret != NativeRdb::E_OK) {
            return ret;
        }
        testIds.push_back(testId);
    }
    return NativeRdb::E_OK;
}

static void SetupLcdEnvironment()
{
    system::SetParameter(TEST_MEDIA_BACKUP_FLAG, "0");
    system::SetParameter(TEST_MEDIA_RESTORE_FLAG, "0");
    system::SetParameter(TEST_CLONE_STATE, "0");
    system::SetParameter(TEST_CLONE_FLAG, "0");
    LcdAgingService::GetInstance().SetMarkingLcdStatus(false);
    auto& worker = LcdAgingWorker::GetInstance();
    worker.isThreadRunning_.store(false);
    worker.shouldStop_.store(false);
}

static void CleanupLcdEnvironment(const vector<int64_t> &testIds)
{
    for (auto testId : testIds) {
        DeletePhotoDataById(testId);
    }
    SetupLcdEnvironment();
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_CanPerformDeepOptimize_LcdCountLessThanMin_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_CanPerformDeepOptimize_LcdCountLessThanMin_test_001");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 100, 2, 102400);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    auto& service = LcdAgingService::GetInstance();
    bool result = false;
    ret = service.HandleCanPerformDeepOptimizeSpace(result);

    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(result);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_CanPerformDeepOptimize_IsCloning_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_CanPerformDeepOptimize_IsCloning_test_002");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 50000, 2, 102400);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    system::SetParameter(TEST_MEDIA_BACKUP_FLAG, std::to_string(MediaFileUtils::UTCTimeSeconds()));

    auto& service = LcdAgingService::GetInstance();
    bool result = false;
    ret = service.HandleCanPerformDeepOptimizeSpace(result);

    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(result);

    system::SetParameter(TEST_MEDIA_BACKUP_FLAG, "0");
    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_CanPerformDeepOptimize_IsRestoring_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_CanPerformDeepOptimize_IsRestoring_test_003");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 50000, 2, 102400);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    system::SetParameter(TEST_MEDIA_RESTORE_FLAG, std::to_string(MediaFileUtils::UTCTimeSeconds()));

    auto& service = LcdAgingService::GetInstance();
    bool result = false;
    ret = service.HandleCanPerformDeepOptimizeSpace(result);

    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(result);

    system::SetParameter(TEST_MEDIA_RESTORE_FLAG, "0");
    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_CanPerformDeepOptimize_IsCleaning_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_CanPerformDeepOptimize_IsCleaning_test_004");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 50000, 2, 102400);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    auto& worker = LcdAgingWorker::GetInstance();
    worker.isThreadRunning_.store(true);

    auto& service = LcdAgingService::GetInstance();
    bool result = false;
    ret = service.HandleCanPerformDeepOptimizeSpace(result);

    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(result);

    worker.isThreadRunning_.store(false);
    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_CanPerformDeepOptimize_IsMarking_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_CanPerformDeepOptimize_IsMarking_test_005");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 50000, 2, 102400);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    LcdAgingService::GetInstance().SetMarkingLcdStatus(true);

    auto& service = LcdAgingService::GetInstance();
    bool result = false;
    ret = service.HandleCanPerformDeepOptimizeSpace(result);

    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(result);

    LcdAgingService::GetInstance().SetMarkingLcdStatus(false);
    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_CanPerformDeepOptimize_HasReleasable_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_CanPerformDeepOptimize_HasReleasable_test_006");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 50000, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    auto& service = LcdAgingService::GetInstance();
    bool result = false;
    ret = service.HandleCanPerformDeepOptimizeSpace(result);

    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(result);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_CanPerformDeepOptimize_MultipleConditions_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_CanPerformDeepOptimize_MultipleConditions_test_007");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 50000, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    system::SetParameter(TEST_MEDIA_BACKUP_FLAG, std::to_string(MediaFileUtils::UTCTimeSeconds()));
    LcdAgingService::GetInstance().SetMarkingLcdStatus(true);

    auto& service = LcdAgingService::GetInstance();
    bool result = false;
    ret = service.HandleCanPerformDeepOptimizeSpace(result);

    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(result);

    system::SetParameter(TEST_MEDIA_BACKUP_FLAG, "0");
    LcdAgingService::GetInstance().SetMarkingLcdStatus(false);
    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_CanPerformDeepOptimize_BoundaryCount_test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_CanPerformDeepOptimize_BoundaryCount_test_008");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 50000, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    auto& service = LcdAgingService::GetInstance();
    bool result = false;
    ret = service.HandleCanPerformDeepOptimizeSpace(result);

    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(result);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_CanPerformDeepOptimize_HighCount_test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_CanPerformDeepOptimize_HighCount_test_009");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 60000, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    auto& service = LcdAgingService::GetInstance();
    bool result = false;
    ret = service.HandleCanPerformDeepOptimizeSpace(result);

    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(result);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_GetDeepOptimizableSpace_NoLcd_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_GetDeepOptimizableSpace_NoLcd_test_001");
    SetupLcdEnvironment();

    auto& service = LcdAgingService::GetInstance();
    int64_t space = 0;
    int32_t ret = service.HandleGetDeepOptimizableSpace(space);

    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(space, 0);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_GetDeepOptimizableSpace_BelowThreshold_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_GetDeepOptimizableSpace_BelowThreshold_test_002");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 39999, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    auto& service = LcdAgingService::GetInstance();
    int64_t space = 0;
    ret = service.HandleGetDeepOptimizableSpace(space);

    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(space, 0);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_GetDeepOptimizableSpace_ExceedThreshold_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_GetDeepOptimizableSpace_ExceedThreshold_test_003");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 40001, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    auto& service = LcdAgingService::GetInstance();
    int64_t space = 0;
    ret = service.HandleGetDeepOptimizableSpace(space);

    EXPECT_EQ(ret, E_OK);
    EXPECT_GT(space, 0);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_GetDeepOptimizableSpace_OldPhotos_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_GetDeepOptimizableSpace_OldPhotos_test_005");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 45000, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    auto& service = LcdAgingService::GetInstance();
    int64_t space = 0;
    ret = service.HandleGetDeepOptimizableSpace(space);

    EXPECT_EQ(ret, E_OK);
    EXPECT_GT(space, 0);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_GetDeepOptimizableSpace_MixedPhotos_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_GetDeepOptimizableSpace_MixedPhotos_test_006");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 25000, 2, 102400, 0);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    vector<int64_t> oldTestIds;
    ret = InsertMultipleLcdPhotos(oldTestIds, 20000, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    testIds.insert(testIds.end(), oldTestIds.begin(), oldTestIds.end());

    auto& service = LcdAgingService::GetInstance();
    int64_t space = 0;
    ret = service.HandleGetDeepOptimizableSpace(space);

    EXPECT_EQ(ret, E_OK);
    EXPECT_GT(space, 0);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_GetDeepOptimizableSpace_LargeFileSize_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_GetDeepOptimizableSpace_LargeFileSize_test_007");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 45000, 2, 2 * 1024 * 1024, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    auto& service = LcdAgingService::GetInstance();
    int64_t space = 0;
    ret = service.HandleGetDeepOptimizableSpace(space);

    EXPECT_EQ(ret, E_OK);
    EXPECT_GT(space, 0);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_GetDeepOptimizableSpace_ZeroFileSize_test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_GetDeepOptimizableSpace_ZeroFileSize_test_008");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 45000, 2, 0, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    auto& service = LcdAgingService::GetInstance();
    int64_t space = 0;
    ret = service.HandleGetDeepOptimizableSpace(space);

    EXPECT_EQ(ret, E_OK);
    EXPECT_GT(space, 0);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_GetDeepOptimizableSpace_LargeScale_test_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_GetDeepOptimizableSpace_LargeScale_test_010");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 100000, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    auto& service = LcdAgingService::GetInstance();
    int64_t space = 0;
    ret = service.HandleGetDeepOptimizableSpace(space);

    EXPECT_EQ(ret, E_OK);
    EXPECT_GT(space, 0);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_HasReleasableLcdImages_Empty_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_HasReleasableLcdImages_Empty_test_003");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 100, 2, 102400, 0);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    auto& service = LcdAgingService::GetInstance();
    bool hasReleasable = service.HasReleasableLcdImages();

    EXPECT_FALSE(hasReleasable);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_CanPerformDeepOptimize_CleanFlagSet_test_013, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_CanPerformDeepOptimize_CleanFlagSet_test_013");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 50000, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    for (auto testId : testIds) {
        string updateSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE +
            " SET clean_flag = 1 WHERE file_id = " + to_string(testId);
        g_rdbStore->ExecuteSql(updateSql);
    }

    auto& service = LcdAgingService::GetInstance();
    bool result = false;
    ret = service.HandleCanPerformDeepOptimizeSpace(result);

    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(result);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_CanPerformDeepOptimize_SyncStatusNotZero_test_014, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_CanPerformDeepOptimize_SyncStatusNotZero_test_014");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 50000, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    for (auto testId : testIds) {
        string updateSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE +
            " SET sync_status = 1 WHERE file_id = " + to_string(testId);
        g_rdbStore->ExecuteSql(updateSql);
    }

    auto& service = LcdAgingService::GetInstance();
    bool result = false;
    ret = service.HandleCanPerformDeepOptimizeSpace(result);

    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(result);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_CanPerformDeepOptimize_IsFavoriteSet_test_015, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_CanPerformDeepOptimize_IsFavoriteSet_test_015");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 50000, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    for (auto testId : testIds) {
        string updateSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE +
            " SET is_favorite = 1 WHERE file_id = " + to_string(testId);
        g_rdbStore->ExecuteSql(updateSql);
    }

    auto& service = LcdAgingService::GetInstance();
    bool result = false;
    ret = service.HandleCanPerformDeepOptimizeSpace(result);

    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(result);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_CanPerformDeepOptimize_ThumbStatusSet_test_016, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_CanPerformDeepOptimize_ThumbStatusSet_test_016");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 50000, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    for (auto testId : testIds) {
        string updateSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE +
            " SET thumb_status = 1 WHERE file_id = " + to_string(testId);
        g_rdbStore->ExecuteSql(updateSql);
    }

    auto& service = LcdAgingService::GetInstance();
    bool result = false;
    ret = service.HandleCanPerformDeepOptimizeSpace(result);

    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(result);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_CanPerformDeepOptimize_MixedPositions_test_017, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_CanPerformDeepOptimize_MixedPositions_test_017");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 16666, 1, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    vector<int64_t> testIds2;
    ret = InsertMultipleLcdPhotos(testIds2, 16667, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    vector<int64_t> testIds3;
    ret = InsertMultipleLcdPhotos(testIds3, 16667, 3, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    testIds.insert(testIds.end(), testIds2.begin(), testIds2.end());
    testIds.insert(testIds.end(), testIds3.begin(), testIds3.end());

    auto& service = LcdAgingService::GetInstance();
    bool result = false;
    ret = service.HandleCanPerformDeepOptimizeSpace(result);

    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(result);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_CanPerformDeepOptimize_JustAboveMin_test_018, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_CanPerformDeepOptimize_JustAboveMin_test_018");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 50001, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    auto& service = LcdAgingService::GetInstance();
    bool result = false;
    ret = service.HandleCanPerformDeepOptimizeSpace(result);

    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(result);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_CanPerformDeepOptimize_AllFlagsSet_test_020, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_CanPerformDeepOptimize_AllFlagsSet_test_020");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 50000, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    system::SetParameter(TEST_MEDIA_BACKUP_FLAG, std::to_string(MediaFileUtils::UTCTimeSeconds()));
    system::SetParameter(TEST_MEDIA_RESTORE_FLAG, std::to_string(MediaFileUtils::UTCTimeSeconds()));
    LcdAgingService::GetInstance().SetMarkingLcdStatus(true);
    auto& worker = LcdAgingWorker::GetInstance();
    worker.isThreadRunning_.store(true);

    auto& service = LcdAgingService::GetInstance();
    bool result = false;
    ret = service.HandleCanPerformDeepOptimizeSpace(result);

    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(result);

    system::SetParameter(TEST_MEDIA_BACKUP_FLAG, "0");
    system::SetParameter(TEST_MEDIA_RESTORE_FLAG, "0");
    LcdAgingService::GetInstance().SetMarkingLcdStatus(false);
    worker.isThreadRunning_.store(false);
    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_GetDeepOptimizableSpace_JustAboveThreshold_test_013, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_GetDeepOptimizableSpace_JustAboveThreshold_test_013");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 40001, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    auto& service = LcdAgingService::GetInstance();
    int64_t space = 0;
    ret = service.HandleGetDeepOptimizableSpace(space);

    EXPECT_EQ(ret, E_OK);
    EXPECT_GT(space, 0);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_GetDeepOptimizableSpace_JustBelowThreshold_test_014, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_GetDeepOptimizableSpace_JustBelowThreshold_test_014");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 39999, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    auto& service = LcdAgingService::GetInstance();
    int64_t space = 0;
    ret = service.HandleGetDeepOptimizableSpace(space);

    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(space, 0);
    
    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_GetDeepOptimizableSpace_VaryingFileSizes_test_016, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_GetDeepOptimizableSpace_VaryingFileSizes_test_016");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    for (int32_t i = 0; i < 45000; i++) {
        int64_t testId;
        int64_t fileSize = (i % 20) * 102400;
        int64_t dateTaken = DATE_TAKEN_TEST_VALUE - (THIRTY_DAYS_MS * 31 / 30) + i;
        int32_t ret = InsertLcdPhotoData(testId, 2, fileSize, dateTaken, dateTaken);
        ASSERT_EQ(ret, NativeRdb::E_OK);
        testIds.push_back(testId);
    }

    auto& service = LcdAgingService::GetInstance();
    int64_t space = 0;
    int32_t ret = service.HandleGetDeepOptimizableSpace(space);

    EXPECT_EQ(ret, E_OK);
    EXPECT_GT(space, 0);
    
    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_GetDeepOptimizableSpace_MixedPositions_test_017, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_GetDeepOptimizableSpace_MixedPositions_test_017");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 15000, 1, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    vector<int64_t> testIds2;
    ret = InsertMultipleLcdPhotos(testIds2, 15000, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    vector<int64_t> testIds3;
    ret = InsertMultipleLcdPhotos(testIds3, 15000, 3, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    testIds.insert(testIds.end(), testIds2.begin(), testIds2.end());
    testIds.insert(testIds.end(), testIds3.begin(), testIds3.end());

    auto& service = LcdAgingService::GetInstance();
    int64_t space = 0;
    ret = service.HandleGetDeepOptimizableSpace(space);

    EXPECT_EQ(ret, E_OK);
    EXPECT_GT(space, 0);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_GetDeepOptimizableSpace_31Days_test_019, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_GetDeepOptimizableSpace_31Days_test_019");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 45000, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    auto& service = LcdAgingService::GetInstance();
    int64_t space = 0;
    ret = service.HandleGetDeepOptimizableSpace(space);

    EXPECT_EQ(ret, E_OK);
    EXPECT_GT(space, 0);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_GetDeepOptimizableSpace_VeryOldPhotos_test_020, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_GetDeepOptimizableSpace_VeryOldPhotos_test_020");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 45000, 2, 102400, 365);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    auto& service = LcdAgingService::GetInstance();
    int64_t space = 0;
    ret = service.HandleGetDeepOptimizableSpace(space);

    EXPECT_EQ(ret, E_OK);
    EXPECT_GT(space, 0);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_HasReleasableLcdImages_Exactly30Days_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_HasReleasableLcdImages_Exactly30Days_test_005");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 100, 2, 102400, 30);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    auto& service = LcdAgingService::GetInstance();
    bool hasReleasable = service.HasReleasableLcdImages();

    EXPECT_FALSE(hasReleasable);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_HasReleasableLcdImages_LcdUsingStatusSet_test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_HasReleasableLcdImages_LcdUsingStatusSet_test_009");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 100, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    for (auto testId : testIds) {
        string updateSql = "UPDATE " + PhotoExtColumn::PHOTOS_EXT_TABLE +
            " SET lcd_using_status = 1 WHERE file_id = " + to_string(testId);
        g_rdbStore->ExecuteSql(updateSql);
    }

    auto& service = LcdAgingService::GetInstance();
    bool hasReleasable = service.HasReleasableLcdImages();

    EXPECT_FALSE(hasReleasable);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_HasReleasableLcdImages_IsTempSet_test_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_HasReleasableLcdImages_IsTempSet_test_010");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 100, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    for (auto testId : testIds) {
        string updateSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE +
            " SET is_temp = 1 WHERE file_id = " + to_string(testId);
        g_rdbStore->ExecuteSql(updateSql);
    }

    auto& service = LcdAgingService::GetInstance();
    bool hasReleasable = service.HasReleasableLcdImages();

    EXPECT_FALSE(hasReleasable);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_HasReleasableLcdImages_TimePendingSet_test_011, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_HasReleasableLcdImages_TimePendingSet_test_011");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 100, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    for (auto testId : testIds) {
        string updateSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE +
            " SET time_pending = 1 WHERE file_id = " + to_string(testId);
        g_rdbStore->ExecuteSql(updateSql);
    }

    auto& service = LcdAgingService::GetInstance();
    bool hasReleasable = service.HasReleasableLcdImages();

    EXPECT_FALSE(hasReleasable);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_HasReleasableLcdImages_SyncStatusNotZero_test_012, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_HasReleasableLcdImages_SyncStatusNotZero_test_012");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 100, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    for (auto testId : testIds) {
        string updateSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE +
            " SET sync_status = 1 WHERE file_id = " + to_string(testId);
        g_rdbStore->ExecuteSql(updateSql);
    }

    auto& service = LcdAgingService::GetInstance();
    bool hasReleasable = service.HasReleasableLcdImages();

    EXPECT_FALSE(hasReleasable);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_HasReleasableLcdImages_CleanFlagSet_test_013, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_HasReleasableLcdImages_CleanFlagSet_test_013");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 100, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    for (auto testId : testIds) {
        string updateSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE +
            " SET clean_flag = 1 WHERE file_id = " + to_string(testId);
        g_rdbStore->ExecuteSql(updateSql);
    }

    auto& service = LcdAgingService::GetInstance();
    bool hasReleasable = service.HasReleasableLcdImages();

    EXPECT_FALSE(hasReleasable);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_HasReleasableLcdImages_IsFavoriteSet_test_014, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_HasReleasableLcdImages_IsFavoriteSet_test_014");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 100, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    for (auto testId : testIds) {
        string updateSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE +
            " SET is_favorite = 1 WHERE file_id = " + to_string(testId);
        g_rdbStore->ExecuteSql(updateSql);
    }

    auto& service = LcdAgingService::GetInstance();
    bool hasReleasable = service.HasReleasableLcdImages();

    EXPECT_FALSE(hasReleasable);

    CleanupLcdEnvironment(testIds);
}

HWTEST_F(MediaLibraryLcdAgingTest, LcdAgingService_HasReleasableLcdImages_ThumbStatusBitSet_test_015, TestSize.Level1)
{
    MEDIA_INFO_LOG("LcdAgingService_HasReleasableLcdImages_ThumbStatusBitSet_test_015");
    SetupLcdEnvironment();

    vector<int64_t> testIds;
    int32_t ret = InsertMultipleLcdPhotos(testIds, 100, 2, 102400, 31);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    for (auto testId : testIds) {
        string updateSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE +
            " SET thumb_status = 1 WHERE file_id = " + to_string(testId);
        g_rdbStore->ExecuteSql(updateSql);
    }

    auto& service = LcdAgingService::GetInstance();
    bool hasReleasable = service.HasReleasableLcdImages();

    EXPECT_FALSE(hasReleasable);

    CleanupLcdEnvironment(testIds);
}
} // namespace Media
} // namespace OHOS