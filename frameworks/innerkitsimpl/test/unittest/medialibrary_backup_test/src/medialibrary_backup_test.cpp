/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#define MLOG_TAG "BackupTest"

#include "medialibrary_backup_test.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>

#define private public
#define protected public
#include "backup_const_column.h"
#include "backup_const_map.h"
#include "backup_database_utils.h"
#include "backup_file_utils.h"
#include "external_source.h"
#include "gallery_source.h"
#include "media_log.h"
#include "upgrade_restore.h"
#include "media_file_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "vision_db_sqls.h"
#include "geo_knowledge_restore.h"
#undef private
#undef protected
#include "mimetype_utils.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
const std::string TEST_BACKUP_PATH = "/data/test/backup/db";
const std::string TEST_BACKUP_PATH_CLOUD = "/data/test/backup/db1";
const std::string TEST_UPGRADE_FILE_DIR = "/data/test/backup/file";
const std::string GALLERY_APP_NAME = "gallery";
const std::string MEDIA_APP_NAME = "external";
const std::string MEDIA_LIBRARY_APP_NAME = "medialibrary";
const std::string TEST_PATH_PREFIX = "/TestPrefix";
const std::string TEST_RELATIVE_PATH = "/Pictures/Test/test.jpg";
const std::string TEST_CLOUD_PATH_PREFIX = "/storage/cloud/files/Photo/1/";
const std::string TEST_FILE = "photoTest.mp4";
const std::string TEST_DIR_PATH = "test";

const int EXPECTED_NUM = 34;
const int EXPECTED_OREINTATION = 270;
const int TEST_LOCAL_MEDIA_ID = 12345;
const int TEST_PREFIX_LEVEL = 4;
const int TEST_SIZE_MIN = 1080;
const int TEST_SIZE_MAX = 2560;
const int TEST_SIZE_INCR_UNIT = 100;
const int TEST_SIZE_MULT_UNIT = 2;
const int TEST_PORTRAIT_ALBUM_COUNT = 2;
const int TEST_PORTRAIT_TAG_COUNT = 3;
const int TEST_PORTRAIT_PHOTO_COUNT = 3;
const int TEST_PORTRAIT_FACE_COUNT = 4;
const float TEST_SCALE_MIN = 0.2;
const float TEST_SCALE_MAX = 0.8;
const float TEST_LANDMARK_BELOW = 0.1;
const float TEST_LANDMARK_BETWEEN = 0.5;
const float TEST_LANDMARK_ABOVE = 0.9;
const std::string EXPECTED_PACKAGE_NAME = "wechat";
const std::string EXPECTED_USER_COMMENT = "user_comment";
const int64_t EXPECTED_DATE_ADDED = 1495970415378;
const int64_t EXPECTED_DATE_TAKEN = 1495970415379;
const std::string EXPECTED_DETAIL_TIME = "2024:09:06 17:00:01";
const int64_t TEST_TRUE_MEDIAID = 1;
const int64_t TEST_FALSE_MEDIAID = -1;
const int64_t TEST_SIZE_2MB = 2 * 1024 * 1024;
const int64_t TEST_SIZE_2MB_BELOW = TEST_SIZE_2MB - 1;
const int64_t TEST_SIZE_2MB_ABOVE = TEST_SIZE_2MB + 1;
const int32_t TEST_MIGRATE_CLOUD_LCD_TYPE = 1;
const int32_t APP_MAIN_DATA_USER_ID = 0;
const int32_t APP_TWIN_DATA_USER_ID_START = 128;
const std::string TEST_TRUE_UNIQUEID = "1";
const std::string TEST_FALSE_UNIQUEID = "-1";
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
const vector<string> CLEAR_SQLS = {
    "DELETE FROM " + PhotoColumn::PHOTOS_TABLE,
    "DELETE FROM " + PhotoAlbumColumns::TABLE + " WHERE " + PhotoAlbumColumns::ALBUM_TYPE + " != " +
        to_string(PhotoAlbumType::SYSTEM),
    "DELETE FROM " + PhotoMap::TABLE,
    "DELETE FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " + PhotoAlbumColumns::ALBUM_SUBTYPE + " != " +
        to_string(PhotoAlbumSubType::SHOOTING_MODE),
    "DELETE FROM " + ANALYSIS_PHOTO_MAP_TABLE,
    "DELETE FROM " + AudioColumn::AUDIOS_TABLE,
    "DELETE FROM tab_analysis_image_face",
    "DELETE FROM tab_analysis_face_tag",
    "DELETE FROM tab_analysis_total",
    CREATE_VISION_INSERT_TRIGGER_FOR_ONCREATE,
};

const string PhotosOpenCall::CREATE_PHOTOS = string("CREATE TABLE IF NOT EXISTS Photos ") +
    " (file_id INTEGER PRIMARY KEY AUTOINCREMENT, data TEXT, title TEXT, display_name TEXT, size BIGINT," +
    " media_type INT, date_added BIGINT, date_taken BIGINT, duration INT, is_favorite INT default 0, " +
    " date_trashed BIGINT DEFAULT 0, hidden INT DEFAULT 0, height INT, width INT, user_comment TEXT, " +
    " orientation INT DEFAULT 0, package_name TEXT, burst_cover_level INT DEFAULT 1, burst_key TEXT, " +
    " dirty INTEGER DEFAULT 0, subtype INT, detail_time TEXT );";

const string PhotosOpenCall::CREATE_PHOTOS_ALBUM = "CREATE TABLE IF NOT EXISTS PhotoAlbum \
    (album_id INTEGER PRIMARY KEY AUTOINCREMENT, album_type INT,   \
    album_subtype INT, album_name TEXT COLLATE NOCASE, cover_uri TEXT, count \
    INT DEFAULT 0, date_modified BIGINT DEFAULT 0, dirty INT DEFAULT  1  , cloud_id TEXT,  \
    relative_path TEXT, contains_hidden INT DEFAULT 0, hidden_count INT DEFAULT 0,  \
    hidden_cover TEXT DEFAULT '', album_order INT,  image_count INT DEFAULT 0,  \
    video_count INT DEFAULT 0, bundle_name TEXT, local_language TEXT,  \
    is_local INT)";

const string PhotosOpenCall::CREATE_PHOTOS_MAP = "CREATE TABLE IF NOT EXISTS  PhotoMap \
    (map_album INT, map_asset INT, dirty INT DEFAULT 1, PRIMARY KEY (map_album, map_asset))";

int PhotosOpenCall::OnCreate(RdbStore &store)
{
    int ret = 0;
    ret += store.ExecuteSql(CREATE_PHOTOS);
    ret += store.ExecuteSql(CREATE_PHOTOS_ALBUM);
    ret += store.ExecuteSql(CREATE_PHOTOS_MAP);
    return ret;
}

int PhotosOpenCall::OnUpgrade(RdbStore &store, int oldVersion, int newVersion)
{
    return 0;
}

std::shared_ptr<NativeRdb::RdbStore> photosStorePtr = nullptr;
std::unique_ptr<UpgradeRestore> restoreService = nullptr;
std::shared_ptr<NativeRdb::RdbStore> photosStorePtrCloud = nullptr;
std::unique_ptr<UpgradeRestore> restoreServiceCloud = nullptr;

void InitPhotoAlbum(std::shared_ptr<NativeRdb::RdbStore> &photosStore)
{
    photosStore->ExecuteSql("INSERT INTO PhotoAlbum (album_type, album_subtype,album_name) \
        VALUES (0, 1, 'test101');");
    photosStore->ExecuteSql("INSERT INTO PhotoAlbum (album_type, album_subtype,album_name) \
        VALUES (1024, 2049, 'TmallPic');");
    photosStore->ExecuteSql("INSERT INTO PhotoAlbum (album_type, album_subtype,album_name) \
        VALUES (1024, 2049, '美图贴贴');");
    photosStore->ExecuteSql("INSERT INTO PhotoAlbum (album_type, album_subtype,album_name) \
        VALUES (1024, 2049, 'funnygallery');");
    photosStore->ExecuteSql("INSERT INTO PhotoAlbum (album_type, album_subtype,album_name) \
        VALUES (1024, 2049, 'xiaohongshu');");
    photosStore->ExecuteSql("INSERT INTO PhotoAlbum (album_type, album_subtype,album_name) \
        VALUES (1024, 2049, 'Douyin');");
    photosStore->ExecuteSql("INSERT INTO PhotoAlbum (album_type, album_subtype,album_name) \
        VALUES (1024, 2049, 'save');");
    photosStore->ExecuteSql("INSERT INTO PhotoAlbum (album_type, album_subtype,album_name) \
        VALUES (1024, 2049, 'Weibo');");
    photosStore->ExecuteSql("INSERT INTO PhotoAlbum (album_type, album_subtype,album_name) \
        VALUES (1024, 2049, 'Camera');");
    photosStore->ExecuteSql("INSERT INTO PhotoAlbum (album_type, album_subtype,album_name) \
        VALUES (1024, 2049, 'Screenshots');");
    photosStore->ExecuteSql("INSERT INTO PhotoAlbum (album_type, album_subtype,album_name) \
        VALUES (1024, 2049, 'Screenrecorder');");
    photosStore->ExecuteSql("INSERT INTO PhotoAlbum (album_type, album_subtype,album_name) VALUES (1024, 2049,'" +
        GetDUALBundleName() + " Share');");
}

void Init(GallerySource &gallerySource, ExternalSource &externalSource, std::string testBackupPath,
    std::unique_ptr<UpgradeRestore> &restorePtr, std::shared_ptr<NativeRdb::RdbStore> &photosStore)
{
    MEDIA_INFO_LOG("start init galleryDb");
    const string galleryDbPath = testBackupPath + "/" + GALLERY_APP_NAME + "/ce/databases/gallery.db";
    gallerySource.Init(galleryDbPath);
    MEDIA_INFO_LOG("end init galleryDb");
    MEDIA_INFO_LOG("start init externalDb");
    const string externalDbPath = testBackupPath + "/" + MEDIA_APP_NAME + "/ce/databases/external.db";
    externalSource.Init(externalDbPath);
    MEDIA_INFO_LOG("end init externalDb");
    const string dbPath = testBackupPath + "/" + MEDIA_LIBRARY_APP_NAME + "/ce/databases/media_library.db";
    NativeRdb::RdbStoreConfig config(dbPath);
    PhotosOpenCall helper;
    int errCode = 0;
    shared_ptr<NativeRdb::RdbStore> store = NativeRdb::RdbHelper::GetRdbStore(config, 1, helper, errCode);
    photosStore = store;
    InitPhotoAlbum(photosStore);
    restorePtr = std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, UPGRADE_RESTORE_ID);
    restorePtr->Init(testBackupPath, TEST_UPGRADE_FILE_DIR, false);
    restorePtr->InitGarbageAlbum();
}

void RestoreFromGallery(std::unique_ptr<UpgradeRestore> &restorePtr, std::shared_ptr<NativeRdb::RdbStore> &photosStore)
{
    std::vector<FileInfo> fileInfos = restorePtr->QueryFileInfos(0);
    for (size_t i = 0; i < fileInfos.size(); i++) {
        const NativeRdb::ValuesBucket values = restorePtr->GetInsertValue(fileInfos[i], TEST_BACKUP_PATH,
            SourceType::GALLERY);
        int64_t rowNum = 0;
        if (photosStore->Insert(rowNum, "Photos", values) != E_OK) {
            MEDIA_ERR_LOG("InsertSql failed, filePath = %{private}s", fileInfos[i].filePath.c_str());
        }
    }
}

void RestoreFromExternal(GallerySource &gallerySource, bool isCamera,
    std::unique_ptr<UpgradeRestore> &restorePtr, std::shared_ptr<NativeRdb::RdbStore> &photosStore)
{
    MEDIA_INFO_LOG("start restore from %{public}s", (isCamera ? "camera" : "others"));
    int32_t maxId = BackupDatabaseUtils::QueryInt(gallerySource.galleryStorePtr_, isCamera ?
        QUERY_MAX_ID_CAMERA_SCREENSHOT : QUERY_MAX_ID_OTHERS, CUSTOM_MAX_ID);
    int32_t type = isCamera ? SourceType::EXTERNAL_CAMERA : SourceType::EXTERNAL_OTHERS;
    std::vector<FileInfo> fileInfos = restorePtr->QueryFileInfosFromExternal(0, maxId, isCamera);
    MEDIA_INFO_LOG("%{public}d asset will restor, maxid: %{public}d", (int)fileInfos.size(), maxId);
    for (size_t i = 0; i < fileInfos.size(); i++) {
        const NativeRdb::ValuesBucket values = restorePtr->GetInsertValue(fileInfos[i], TEST_BACKUP_PATH, type);
        int64_t rowNum = 0;
        if (photosStore->Insert(rowNum, "Photos", values) != E_OK) {
            MEDIA_ERR_LOG("InsertSql failed, filePath = %{private}s", fileInfos[i].filePath.c_str());
        }
    }
}

void MediaLibraryBackupTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
    GallerySource gallerySource;
    ExternalSource externalSource;
    Init(gallerySource, externalSource, TEST_BACKUP_PATH, restoreService, photosStorePtr);
    RestoreFromGallery(restoreService, photosStorePtr);
    RestoreFromExternal(gallerySource, true, restoreService, photosStorePtr);
    RestoreFromExternal(gallerySource, false, restoreService, photosStorePtr);
    GallerySource gallerySourceCloud;
    ExternalSource externalSourceCloud;
    Init(gallerySourceCloud, externalSourceCloud, TEST_BACKUP_PATH_CLOUD, restoreServiceCloud, photosStorePtrCloud);
    RestoreFromGallery(restoreServiceCloud, photosStorePtrCloud);
    RestoreFromExternal(gallerySourceCloud, true, restoreServiceCloud, photosStorePtrCloud);
    RestoreFromExternal(gallerySourceCloud, false, restoreServiceCloud, photosStorePtrCloud);
}

void MediaLibraryBackupTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

// SetUp:Execute before each test case
void MediaLibraryBackupTest::SetUp() {}

void MediaLibraryBackupTest::TearDown(void) {}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_init, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_init start");
    std::unique_ptr<UpgradeRestore> restoreService = std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME,
        UPGRADE_RESTORE_ID);
    int32_t result = restoreService->Init(TEST_BACKUP_PATH, TEST_UPGRADE_FILE_DIR, false);
    EXPECT_EQ(result, 0);
    MEDIA_INFO_LOG("medialib_backup_test_init end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_not_sync_valid, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_not_sync_valid start");
    std::string queryNotSyncValid = "SELECT file_id from Photos where display_name ='not_sync_weixin.jpg'";
    auto resultSet = photosStorePtr->QuerySql(queryNotSyncValid);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_not_sync_valid end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_not_sync_invalid, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_not_sync_invalid start");
    std::string queryNotSyncInvalid =
        "SELECT file_id from Photos where display_name ='not_sync_invalid.jpg'";
    auto resultSet = photosStorePtr->QuerySql(queryNotSyncInvalid);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_not_sync_invalid end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_not_sync_pending_camera, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_not_sync_pending_camera start");
    std::string queryNotSyncPendingCamera =
        "SELECT file_id from Photos where display_name ='not_sync_pending_camera.jpg'";
    auto resultSet = photosStorePtr->QuerySql(queryNotSyncPendingCamera);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_not_sync_pending_camera end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_not_sync_pending_others, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_not_sync_pending_others start");
    std::string queryNotSyncPendingOthers =
        "SELECT file_id from Photos where display_name ='not_sync_pending_others.jpg'";
    auto resultSet = photosStorePtr->QuerySql(queryNotSyncPendingOthers);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_not_sync_pending_others end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_not_restore_size_0, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_not_restore_size_0 start");
    std::string queryNotSyncPendingOthers =
        "SELECT file_id from Photos where display_name ='zero_size.jpg'";
    auto resultSet = photosStorePtr->QuerySql(queryNotSyncPendingOthers);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_not_restore_size_0 end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_create_asset_path_by_id, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_create_asset_path_by_id start");
    int32_t uniqueId = 2;
    int32_t fileType = 1;
    std::string extension = "jpg";
    std::string cloudPath;
    int64_t currentTime = MediaFileUtils::UTCTimeSeconds();
    std::string dir = RESTORE_CLOUD_DIR + "/" + to_string(uniqueId) + "/IMG_" +
        to_string(MediaFileUtils::UTCTimeSeconds()) + "_00" + to_string(uniqueId) + "." + extension;
    int32_t errCode = BackupFileUtils::CreateAssetPathById(uniqueId, fileType, extension, cloudPath);
    MEDIA_INFO_LOG("dir: %{public}s, cloudPath: %{public}s", dir.c_str(), cloudPath.c_str());
    ASSERT_TRUE(dir == cloudPath);
    MEDIA_INFO_LOG("medialib_backup_test_create_asset_path_by_id end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_cal_not_found_number_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_cal_not_found_number_001 start");
    restoreService->notFoundNumber_ = 0;
    std::vector<FileInfo> fileInfos = restoreService->QueryFileInfos(0);
    restoreService->restoreMode_ = RESTORE_MODE_PROC_ALL_DATA;
    (void)restoreService->BaseRestore::GetInsertValues(0, fileInfos, 0);
    EXPECT_EQ(restoreService->notFoundNumber_, 0);

    restoreService->restoreMode_ = RESTORE_MODE_PROC_TWIN_DATA;
    (void)restoreService->BaseRestore::GetInsertValues(0, fileInfos, 0);
    EXPECT_EQ(restoreService->notFoundNumber_, fileInfos.size());
    MEDIA_INFO_LOG("medialib_backup_test_cal_not_found_number end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_cal_not_found_number_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_cal_not_found_number_002 start");
    restoreService->notFoundNumber_ = 0;
    std::vector<FileInfo> fileInfos;
    FileInfo fileInfo;
    fileInfo.userId = APP_TWIN_DATA_USER_ID_START;
    fileInfos.push_back(fileInfo);
    restoreService->restoreMode_ = RESTORE_MODE_PROC_ALL_DATA;
    (void)restoreService->BaseRestore::GetInsertValues(0, fileInfos, 0);
    EXPECT_EQ(restoreService->notFoundNumber_, 0);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_cal_not_found_number_003, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_cal_not_found_number_003 start");
    restoreService->notFoundNumber_ = 0;
    std::vector<FileInfo> fileInfos;
    FileInfo fileInfo;
    fileInfo.userId = APP_MAIN_DATA_USER_ID;
    fileInfos.push_back(fileInfo);
    restoreService->restoreMode_ = RESTORE_MODE_PROC_ALL_DATA;
    (void)restoreService->BaseRestore::GetInsertValues(0, fileInfos, 0);
    EXPECT_EQ(restoreService->notFoundNumber_, 0);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_cal_not_found_number_004, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_cal_not_found_number_004 start");
    restoreService->notFoundNumber_ = 0;
    std::vector<FileInfo> fileInfos;
    FileInfo fileInfo;
    fileInfo.userId = APP_MAIN_DATA_USER_ID;
    fileInfo.filePath = TEST_DIR_PATH;
    fileInfos.push_back(fileInfo);
    restoreService->restoreMode_ = RESTORE_MODE_PROC_ALL_DATA;
    (void)restoreService->BaseRestore::GetInsertValues(0, fileInfos, 0);
    EXPECT_EQ(restoreService->notFoundNumber_, 0);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_update_clone, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_update_clone start");
    int32_t uniqueId = 2;
    int32_t fileType = 1;
    std::string extension = "jpg";
    std::string cloudPath;
    int64_t currentTime = MediaFileUtils::UTCTimeSeconds();
    std::string dir = RESTORE_CLOUD_DIR + "/" + to_string(uniqueId) + "/IMG_" +
        to_string(MediaFileUtils::UTCTimeSeconds()) + "_00" + to_string(uniqueId) + "." + extension;
    int32_t errCode = BackupFileUtils::CreateAssetPathById(uniqueId, fileType, extension, cloudPath);
    MEDIA_INFO_LOG("dir: %{public}s, cloudPath: %{public}s", dir.c_str(), cloudPath.c_str());
    ASSERT_TRUE(dir == cloudPath);
    MEDIA_INFO_LOG("medialib_backup_test_update_clone end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_a_media_not_sync_valid, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_a_media_not_sync_valid start");
    std::string queryNotSyncValid = "SELECT file_id from Photos where display_name ='a_media_not_sync.jpg'";
    auto resultSet = photosStorePtr->QuerySql(queryNotSyncValid);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_a_media_not_sync_valid end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_a_media_zero_size, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_a_media_zero_size start");
    std::string queryNotSyncValid = "SELECT file_id from Photos where display_name ='a_media_zero_size.jpg'";
    auto resultSet = photosStorePtr->QuerySql(queryNotSyncValid);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_a_media_zero_size end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_duplicate_data_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_duplicate_data_001 start");
    restoreService->photosRestore_.galleryRdb_ = restoreService->galleryRdb_;
    ASSERT_NE(restoreService->photosRestore_.galleryRdb_, nullptr);
 
    restoreService->photosRestore_.duplicateDataUsedCountMap_.clear();
    restoreService->AnalyzeGalleryErrorSource();
    size_t count = restoreService->photosRestore_.duplicateDataUsedCountMap_.size();
    MEDIA_INFO_LOG("count: %{public}zu", count);
    EXPECT_GT(count, 0); // has duplicate data
 
    string dataPath = "/storage/emulated/0/A/media/Rocket/test/duplicate_data.mp4";
    bool isDuplicateData = restoreService->photosRestore_.IsDuplicateData(dataPath);
    MEDIA_INFO_LOG("check %{public}s: %{public}d", dataPath.c_str(), static_cast<int32_t>(isDuplicateData));
    EXPECT_EQ(isDuplicateData, false); // first time used, not duplicate
    isDuplicateData = restoreService->photosRestore_.IsDuplicateData(dataPath);
    MEDIA_INFO_LOG("double check %{public}s: %{public}d", dataPath.c_str(), static_cast<int32_t>(isDuplicateData));
    EXPECT_EQ(isDuplicateData, true); // second time used, duplicate
    dataPath = "/storage/emulated/0/fake/fake.jpg";
    isDuplicateData = restoreService->photosRestore_.IsDuplicateData(dataPath);
    MEDIA_INFO_LOG("check %{public}s: %{public}d", dataPath.c_str(), static_cast<int32_t>(isDuplicateData));
    EXPECT_EQ(isDuplicateData, false); // not exist in map, not duplicate
    MEDIA_INFO_LOG("medialib_backup_test_duplicate_data_001 end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_duplicate_data_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_duplicate_data_002 start");
    restoreService->photosRestore_.galleryRdb_ = restoreService->galleryRdb_;
    ASSERT_NE(restoreService->photosRestore_.galleryRdb_, nullptr);

    restoreService->photosRestore_.duplicateDataUsedCountMap_.clear();
    restoreService->AnalyzeGalleryErrorSource();
 
    string dataPath = "/storage/emulated/0/A/media/Rocket/test/DUPLICATE_DATA_CASE.mp4";
    bool isDuplicateData = restoreService->photosRestore_.IsDuplicateData(dataPath);
    EXPECT_EQ(isDuplicateData, false);
    
    dataPath = "/storage/emulated/0/A/media/Rocket/test/duplicate_data_case.mp4";
    isDuplicateData = restoreService->photosRestore_.IsDuplicateData(dataPath);
    EXPECT_EQ(isDuplicateData, true);
    MEDIA_INFO_LOG("medialib_backup_test_duplicate_data_002 end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_restore_mode_not_del_db, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_restore_mode_not_del_db start");
    const string galleryDbPath = TEST_BACKUP_PATH + "/" + GALLERY_APP_NAME + "/ce/databases/gallery.db";
    const string externalDbPath = TEST_BACKUP_PATH + "/" + MEDIA_APP_NAME + "/ce/databases/external.db";
    bool isGalleryDbExist = MediaFileUtils::IsFileExists(galleryDbPath);
    bool isExternalDbExist = MediaFileUtils::IsFileExists(externalDbPath);
    EXPECT_EQ(isGalleryDbExist, true);
    EXPECT_EQ(isExternalDbExist, true);

    restoreService->restoreMode_ = RESTORE_MODE_PROC_MAIN_DATA;
    restoreService->RestoreAudio();
    restoreService->RestorePhoto();
    isGalleryDbExist = MediaFileUtils::IsFileExists(galleryDbPath);
    isExternalDbExist = MediaFileUtils::IsFileExists(externalDbPath);
    EXPECT_EQ(isGalleryDbExist, true);
    EXPECT_EQ(isExternalDbExist, true);
    MEDIA_INFO_LOG("medialib_backup_test_restore_mode_not_del_db end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_restore_mode_del_db, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_restore_mode_del_db start");
    const string galleryDbPath = TEST_BACKUP_PATH + "/" + GALLERY_APP_NAME + "/ce/databases/gallery.db";
    const string externalDbPath = TEST_BACKUP_PATH + "/" + MEDIA_APP_NAME + "/ce/databases/external.db";
    bool isGalleryDbExist = MediaFileUtils::IsFileExists(galleryDbPath);
    bool isExternalDbExist = MediaFileUtils::IsFileExists(externalDbPath);
    EXPECT_EQ(isGalleryDbExist, true);
    EXPECT_EQ(isExternalDbExist, true);

    restoreService->restoreMode_ = RESTORE_MODE_PROC_ALL_DATA;
    restoreService->RestoreAudio();
    restoreService->RestorePhoto();
    isGalleryDbExist = MediaFileUtils::IsFileExists(galleryDbPath);
    isExternalDbExist = MediaFileUtils::IsFileExists(externalDbPath);
    EXPECT_EQ(isGalleryDbExist, false);
    EXPECT_EQ(isExternalDbExist, false);
    MEDIA_INFO_LOG("medialib_backup_test_restore_mode_del_db end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_modify_file, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_modify_file start");
    struct stat bufBefore;
    const char *path = "/data/test/backup/1.txt";
    stat(path, &bufBefore);
    int64_t beforeModifiedTime = MediaFileUtils::Timespec2Millisecond(bufBefore.st_mtim);
    int64_t currentTime = MediaFileUtils::UTCTimeSeconds();
    struct utimbuf buf;
    buf.actime = currentTime;
    buf.modtime = currentTime;
    utime(path, &buf);
    struct stat bufAfter;
    stat(path, &bufAfter);
    int64_t afterModifiedTime = MediaFileUtils::Timespec2Millisecond(bufAfter.st_mtim);
    ASSERT_TRUE(afterModifiedTime != beforeModifiedTime);
    MEDIA_INFO_LOG("medialib_backup_test_modify_file end");
}

HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_CreateAssetPathById_mediaType_audio, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_CreateAssetPathById_mediaType_audio start");
    int32_t uniqueId = 2;
    int32_t fileType = 3;
    std::string extension = "mp3";
    std::string cloudPath;
    int32_t errCode = BackupFileUtils::CreateAssetPathById(uniqueId, fileType, extension, cloudPath);
    EXPECT_EQ(errCode, E_OK);
    MEDIA_INFO_LOG("BackupFileUtils_CreateAssetPathById_mediaType_audio end");
}

HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_CreateAssetPathById_mediaType_video, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_CreateAssetPathById_mediaType_video start");
    int32_t uniqueId = 2;
    int32_t fileType = 2;
    std::string extension = "mp4";
    std::string cloudPath;
    int32_t errCode = BackupFileUtils::CreateAssetPathById(uniqueId, fileType, extension, cloudPath);
    EXPECT_EQ(errCode, E_OK);
    MEDIA_INFO_LOG("BackupFileUtils_CreateAssetPathById_mediaType_video end");
}

HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_CreateAssetPathById_illegal_fileId, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_CreateAssetPathById_Illegal_fileId start");
    int32_t fileId = -1;
    int32_t fileType = 2;
    std::string extension = "mp4";
    std::string cloudPath;
    // fileId < 0 CreateAssetPathById will return E_INVALID_FILEID -208
    int32_t errCode = BackupFileUtils::CreateAssetPathById(fileId, fileType, extension, cloudPath);
    EXPECT_EQ(errCode, E_INVALID_FILEID);
    MEDIA_INFO_LOG("BackupFileUtils_CreateAssetPathById_Illegal_fileId end");
}

HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_CreateAssetPathById_Normal_fileId, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_CreateAssetPathById_Normal_fileId start");
    // file id > ASSET_DIR_START_NUM * ASSET_IN_BUCKET_NUM_MAX (16 * 1000) and Remainder != 0
    int32_t fileId = 16 * 1000 + 1;
    int32_t fileType = 2;
    std::string extension = "mp4";
    std::string cloudPath;
    int32_t errCode = BackupFileUtils::CreateAssetPathById(fileId, fileType, extension, cloudPath);
    EXPECT_EQ(errCode, E_OK);
    MEDIA_INFO_LOG("BackupFileUtils_CreateAssetPathById_Normal_fileId end");
}

HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_CreateAssetPathById_Normal_fileId_NoRemainder, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_CreateAssetPathById_Normal_fileId_NoRemainder start");
    // file id > ASSET_DIR_START_NUM * ASSET_IN_BUCKET_NUM_MAX (16 * 1000) and fileIdRemainder == 0
    int32_t fileId = 16 * 1000 + 16;
    int32_t fileType = 2;
    std::string extension = "mp4";
    std::string cloudPath;
    int32_t errCode = BackupFileUtils::CreateAssetPathById(fileId, fileType, extension, cloudPath);
    EXPECT_EQ(errCode, E_OK);
    MEDIA_INFO_LOG("BackupFileUtils_CreateAssetPathById_Normal_fileId_NoRemainder end");
}

HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_CreateAssetPathById_illegal_fileType_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_CreateAssetPathById_illegal_fileType_001 start");
    int32_t fileId = 2;
    int32_t fileType = -1;
    std::string extension = "mp4";
    std::string cloudPath;
    // normal fileId and illegal fileType CreateAssetPathById will return E_INVALID_VALUES
    int32_t errCode = BackupFileUtils::CreateAssetPathById(fileId, fileType, extension, cloudPath);
    EXPECT_EQ(errCode, E_INVALID_VALUES);
    MEDIA_INFO_LOG("BackupFileUtils_CreateAssetPathById_illegal_fileType_001 end");
}

HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_CreateAssetPathById_Illegal_fileType_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_CreateAssetPathById_Illegal_fileType_002 start");
    // fileId = ASSET_MAX_COMPLEMENT_ID + invalid file type
    int32_t fileId = 999;
    int32_t fileType = -1;
    std::string extension = "mp4";
    std::string cloudPath;
    int32_t errCode = BackupFileUtils::CreateAssetPathById(fileId, fileType, extension, cloudPath);
    EXPECT_EQ(errCode, E_INVALID_VALUES);
    MEDIA_INFO_LOG("BackupFileUtils_CreateAssetPathById_Illegal_fileType_002 end");
}

HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_CreateAssetPathById_Illegal_fileType_003, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_CreateAssetPathById_Illegal_fileType_003 start");
    // fileId > ASSET_MAX_COMPLEMENT_ID + invalid file type
    int32_t fileId = 1000;
    int32_t fileType = -1;
    std::string extension = "mp4";
    std::string cloudPath;
    int32_t errCode = BackupFileUtils::CreateAssetPathById(fileId, fileType, extension, cloudPath);
    EXPECT_EQ(errCode, E_INVALID_VALUES);
    MEDIA_INFO_LOG("BackupFileUtils_CreateAssetPathById_Illegal_fileType_003 end");
}

HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_GetFileNameFromPath_path_empty, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_GetFileNameFromPath_path_empty start");
    string path;
    string res = BackupFileUtils::GetFileNameFromPath(path);
    EXPECT_EQ(res, "");
    MEDIA_INFO_LOG("BackupFileUtils_GetFileNameFromPath_path_empty end");
}

HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_GetFileNameFromPath_path_not_have, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_GetFileNameFromPath_path_not_have start");
    string path = "test";
    string res = BackupFileUtils::GetFileNameFromPath(path);
    EXPECT_EQ(res, "");
    MEDIA_INFO_LOG("BackupFileUtils_GetFileNameFromPath_path_not_have end");
}

HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_GetFileNameFromPath_ok, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_GetFileNameFromPath_ok start");
    string path = "test/ee/ee";
    string res = BackupFileUtils::GetFileNameFromPath(path);
    EXPECT_NE(res, "");
    MEDIA_INFO_LOG("BackupFileUtils_GetFileNameFromPath_ok end");
}

HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_GetFileTitle_dispalyName_empty, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_GetFileTitle_dispalyName_empty start");
    string displayName;
    string res = BackupFileUtils::GetFileTitle(displayName);
    EXPECT_EQ(res, "");
    MEDIA_INFO_LOG("BackupFileUtils_GetFileTitle_dispalyName_empty end");
}

HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_GetFileTitle_ok, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_GetFileTitle_ok start");
    string displayName = "test.mp3";
    string res = BackupFileUtils::GetFileTitle(displayName);
    EXPECT_NE(res, "");
    MEDIA_INFO_LOG("BackupFileUtils_GetFileTitle_ok end");
}

HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_GetFileTitle_no_dot, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_GetFileTitle_no_dot start");
    string displayName = "testmp3";
    string res = BackupFileUtils::GetFileTitle(displayName);
    EXPECT_EQ(res, "testmp3");
    MEDIA_INFO_LOG("BackupFileUtils_GetFileTitle_no_dot end");
}

HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_GetFullPathByPrefixType_normal_type, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_GetFullPathByPrefixType_normal_type start");
    // test case 1 normal PrefixType
    string path = "/test.cpp";
    PrefixType type = PrefixType::CLOUD;
    string res = BackupFileUtils::GetFullPathByPrefixType(type, path);
    EXPECT_EQ(res, "/storage/cloud/files/test.cpp");
    MEDIA_INFO_LOG("BackupFileUtils_GetFullPathByPrefixType_normal_type end");
}

HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_GetFullPathByPrefixType_illegal_type, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_GetFullPathByPrefixType_illegal_type start");
    // test case 2 illegal PrefixType
    string path = "/test.cpp";
    PrefixType type = static_cast<PrefixType>(-1);
    string res = BackupFileUtils::GetFullPathByPrefixType(type, path);
    EXPECT_EQ(res.length(), 0);
    MEDIA_INFO_LOG("BackupFileUtils_GetFullPathByPrefixType_illegal_type end");
}

HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_GarbleFileName_normal_name, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_GarbleFileName_normal_name start");
    // name start with Screenshot_
    string name = "Screenshot_test";
    auto ret = BackupFileUtils::GarbleFileName(name);
    EXPECT_EQ(ret, "Screenshot_test");

    // name start with SVID_
    name = "SVID_test";
    ret = BackupFileUtils::GarbleFileName(name);
    EXPECT_EQ(ret, "SVID_test");

    // name start with IMG_
    name = "IMG_test";
    ret = BackupFileUtils::GarbleFileName(name);
    EXPECT_EQ(ret, "IMG_test");

    // name start with VID_
    name = "VID_test";
    ret = BackupFileUtils::GarbleFileName(name);
    EXPECT_EQ(ret, "VID_test");

    // name without extension and size <= GARBLE.size() * 2
    name = "test";
    ret = BackupFileUtils::GarbleFileName(name);
    EXPECT_EQ(ret, name);

    // name with extension
    name = "test_garble_file_name.ext";
    ret = BackupFileUtils::GarbleFileName(name);
    EXPECT_EQ(ret.find(GARBLE), 0);
    MEDIA_INFO_LOG("BackupFileUtils_GarbleFileName_normal_name end");
}

HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_GarbleFileName_empty_name, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_GarbleFileName_empty_name start");
    // empty file name
    string name;
    auto ret = BackupFileUtils::GarbleFileName(name);
    EXPECT_EQ(ret.length(), 0);
    MEDIA_INFO_LOG("BackupFileUtils_GarbleFileName_empty_name end");
}

HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_GetReplacedPathByPrefixType_illegal_srcprefix, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_GetReplacedPathByPrefixType_illegal_srcprefix start");
    // illegal srcPrefix + normal dstPrefix
    PrefixType srcPrefix = static_cast<PrefixType>(-1);
    PrefixType dstPrefix = PrefixType::CLOUD_EDIT_DATA;
    string path = "test_path";
    auto ret = BackupFileUtils::GetReplacedPathByPrefixType(srcPrefix, dstPrefix, path);
    EXPECT_EQ(ret.length(), 0);
    MEDIA_INFO_LOG("BackupFileUtils_GetReplacedPathByPrefixType_illegal_srcprefix end");
}

HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_GetReplacedPathByPrefixType_illegal_dstprefix, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_GetReplacedPathByPrefixType_illegal_dstprefix start");
    // illegal dstPrefix + normal srcPrefix
    PrefixType srcPrefix = PrefixType::CLOUD_EDIT_DATA;
    PrefixType dstPrefix = static_cast<PrefixType>(-1);
    string path = "test_path";
    auto ret = BackupFileUtils::GetReplacedPathByPrefixType(srcPrefix, dstPrefix, path);
    EXPECT_EQ(ret.length(), 0);
    MEDIA_INFO_LOG("BackupFileUtils_GetReplacedPathByPrefixType_illegal_dstprefix end");
}

HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_GetReplacedPathByPrefixType_normal_prefix, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_GetReplacedPathByPrefixType_normal_prefix start");
    // normal srcPrefix + normal dstPrefix
    PrefixType srcPrefix = PrefixType::LOCAL;
    PrefixType dstPrefix = PrefixType::LOCAL;
    string path = "test_path";
    auto ret = BackupFileUtils::GetReplacedPathByPrefixType(srcPrefix, dstPrefix, path);
    EXPECT_EQ(ret, "/storage/media/local/files");
    MEDIA_INFO_LOG("BackupFileUtils_GetReplacedPathByPrefixType_normal_prefix end");
}

HWTEST_F(MediaLibraryBackupTest, RestoreAudio_sceneCode_UPGRADE_RESTORE_ID, TestSize.Level2)
{
    MEDIA_INFO_LOG("RestoreAudio_sceneCode_UPGRADE_RESTORE_ID start");
    restoreService->RestoreAudio();
    std::string queryAudio = "SELECT file_id from Audios where display_name ='audio1.mp3'";
    auto resultSet = photosStorePtr->QuerySql(queryAudio);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("RestoreAudio_sceneCode_UPGRADE_RESTORE_ID end");
}

HWTEST_F(MediaLibraryBackupTest, RestoreAudio_sceneCode_Clone, TestSize.Level2)
{
    MEDIA_INFO_LOG("RestoreAudio_sceneCode_Clone start");
    restoreService->sceneCode_ = DUAL_FRAME_CLONE_RESTORE_ID;
    restoreService->RestoreAudio();
    std::string queryAudio = "SELECT file_id from Audios where display_name ='audio1.mp3'";
    auto resultSet = photosStorePtr->QuerySql(queryAudio);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("RestoreAudio_sceneCode_Clone end");
}

HWTEST_F(MediaLibraryBackupTest, RestoreAudio_RestoreAudioBatch_clone_no_audioRdb, TestSize.Level2)
{
    MEDIA_INFO_LOG("RestoreAudio_RestoreAudioBatch_clone_no_db start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    upgrade->RestoreAudioBatch(0);
    std::string queryAudio = "SELECT file_id from Audios where display_name ='audio1.mp3'";
    auto resultSet = photosStorePtr->QuerySql(queryAudio);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("RestoreAudio_RestoreAudioBatch_clone_no_db end");
}

HWTEST_F(MediaLibraryBackupTest, RestoreAudio_RestoreAudioBatch_clone_fake_audiodb, TestSize.Level2)
{
    MEDIA_INFO_LOG("RestoreAudio_RestoreAudioBatch_clone_fake_audiodb start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    upgrade->audioRdb_ = photosStorePtr;
    upgrade->RestoreAudioBatch(0);
    std::string queryAudio = "SELECT file_id from Audios where display_name ='audio1.mp3'";
    auto resultSet = photosStorePtr->QuerySql(queryAudio);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("RestoreAudio_RestoreAudioBatch_clone_no_db end");
}

HWTEST_F(MediaLibraryBackupTest, RestoreAudio_ParseResultSetFromAudioDb_return_false, TestSize.Level2)
{
    MEDIA_INFO_LOG("RestoreAudio_ParseResultSetFromAudioDb_return_false start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    std::shared_ptr<NativeRdb::ResultSet> resultSet;
    FileInfo info;
    bool res = upgrade->ParseResultSetFromAudioDb(resultSet, info);
    EXPECT_NE(res, true);
    MEDIA_INFO_LOG("RestoreAudio_ParseResultSetFromAudioDb_return_false end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test001, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test001 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test001 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_id from PhotoAlbum where album_name = 'test001';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test001 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test001 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test002, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test002 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test002 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_id from PhotoAlbum where album_name = 'test002';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test002 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test003, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test003 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test003 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_id from PhotoAlbum where album_name = 'test003';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test003 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test003 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test004, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test004 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test004 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_id from PhotoAlbum where album_name = 'TmallPic';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_TRUE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test004 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test004 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test005, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test005 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test005 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_id from PhotoAlbum where album_name = 'UCDownloads';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test005 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test005 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test006, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test006 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test006 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_id from PhotoAlbum where album_name  = 'xiaohongshu';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_TRUE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test006 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test006 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test007, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test007 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test007 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_id from PhotoAlbum where album_name = 'Douyin';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_TRUE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test007 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test007 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test008, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test008 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test008 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_id from PhotoAlbum where album_name = 'Weibo';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_TRUE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test008 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test008 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test009, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test009 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test009 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_id from PhotoAlbum where album_name = 'Camera';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_TRUE(resultSet->GoToNextRow() == NativeRdb::E_OK);

    queryTrashed = "SELECT file_id from Photos where display_name = 'camera1.jpg';";
    resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test009 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test009 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test010, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test010 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test010 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_id from PhotoAlbum where album_name = 'Screenshots';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_TRUE(resultSet->GoToNextRow() == NativeRdb::E_OK);

    queryTrashed = "SELECT file_id from Photos where display_name = 'screenshots1.jpg';";
    resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test010 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test010 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test011, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test011 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test011 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_id from PhotoAlbum where album_name = 'Screenrecorder';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_TRUE(resultSet->GoToNextRow() == NativeRdb::E_OK);

    queryTrashed = "SELECT file_id from Photos where display_name = 'screenrecorder1.mp4';";
    resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test011 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test011 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test012, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test012 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test012 start");
    restoreService->RestorePhoto();
    std::string sql = "SELECT album_id from PhotoAlbum where album_name = '" + GetDUALBundleName() + " Share';";
    auto resultSet = photosStorePtr->QuerySql(sql);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_TRUE(resultSet->GoToNextRow() == NativeRdb::E_OK);

    sql = "SELECT file_id from Photos where display_name = '" + GetDUALBundleName() + "Share1.jpg';";
    resultSet = photosStorePtr->QuerySql(sql);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test012 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test012 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test101, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test101 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test101 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_type,album_subtype from PhotoAlbum where album_name = 'test101';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    int32_t album_type = GetInt32Val("album_type", resultSet);
    int32_t album_subtype = GetInt32Val("album_subtype", resultSet);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_TRUE(resultSet->GoToNextRow() == NativeRdb::E_OK);

    EXPECT_EQ(album_type, 0);
    EXPECT_EQ(album_subtype, 0);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test101 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test101 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test102, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test102 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test102 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_id from PhotoAlbum where album_name = 'TmallPic';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_TRUE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test102 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test102 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test103, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test103 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test103 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_id from PhotoAlbum where album_name = 'MTTT';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test103 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test103 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test104, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test104 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test104 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_id from PhotoAlbum where album_name = 'funnygallery';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_TRUE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test104 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test104 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test105, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test105 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test105 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_id from PhotoAlbum where album_name = 'xiaohongshu';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_TRUE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test105 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test105 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test106, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test106 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test106 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_id from PhotoAlbum where album_name = 'Douyin';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_TRUE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test106 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test106 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test107, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test107 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test107 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_id from PhotoAlbum where album_name = 'save';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_TRUE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test107 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test107 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test108, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test108 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test108 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_id from PhotoAlbum where album_name = 'Weibo';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_TRUE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test108 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test108 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test109, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test109 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test109 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_id from PhotoAlbum where album_name = 'Camera';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_TRUE(resultSet->GoToNextRow() == NativeRdb::E_OK);

    queryTrashed = "SELECT file_id from Photos where display_name = 'camera2.jpg';";
    resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test109 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test109 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test110, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test110 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test110 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_id from PhotoAlbum where album_name = 'Screenshots';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_TRUE(resultSet->GoToNextRow() == NativeRdb::E_OK);

    queryTrashed = "SELECT file_id from Photos where display_name = 'screenshots2.jpg';";
    resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test110 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test110 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test111, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test111 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test111 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_id from PhotoAlbum where album_name = 'Screenrecorder';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_TRUE(resultSet->GoToNextRow() == NativeRdb::E_OK);

    queryTrashed = "SELECT file_id from Photos where display_name = 'screenrecorder2.mp4';";
    resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test111 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test111 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ablum_test112, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test112 start";
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test112 start");
    restoreService->RestorePhoto();
    std::string queryTrashed = "SELECT album_id from PhotoAlbum where album_name = '" + GetDUALBundleName() +" Share';";
    auto resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_TRUE(resultSet->GoToNextRow() == NativeRdb::E_OK);

    queryTrashed = "SELECT file_id from Photos where display_name = '" + GetDUALBundleName() + "Share2.jpg';";
    resultSet = photosStorePtr->QuerySql(queryTrashed);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_ablum_test112 end");
    GTEST_LOG_(INFO) << "medialib_backup_test_ablum_test112 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_InsertAudio_upgrade, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_InsertAudio_upgrade start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    upgrade->mediaLibraryRdb_ = photosStorePtr;
    uint64_t fileNumber = upgrade->migrateAudioFileNumber_;
    upgrade->migrateAudioFileNumber_ = 0;
    std::vector<FileInfo> fileInfos;
    FileInfo fileInfo1;
    fileInfo1.filePath = "test";
    fileInfos.push_back(fileInfo1);
    FileInfo fileInfo2;
    fileInfo2.filePath = "/storage/cloud/100/files/Documents/CreateImageThumbnailTest_001.jpg";
    fileInfo2.cloudPath = "/storage/cloud/files/Audio/10/AUD_3322.jpg";
    fileInfos.push_back(fileInfo2);
    upgrade->InsertAudio(UPGRADE_RESTORE_ID, fileInfos);
    EXPECT_EQ(upgrade->migrateAudioFileNumber_, 0);
    upgrade->migrateAudioFileNumber_ = fileNumber;
    MEDIA_INFO_LOG("medialib_backup_InsertAudio_upgrade end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_InsertAudio_empty_file, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_InsertAudio_empty_file start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    upgrade->mediaLibraryRdb_ = photosStorePtr;
    uint64_t fileNumber = upgrade->migrateAudioFileNumber_;
    upgrade->migrateAudioFileNumber_ = 0;
    std::vector<FileInfo> fileInfos;
    upgrade->InsertAudio(DUAL_FRAME_CLONE_RESTORE_ID, fileInfos);
    EXPECT_EQ(upgrade->migrateAudioFileNumber_, 0);
    upgrade->migrateAudioFileNumber_ = fileNumber;
    MEDIA_INFO_LOG("medialib_backup_InsertAudio_empty_file end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_MoveDirectory, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_MoveDirectory start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    string srcDir = "/data/test";
    string dstDir = "/data/test1";
    EXPECT_EQ(upgrade->MoveDirectory(srcDir, dstDir), E_OK);
    // delete the destination directory after moving to prevent errors caused by an existing directory
    auto ret = MediaFileUtils::DeleteDir(dstDir);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("medialib_backup_MoveDirectory end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_BatchQueryPhoto, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_BatchQueryPhoto start");
    vector<FileInfo> fileInfos;
    FileInfo fileInfo1;
    fileInfo1.cloudPath = "test";
    fileInfo1.mediaAlbumId = TEST_FALSE_MEDIAID;
    fileInfos.push_back(fileInfo1);

    FileInfo fileInfo2;
    fileInfo2.cloudPath = "";
    fileInfo2.mediaAlbumId = TEST_FALSE_MEDIAID;
    fileInfos.push_back(fileInfo2);

    FileInfo fileInfo3;
    fileInfo3.cloudPath = "test";
    fileInfo3.mediaAlbumId = TEST_TRUE_MEDIAID;
    fileInfos.push_back(fileInfo3);

    FileInfo fileInfo4;
    fileInfo4.cloudPath = "";
    fileInfo4.mediaAlbumId = TEST_TRUE_MEDIAID;
    fileInfos.push_back(fileInfo4);
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    NeedQueryMap needQueryMap;
    upgrade->NeedBatchQueryPhotoForPhotoMap(fileInfos, needQueryMap);
    upgrade->BatchQueryPhoto(fileInfos, false, needQueryMap);
    EXPECT_EQ(fileInfos[0].fileIdNew, TEST_FALSE_MEDIAID);
    MEDIA_INFO_LOG("medialib_backup_BatchQueryPhoto end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ParseXml_002, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ParseXml_002 start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    string xmlPath = "/data/test/backup/test1.xml";
    auto res = upgrade->ParseXml(xmlPath);
    EXPECT_EQ(res, -1);
    GTEST_LOG_(INFO) << "medialib_backup_test_ParseXml_002 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ParseXml_003, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ParseXml_003 start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    string xmlPath = "/data/test/backup/test2.xml";
    auto res = upgrade->ParseXml(xmlPath);
    EXPECT_EQ(res, -1);
    GTEST_LOG_(INFO) << "medialib_backup_test_ParseXml_003 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ParseXml_004, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ParseXml_004 start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    string xmlPath = "/data/test/backup/test3.xml";
    auto res = upgrade->ParseXml(xmlPath);
    EXPECT_EQ(res, -1);
    GTEST_LOG_(INFO) << "medialib_backup_test_ParseXml_004 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ParseXml_005, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_ParseXml_005 start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    string xmlPath = "/data/test/backup/test4.xml";
    auto res = upgrade->ParseXml(xmlPath);
    EXPECT_EQ(res, -1);
    GTEST_LOG_(INFO) << "medialib_backup_test_ParseXml_005 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_StringToInt_001, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_StringToInt_001 start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    string xmlPath = "";
    auto res = upgrade->StringToInt(xmlPath);
    EXPECT_EQ(res, 0);
    GTEST_LOG_(INFO) << "medialib_backup_test_StringToInt_001 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_StringToInt_002, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_StringToInt_002 start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    string xmlPath = "   22e";
    auto res = upgrade->StringToInt(xmlPath);
    EXPECT_EQ(res, 0);
    GTEST_LOG_(INFO) << "medialib_backup_test_StringToInt_002 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_StringToInt_003, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_StringToInt_003 start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    string xmlPath = "777777777777777777777777777777777777777777777777777";
    auto res = upgrade->StringToInt(xmlPath);
    EXPECT_EQ(res, 0);
    GTEST_LOG_(INFO) << "medialib_backup_test_StringToInt_003 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_SetValueFromMetaData, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_SetValueFromMetaData start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);

    // hasOrientation = true and fileinfo.fileType != MEDIA_TYPE_VIDEO
    FileInfo fileInfo;
    fileInfo.fileType = MEDIA_TYPE_FILE;
    NativeRdb::ValuesBucket valueBucket;
    valueBucket.PutString(PhotoColumn::PHOTO_ORIENTATION, "test");
    upgrade->SetValueFromMetaData(fileInfo, valueBucket);
    GTEST_LOG_(INFO) << "medialib_backup_test_SetValueFromMetaData end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_SetUserComment_TEST1, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_SetUserComment_TEST1 start";
    FileInfo fileInfo;
    fileInfo.filePath = "TEST";
    fileInfo.userComment = "TEST123";
    NativeRdb::ValuesBucket valueBucket;
    // no column user comment
    bool hasUserComment = valueBucket.HasColumn(PhotoColumn::PHOTO_USER_COMMENT);
    EXPECT_EQ(hasUserComment, false);
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, UPGRADE_RESTORE_ID);
    upgrade->SetValueFromMetaData(fileInfo, valueBucket);
    hasUserComment = valueBucket.HasColumn(PhotoColumn::PHOTO_USER_COMMENT);
    EXPECT_EQ(hasUserComment, true);
    GTEST_LOG_(INFO) << "medialib_backup_test_SetUserComment_TEST1 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_SetUserComment_TEST2, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_SetUserComment_TEST2 start";
    FileInfo fileInfo;
    fileInfo.filePath = "TEST";
    fileInfo.userComment = "TEST123";
    NativeRdb::ValuesBucket valueBucket;
    // has column user comment
    valueBucket.PutString(PhotoColumn::PHOTO_USER_COMMENT, fileInfo.userComment);
    bool hasUserComment = valueBucket.HasColumn(PhotoColumn::PHOTO_USER_COMMENT);
    EXPECT_EQ(hasUserComment, true);
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, UPGRADE_RESTORE_ID);
    upgrade->SetValueFromMetaData(fileInfo, valueBucket);
    hasUserComment = valueBucket.HasColumn(PhotoColumn::PHOTO_USER_COMMENT);
    EXPECT_EQ(hasUserComment, true);
    GTEST_LOG_(INFO) << "medialib_backup_test_SetUserComment_TEST2 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GetBackupInfo, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_GetBackupInfo start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);

    string str = upgrade->GetBackupInfo();
    EXPECT_EQ(str, "");
    GTEST_LOG_(INFO) << "medialib_backup_test_GetBackupInfo end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_InsertPhoto_empty, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_InsertPhoto_empty start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);

    // empty mediaLibraryRdb_
    int sceneCode = 0;
    int32_t sourceType = 0;
    vector<FileInfo> fileInfos;
    upgrade->mediaLibraryRdb_ = nullptr;
    upgrade->InsertPhoto(sceneCode, fileInfos, sourceType);

    //normal mediaLibraryRdb_ and empty file
    upgrade->mediaLibraryRdb_ = photosStorePtr;
    upgrade->InsertPhoto(sceneCode, fileInfos, sourceType);
    GTEST_LOG_(INFO) << "medialib_backup_test_InsertPhoto_empty end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_InsertPhoto_normal, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_InsertPhoto_normal start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);

    int sceneCode = 0;
    FileInfo fileInfo1;
    FileInfo fileInfo2;
    int32_t sourceType = SourceType::GALLERY;
    upgrade->mediaLibraryRdb_ = photosStorePtr;

    vector<FileInfo> fileInfos;
    fileInfo1.filePath = "test";
    fileInfos.push_back(fileInfo1);
    fileInfo2.filePath = "/storage/cloud/100/files/Documents/CreateImageThumbnailTest_001.jpg";
    fileInfo2.cloudPath = "storage/cloud/files/Audio/10/AUD_3322/jpg";
    fileInfos.push_back(fileInfo2);
    upgrade->InsertPhoto(sceneCode, fileInfos, sourceType);

    // insertPhoto sourceType != SourceType::GALLERY
    sourceType = SourceType::EXTERNAL_CAMERA;
    upgrade->InsertPhoto(sceneCode, fileInfos, sourceType);
    GTEST_LOG_(INFO) << "medialib_backup_test_InsertPhoto_normal end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_InsertCloudPhoto, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_InsertCloudPhoto start");
    int sceneCode = 0;
    int32_t sourceType = SourceType::GALLERY;
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    FileInfo fileInfo;
    vector<FileInfo> fileInfos = {fileInfo};
    upgrade->mediaLibraryRdb_ = photosStorePtr;
    int32_t ret = upgrade->InsertCloudPhoto(sceneCode, fileInfos, sourceType);
    EXPECT_EQ(ret, E_OK);
    upgrade->mediaLibraryRdb_ = nullptr;
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_UpdateFailedFileByFileType_image, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_UpdateFailedFileByFileType_image start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);

    // type = image
    int32_t errCode = 0;
    string path = "image/";
    string type = STAT_TYPE_PHOTO;
    int32_t fileType = MediaType::MEDIA_TYPE_IMAGE;
    FileInfo fileInfo;
    fileInfo.oldPath = path;

    upgrade->UpdateFailedFileByFileType(fileType, fileInfo, errCode);
    auto ret = upgrade->GetFailedFiles(type);
    // ret[path] = errCode
    EXPECT_EQ(ret[path].errorCode, to_string(errCode));
    GTEST_LOG_(INFO) << "medialib_backup_test_UpdateFailedFileByFileType_image end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_UpdateFailedFileByFileType_video, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_UpdateFailedFileByFileType_video start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);

    // type = video
    int32_t errCode = 1;
    string path = "video/";
    string type = STAT_TYPE_VIDEO;
    int32_t fileType = MediaType::MEDIA_TYPE_VIDEO;
    FileInfo fileInfo;
    fileInfo.oldPath = path;

    upgrade->UpdateFailedFileByFileType(fileType, fileInfo, errCode);
    auto ret = upgrade->GetFailedFiles(type);
    // ret[path] = errCode
    EXPECT_EQ(ret[path].errorCode, to_string(errCode));
    GTEST_LOG_(INFO) << "medialib_backup_test_UpdateFailedFileByFileType_video end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_UpdateFailedFileByFileType_audio, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_UpdateFailedFileByFileType_audio start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);

    // type = audio
    int32_t errCode = -1;
    string path = "audio/";
    string type = STAT_TYPE_AUDIO;
    int32_t fileType = MediaType::MEDIA_TYPE_AUDIO;
    FileInfo fileInfo;
    fileInfo.oldPath = path;

    upgrade->UpdateFailedFileByFileType(fileType, fileInfo, errCode);
    auto ret = upgrade->GetFailedFiles(type);
    // ret[path] = errCode
    EXPECT_EQ(ret[path].errorCode, to_string(errCode));
    GTEST_LOG_(INFO) << "medialib_backup_test_UpdateFailedFileByFileType_audio end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_UpdateFailedFileByFileType_illegal_filetype, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_UpdateFailedFileByFileType_illegal_filetype start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);

    // illegal file type
    int32_t fileType = -1;
    string path = "test_path";
    int32_t errCode = 3;
    FileInfo fileInfo;
    fileInfo.oldPath = path;

    upgrade->UpdateFailedFileByFileType(fileType, fileInfo, errCode);
    auto ret = upgrade->GetErrorInfoJson();
    string str = ret[STAT_KEY_ERROR_INFO].dump();
    str.erase(std::remove(str.begin(), str.end(), '\"'), str.end());
    EXPECT_EQ(str, "");
    GTEST_LOG_(INFO) << "medialib_backup_test_UpdateFailedFileByFileType_illegal_filetype end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_SetErrorCode, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_SetErrorCode start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);

    // errCode = INIT_FAILED
    int32_t errCode = INIT_FAILED;
    upgrade->SetErrorCode(errCode);
    auto ret = upgrade->GetErrorInfoJson();
    string str = ret[STAT_KEY_ERROR_INFO].dump();
    str.erase(std::remove(str.begin(), str.end(), '\"'), str.end());
    EXPECT_EQ(str, "RESTORE_INIT_FAILED");

    // illegal errCode
    errCode = 999;
    upgrade->SetErrorCode(errCode);
    ret = upgrade->GetErrorInfoJson();
    str = ret[STAT_KEY_ERROR_INFO].dump();
    str.erase(std::remove(str.begin(), str.end(), '\"'), str.end());
    EXPECT_EQ(str.length(), 0);
    GTEST_LOG_(INFO) << "medialib_backup_test_SetErrorCode end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GetSubCountInfo_illegal_type, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_GetSubCountInfo_illegal_type start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);

    // case 1 illegal type GetSubCountInfo will return empty failedFiles
    string type = "test_types";
    SubCountInfo ret = upgrade->GetSubCountInfo(type);
    EXPECT_EQ(ret.failedFiles.empty(), 1);
    GTEST_LOG_(INFO) << "medialib_backup_test_GetSubCountInfo_illegal_type end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GetCountInfoJson_empty_types, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_GetCountInfoJson_empty_types start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);

    vector<string> types;
    auto ret = upgrade->GetCountInfoJson(types);
    //json node not available will return null
    string str = ret[STAT_KEY_INFOS].dump();
    str.erase(std::remove(str.begin(), str.end(), '\"'), str.end());
    EXPECT_EQ(str, "null");
    GTEST_LOG_(INFO) << "medialib_backup_test_GetCountInfoJson_empty_types end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GetCountInfoJson_normal_types, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_GetCountInfoJson_normal_types start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);

    vector<string> types = STAT_TYPES;
    auto ret = upgrade->GetCountInfoJson(types);
    // stat_types[1] = video ret[1][STAT_KEY_BACKUP_INFO] = video
    auto str = ret[STAT_KEY_INFOS][1][STAT_KEY_BACKUP_INFO].dump();
    str.erase(std::remove(str.begin(), str.end(), '\"'), str.end());
    EXPECT_EQ(str, "video");
    GTEST_LOG_(INFO) << "medialib_backup_test_GetCountInfoJson_normal_types end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GetCountInfoJson_illegal_types, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_GetCountInfoJson_illegal_types start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);

    vector<string> types = { "test" };
    auto ret = upgrade->GetCountInfoJson(types);
    string str = ret[STAT_KEY_INFOS][1][STAT_KEY_BACKUP_INFO].dump();
    str.erase(std::remove(str.begin(), str.end(), '\"'), str.end());
    EXPECT_NE(str, "test");
    GTEST_LOG_(INFO) << "medialib_backup_test_GetCountInfoJson_illegal_types end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_need_batch_query_photo, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_need_batch_query_photo start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, UPGRADE_RESTORE_ID);
    std::vector<FileInfo> fileInfos;
    NeedQueryMap needQueryMap;
    auto ret = upgrade->NeedBatchQueryPhoto(fileInfos, needQueryMap);
    EXPECT_EQ(ret, false);
    EXPECT_EQ(needQueryMap.empty(), true);
    GTEST_LOG_(INFO) << "medialib_backup_test_need_batch_query_photo end";
}

void TestConvertPathToRealPathByStorageType(bool isSd)
{
    std::string srcPath = isSd ? "/storage/ABCD-0000" : "/storage/emulated/0";
    srcPath += TEST_RELATIVE_PATH;
    FileInfo fileInfo;
    fileInfo.fileSize = TEST_SIZE_2MB_BELOW;
    fileInfo.localMediaId = TEST_LOCAL_MEDIA_ID;
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    std::string newPath;
    std::string relativePath;
    bool result = upgrade->ConvertPathToRealPath(srcPath, TEST_PATH_PREFIX, newPath, relativePath, fileInfo);
    EXPECT_EQ(result, true);
    EXPECT_EQ(newPath, TEST_PATH_PREFIX + srcPath);
    EXPECT_EQ(relativePath, TEST_RELATIVE_PATH);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_convert_path_to_real_path_001, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_001 start";
    TestConvertPathToRealPathByStorageType(false); // internal, normal
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_001 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_convert_path_to_real_path_002, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_002 start";
    TestConvertPathToRealPathByStorageType(true); // sd card, normal
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_002 end";
}

void TestConvertPathToRealPathByFileSize(int64_t fileSize, const std::string &srcPath,
    const std::string &expectedNewPath)
{
    FileInfo fileInfo;
    fileInfo.fileSize = fileSize;
    fileInfo.localMediaId = TEST_LOCAL_MEDIA_ID;
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    std::string newPath;
    std::string relativePath;
    bool result = upgrade->ConvertPathToRealPath(srcPath, TEST_PATH_PREFIX, newPath, relativePath, fileInfo);
    EXPECT_EQ(result, true);
    EXPECT_EQ(newPath, expectedNewPath);
    EXPECT_EQ(relativePath, TEST_RELATIVE_PATH);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_convert_path_to_real_path_003, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_003 start";
    std::string srcPath = "/storage/ABCD-0000" + TEST_RELATIVE_PATH;
    std::string expectedNewPath = TEST_PATH_PREFIX + srcPath;
    TestConvertPathToRealPathByFileSize(TEST_SIZE_2MB_BELOW, srcPath, expectedNewPath); // Sd, below 2MB
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_003 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_convert_path_to_real_path_004, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_004 start";
    std::string srcPath = "/storage/ABCD-0000" + TEST_RELATIVE_PATH;
    std::string expectedNewPath = TEST_PATH_PREFIX + TEST_RELATIVE_PATH;
    TestConvertPathToRealPathByFileSize(TEST_SIZE_2MB, srcPath, expectedNewPath); // Sd, equal to 2MB
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_004 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_convert_path_to_real_path_005, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_005 start";
    std::string srcPath = "/storage/ABCD-0000" + TEST_RELATIVE_PATH;
    std::string expectedNewPath = TEST_PATH_PREFIX + TEST_RELATIVE_PATH;
    TestConvertPathToRealPathByFileSize(TEST_SIZE_2MB_ABOVE, srcPath, expectedNewPath); // Sd, above 2MB
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_005 end";
}

void TestConvertPathToRealPathByLocalMediaId(int32_t localMediaId, const std::string &srcPath,
    const std::string &expectedNewPath)
{
    FileInfo fileInfo;
    fileInfo.fileSize = TEST_SIZE_2MB_ABOVE;
    fileInfo.localMediaId = localMediaId;
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    std::string newPath;
    std::string relativePath;
    bool result = upgrade->ConvertPathToRealPath(srcPath, TEST_PATH_PREFIX, newPath, relativePath, fileInfo);
    EXPECT_EQ(result, true);
    EXPECT_EQ(newPath, expectedNewPath);
    EXPECT_EQ(relativePath, TEST_RELATIVE_PATH);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_convert_path_to_real_path_006, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_006 start";
    std::string srcPath = "/storage/ABCD-0000" + TEST_RELATIVE_PATH;
    std::string expectedNewPath = TEST_PATH_PREFIX + srcPath;
    TestConvertPathToRealPathByLocalMediaId(GALLERY_HIDDEN_ID, srcPath, expectedNewPath); // Sd, hidden
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_006 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_convert_path_to_real_path_007, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_007 start";
    std::string srcPath = "/storage/ABCD-0000" + TEST_RELATIVE_PATH;
    std::string expectedNewPath = TEST_PATH_PREFIX + srcPath;
    TestConvertPathToRealPathByLocalMediaId(GALLERY_TRASHED_ID, srcPath, expectedNewPath); // Sd, trashed
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_007 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_get_path_pos_by_prefix_level, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_get_path_pos_by_prefix_level start";
    std::string path = "/../";
    size_t pos = 0;
    bool result = BackupFileUtils::GetPathPosByPrefixLevel(DUAL_FRAME_CLONE_RESTORE_ID, path, TEST_PREFIX_LEVEL,
        pos);
    EXPECT_EQ(result, false);
    GTEST_LOG_(INFO) << "medialib_backup_test_get_path_pos_by_prefix_level end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_update_duplicate_number, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_update_duplicate_number start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    upgrade->UpdateDuplicateNumber(MediaType::MEDIA_TYPE_IMAGE);
    EXPECT_GT(upgrade->migratePhotoDuplicateNumber_, 0);
    upgrade->UpdateDuplicateNumber(MediaType::MEDIA_TYPE_VIDEO);
    EXPECT_GT(upgrade->migrateVideoDuplicateNumber_, 0);
    upgrade->UpdateDuplicateNumber(MediaType::MEDIA_TYPE_AUDIO);
    EXPECT_GT(upgrade->migrateAudioDuplicateNumber_, 0);
    uint64_t photoBefore = upgrade->migratePhotoDuplicateNumber_;
    uint64_t videoBefore = upgrade->migrateVideoDuplicateNumber_;
    uint64_t audioBefore = upgrade->migrateAudioDuplicateNumber_;
    upgrade->UpdateDuplicateNumber(-1);
    EXPECT_EQ(upgrade->migratePhotoDuplicateNumber_, photoBefore);
    EXPECT_EQ(upgrade->migrateVideoDuplicateNumber_, videoBefore);
    EXPECT_EQ(upgrade->migrateAudioDuplicateNumber_, audioBefore);
    GTEST_LOG_(INFO) << "medialib_backup_test_update_duplicate_number end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_update_sd_where_clause, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_update_sd_where_clause start";
    std::string whereClause;
    BackupDatabaseUtils::UpdateSdWhereClause(whereClause, true);
    EXPECT_EQ(whereClause.empty(), true);
    BackupDatabaseUtils::UpdateSdWhereClause(whereClause, false);
    EXPECT_EQ(whereClause.empty(), false);
    GTEST_LOG_(INFO) << "medialib_backup_test_update_sd_where_clause end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_get_landmarks_scale_001, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_get_landmarks_scale_001 start";
    int length = TEST_SIZE_MIN; // (0, 2 * min), no need to scale
    float scale = BackupDatabaseUtils::GetLandmarksScale(length, length);
    EXPECT_EQ(scale, 1);
    GTEST_LOG_(INFO) << "medialib_backup_test_get_landmarks_scale_001 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_get_landmarks_scale_002, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_get_landmarks_scale_002 start";
    int length = TEST_SIZE_MIN * TEST_SIZE_MULT_UNIT + TEST_SIZE_INCR_UNIT; // [2 * min, 4 * min)
    float scale = BackupDatabaseUtils::GetLandmarksScale(length, length);
    float expectedScale = 1.0 / TEST_SIZE_MULT_UNIT;
    EXPECT_EQ(scale, expectedScale);
    GTEST_LOG_(INFO) << "medialib_backup_test_get_landmarks_scale_002 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_get_landmarks_scale_003, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_get_landmarks_scale_003 start";
    int length = TEST_SIZE_MIN * TEST_SIZE_MULT_UNIT * TEST_SIZE_MULT_UNIT; // [4 * min, ...)
    float scale = BackupDatabaseUtils::GetLandmarksScale(length, length);
    float expectedScale = 1.0 / TEST_SIZE_MULT_UNIT / TEST_SIZE_MULT_UNIT;
    EXPECT_EQ(scale, expectedScale);
    GTEST_LOG_(INFO) << "medialib_backup_test_get_landmarks_scale_003 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_get_landmarks_scale_004, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_get_landmarks_scale_004 start";
    int width = TEST_SIZE_MIN;
    int height = TEST_SIZE_MAX * TEST_SIZE_MULT_UNIT; // max len exceeds
    float scale = BackupDatabaseUtils::GetLandmarksScale(width, height);
    float expectedScale = 1.0 / TEST_SIZE_MULT_UNIT;
    EXPECT_EQ(scale, expectedScale);
    GTEST_LOG_(INFO) << "medialib_backup_test_get_landmarks_scale_004 end";
}

void InitFaceInfoScale(FaceInfo &faceInfo, float scaleX, float scaleY, float scaleWidth, float scaleHeight)
{
    faceInfo.scaleX = scaleX;
    faceInfo.scaleY = scaleY;
    faceInfo.scaleWidth = scaleWidth;
    faceInfo.scaleHeight = scaleHeight;
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_is_landmark_valid_001, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_is_landmark_valid_001 start";
    FaceInfo faceInfo;
    InitFaceInfoScale(faceInfo, TEST_SCALE_MIN, TEST_SCALE_MIN, TEST_SCALE_MAX - TEST_SCALE_MIN,
        TEST_SCALE_MAX - TEST_SCALE_MIN);
    bool result = BackupDatabaseUtils::IsLandmarkValid(faceInfo, TEST_LANDMARK_BETWEEN, TEST_LANDMARK_BETWEEN);
    EXPECT_EQ(result, true);
    GTEST_LOG_(INFO) << "medialib_backup_test_is_landmark_valid_001 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_is_landmark_valid_002, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_is_landmark_valid_002 start";
    FaceInfo faceInfo;
    InitFaceInfoScale(faceInfo, TEST_SCALE_MIN, TEST_SCALE_MIN, TEST_SCALE_MAX - TEST_SCALE_MIN,
        TEST_SCALE_MAX - TEST_SCALE_MIN);
    bool result = BackupDatabaseUtils::IsLandmarkValid(faceInfo, TEST_LANDMARK_BETWEEN, TEST_LANDMARK_BELOW);
    EXPECT_EQ(result, false);
    GTEST_LOG_(INFO) << "medialib_backup_test_is_landmark_valid_002 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_is_landmark_valid_003, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_is_landmark_valid_003 start";
    FaceInfo faceInfo;
    InitFaceInfoScale(faceInfo, TEST_SCALE_MIN, TEST_SCALE_MIN, TEST_SCALE_MAX - TEST_SCALE_MIN,
        TEST_SCALE_MAX - TEST_SCALE_MIN);
    bool result = BackupDatabaseUtils::IsLandmarkValid(faceInfo, TEST_LANDMARK_BETWEEN, TEST_LANDMARK_ABOVE);
    EXPECT_EQ(result, false);
    GTEST_LOG_(INFO) << "medialib_backup_test_is_landmark_valid_003 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_is_landmark_valid_004, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_is_landmark_valid_004 start";
    FaceInfo faceInfo;
    InitFaceInfoScale(faceInfo, TEST_SCALE_MIN, TEST_SCALE_MIN, TEST_SCALE_MAX - TEST_SCALE_MIN,
        TEST_SCALE_MAX - TEST_SCALE_MIN);
    bool result = BackupDatabaseUtils::IsLandmarkValid(faceInfo, TEST_LANDMARK_BELOW, TEST_LANDMARK_BETWEEN);
    EXPECT_EQ(result, false);
    GTEST_LOG_(INFO) << "medialib_backup_test_is_landmark_valid_004 end";
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_is_landmark_valid_005, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_is_landmark_valid_005 start";
    FaceInfo faceInfo;
    InitFaceInfoScale(faceInfo, TEST_SCALE_MIN, TEST_SCALE_MIN, TEST_SCALE_MAX - TEST_SCALE_MIN,
        TEST_SCALE_MAX - TEST_SCALE_MIN);
    bool result = BackupDatabaseUtils::IsLandmarkValid(faceInfo, TEST_LANDMARK_ABOVE, TEST_LANDMARK_BETWEEN);
    EXPECT_EQ(result, false);
    GTEST_LOG_(INFO) << "medialib_backup_test_is_landmark_valid_005 end";
}

void ExecuteSqls(shared_ptr<RdbStore> rdbStore, const vector<string> &sqls)
{
    for (const auto &sql : sqls) {
        int32_t errCode = rdbStore->ExecuteSql(sql);
        if (errCode == E_OK) {
            continue;
        }
        MEDIA_ERR_LOG("Execute %{public}s failed: %{public}d", sql.c_str(), errCode);
    }
}

void ClearData(shared_ptr<RdbStore> rdbStore)
{
    MEDIA_INFO_LOG("Start clear data");
    ExecuteSqls(rdbStore, CLEAR_SQLS);
    MediaLibraryUnitTestUtils::InitUnistore();
    auto mediaLibraryRdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    MediaLibraryRdbUtils::UpdateAllAlbums(mediaLibraryRdbStore);
    MEDIA_INFO_LOG("End clear data");
}

void InsertPhoto(unique_ptr<UpgradeRestore> &upgrade, vector<FileInfo> &fileInfos)
{
    // Photo
    for (auto &fileInfo : fileInfos) {
        fileInfo.cloudPath = TEST_CLOUD_PATH_PREFIX + fileInfo.displayName;
        ValuesBucket values = upgrade->GetInsertValue(fileInfo, fileInfo.cloudPath, SourceType::GALLERY);
        int64_t rowNum = 0;
        if (upgrade->mediaLibraryRdb_->Insert(rowNum, PhotoColumn::PHOTOS_TABLE, values) != E_OK) {
            MEDIA_ERR_LOG("InsertSql failed, filePath = %{public}s", fileInfo.filePath.c_str());
        }
    }
    // Photo related
    upgrade->InsertPhotoRelated(fileInfos, SourceType::GALLERY);
}

void RestorePhotoWithPortrait(unique_ptr<UpgradeRestore> &upgrade)
{
    vector<FileInfo> fileInfos = upgrade->QueryFileInfos(0);
    InsertPhoto(upgrade, fileInfos);
    upgrade->UpdateFaceAnalysisStatus();
}

void QueryInt(shared_ptr<RdbStore> rdbStore, const string &querySql, const string &columnName,
    int32_t &result)
{
    ASSERT_NE(rdbStore, nullptr);
    auto resultSet = rdbStore->QuerySql(querySql);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    result = GetInt32Val(columnName, resultSet);
    MEDIA_INFO_LOG("Query %{public}s result: %{public}d", querySql.c_str(), result);
}

void QueryPortraitAlbumCount(shared_ptr<RdbStore> rdbStore, int32_t &result)
{
    string querySql = "SELECT count(DISTINCT group_tag) AS count FROM AnalysisAlbum WHERE album_type = 4096 AND \
        album_subtype = 4102";
    QueryInt(rdbStore, querySql, "count", result);
}

void QueryPortraitTagCount(shared_ptr<RdbStore> rdbStore, int32_t &result)
{
    string querySql = "SELECT count(1) as count FROM tab_analysis_face_tag";
    QueryInt(rdbStore, querySql, "count", result);
}

void QueryPortraitPhotoCount(shared_ptr<RdbStore> rdbStore, int32_t &result)
{
    string querySql = "SELECT count(DISTINCT file_id) as count FROM tab_analysis_image_face";
    QueryInt(rdbStore, querySql, "count", result);
}

void QueryPortraitFaceCount(shared_ptr<RdbStore> rdbStore, int32_t &result)
{
    string querySql = "SELECT count(1) as count FROM tab_analysis_image_face";
    QueryInt(rdbStore, querySql, "count", result);
}

void QueryPortraitTotalCount(shared_ptr<RdbStore> rdbStore, int32_t &result)
{
    string querySql = "SELECT count(1) as count FROM tab_analysis_total WHERE face > 0";
    QueryInt(rdbStore, querySql, "count", result);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_is_livephoto, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_is_livephoto start");
    FileInfo info;
    info.specialFileType = 50;
    EXPECT_EQ(BackupFileUtils::IsLivePhoto(info), true);
    info.specialFileType = 1050;
    EXPECT_EQ(BackupFileUtils::IsLivePhoto(info), true);
    info.specialFileType = 0;
    EXPECT_EQ(BackupFileUtils::IsLivePhoto(info), false);
    info.specialFileType = 1000;
    EXPECT_EQ(BackupFileUtils::IsLivePhoto(info), false);
    info.specialFileType = -50;
    EXPECT_EQ(BackupFileUtils::IsLivePhoto(info), false);
    MEDIA_INFO_LOG("medialib_backup_test_is_livephoto end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_convert_to_moving_photo, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_convert_to_moving_photo start");
    string livePhotoPath = "/data/test/backup_test_livephoto.jpg";
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/data/test/"), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(livePhotoPath), true);

    FileInfo info;
    info.filePath = livePhotoPath;
    BackupFileUtils::ConvertToMovingPhoto(info);
    EXPECT_EQ(info.movingPhotoVideoPath, "/data/test/backup_test_livephoto.jpg.mp4");
    EXPECT_EQ(info.extraDataPath, "/data/test/backup_test_livephoto.jpg.extra");

    info.filePath = livePhotoPath;
    EXPECT_EQ(MediaFileUtils::CreateFile(info.movingPhotoVideoPath), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(info.extraDataPath), true);
    BackupFileUtils::ConvertToMovingPhoto(info);
    EXPECT_EQ(info.movingPhotoVideoPath, "/data/test/backup_test_livephoto.jpg.mp4.dup.mp4");
    EXPECT_EQ(info.extraDataPath, "/data/test/backup_test_livephoto.jpg.extra.dup.extra");
    MEDIA_INFO_LOG("medialib_backup_test_convert_to_moving_photo end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_file_access_helper, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_file_access_helper start");
    string backupFilePath = "/data/test/Pictures/Camera/Flower.png";
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/data/test/Pictures/Camera/"), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(backupFilePath), true);

    string lowerCasePath = "/data/test/pictures/camera/flower.png";
    EXPECT_EQ(-1, access(lowerCasePath.c_str(), F_OK));

    std::shared_ptr<FileAccessHelper> fileAccessHelper_ = std::make_shared<FileAccessHelper>();
    string resultPath = lowerCasePath;
    bool res = fileAccessHelper_->GetValidPath(resultPath);
    EXPECT_EQ(true, res);
    EXPECT_EQ(backupFilePath, resultPath);

    resultPath = "/data/test/Pictures/Camera/FlowerNOT.png";
    res = fileAccessHelper_->GetValidPath(resultPath);
    EXPECT_EQ(false, res);

    MEDIA_INFO_LOG("medialib_backup_test_file_access_helper end");
}
 
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_media_type_beyong_1_3, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_media_type start");
    PhotosRestore photosRestore;
    FileInfo fileInfo;
    fileInfo.displayName = "abc.jpg";
    MimeTypeUtils::InitMimeTypeMap();
    EXPECT_EQ(photosRestore.FindMediaType(fileInfo), MediaType::MEDIA_TYPE_IMAGE);
    fileInfo.displayName = "abc.mp4";
    EXPECT_EQ(photosRestore.FindMediaType(fileInfo), MediaType::MEDIA_TYPE_VIDEO);
    fileInfo.fileType = 1;
    fileInfo.displayName = "abc.mp4";
    EXPECT_EQ(photosRestore.FindMediaType(fileInfo), MediaType::MEDIA_TYPE_IMAGE);
    fileInfo.fileType = 3;
    fileInfo.displayName = "abc.jpg";
    EXPECT_EQ(photosRestore.FindMediaType(fileInfo), MediaType::MEDIA_TYPE_VIDEO);
    MEDIA_INFO_LOG("medialib_backup_test_media_type end");
}

void TestAppTwinData(const string &path, const string &expectedExtraPrefix, int32_t sceneCode = UPGRADE_RESTORE_ID)
{
    string extraPrefix = BackupFileUtils::GetExtraPrefixForRealPath(sceneCode, path);
    MEDIA_INFO_LOG("path: %{public}s, extraPrefix: %{public}s", path.c_str(), extraPrefix.c_str());
    EXPECT_EQ(extraPrefix, expectedExtraPrefix);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_app_twin_data_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_001 start");
    TestAppTwinData("", "", CLONE_RESTORE_ID); // not upgrade
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_001 end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_app_twin_data_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_002 start");
    TestAppTwinData("", ""); // not app twin data: empty
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_002 end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_app_twin_data_003, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_003 start");
    TestAppTwinData("/storage/ABCE-EFGH/0/", ""); // not app twin data: external
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_003 end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_app_twin_data_004, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_004 start");
    TestAppTwinData("/storage/emulated/0/", ""); // not app twin data: main user
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_004 end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_app_twin_data_005, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_005 start");
    TestAppTwinData("/storage/emulated", ""); // not app twin data: first / not found
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_005 end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_app_twin_data_006, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_006 start");
    TestAppTwinData("/storage/emulated/", ""); // not app twin data: second / not found
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_006 end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_app_twin_data_007, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_007 start");
    TestAppTwinData("/storage/emulated/abc/", ""); // not app twin data: not number
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_007 end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_app_twin_data_008, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_008 start");
    TestAppTwinData("/storage/emulated/1234/", ""); // not app twin data: not in [128, 147]
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_008 end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_app_twin_data_009, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_009 start");
    TestAppTwinData("/storage/emulated/128/", APP_TWIN_DATA_PREFIX); // app twin data
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_009 end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GetFileFolderFromPath, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_GetFileFolderFromPath start");
    // test case 1 empty path
    string path;
    auto result = BackupFileUtils::GetFileFolderFromPath(path, true);
    EXPECT_EQ(result.empty(), true);

    // test case 2 : no '/' in path
    path = "GHY.txt";
    result = BackupFileUtils::GetFileFolderFromPath(path, true);
    EXPECT_EQ(result.empty(), true);

    // test case 3 : normal path
    path = "/data/test/GHY.txt";
    result = BackupFileUtils::GetFileFolderFromPath(path, true);
    EXPECT_EQ(result, "/data/test");

    // test case 4 : end > start
    path = "/GHY.txt";
    result = BackupFileUtils::GetFileFolderFromPath(path, true);
    EXPECT_EQ(result.empty(), true);
    MEDIA_INFO_LOG("medialib_backup_test_GetFileFolderFromPath end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_PreparePath, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_PreparePath start");
    // test case 1 empty path
    string path;
    auto result = BackupFileUtils::PreparePath(path);
    EXPECT_EQ(result, E_CHECK_DIR_FAIL);

    // test case 2 path end with '/'
    path = "GHY.txt/";
    result = BackupFileUtils::PreparePath(path);
    EXPECT_EQ(result, E_CHECK_DIR_FAIL);

    // normal dir and file
    path = "/data/test/GYH/test.txt";
    MediaFileUtils::CreateDirectory("/data/test/GYH/");
    MediaFileUtils::CreateFile("path");
    result = BackupFileUtils::PreparePath(path);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_PreparePath end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GarbleFilePath, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_GarbleFilePath start");
    string path = "/storage/test/GYH/test.txt";
    string clonePath;
    // test case 1 invalid sceneCode
    string result = BackupFileUtils::GarbleFilePath(path, -1, clonePath);
    EXPECT_EQ(result, "***/test/GYH/test.txt");

    // test case 2 sceneCode = OTHERS_PHONE_CLONE_RESTORE
    result = BackupFileUtils::GarbleFilePath(path, OTHERS_PHONE_CLONE_RESTORE, clonePath);
    EXPECT_EQ(result, "***/test.txt");
    MEDIA_INFO_LOG("medialib_backup_test_GarbleFilePath end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_CreatePath, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_CreatePath start");
    // invalid data type
    string displayName = "GHY.mp3";
    string path = "data/test/GYH";
    auto ret = BackupFileUtils::CreatePath(-1, displayName, path);
    EXPECT_EQ(ret, E_INVALID_FILEID);
    MEDIA_INFO_LOG("medialib_backup_test_CreatePath end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_IsLowQualityImage, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_IsLowQualityImage start");
    string filePath = "/data/test/GYH/GYH.mp3";
    int sceneCode = -1;
    string relativePath;
    bool hasLowQualityImage = false;
    auto ret = BackupFileUtils::IsLowQualityImage(filePath, sceneCode, relativePath, hasLowQualityImage);
    EXPECT_EQ(ret, E_FAIL);
    MEDIA_INFO_LOG("medialib_backup_test_IsLowQualityImage end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ConvertLowQualityPath, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_ConvertLowQualityPath start");
    int sceneCode = 0;
    string filePath;
    string relativePath;
    // test case 1 empty file path
    auto result = BackupFileUtils::ConvertLowQualityPath(sceneCode, filePath, relativePath);
    EXPECT_EQ(result.empty(), true);

    // test case 2 no '/' in path
    filePath = "GHY.txt";
    result = BackupFileUtils::ConvertLowQualityPath(sceneCode, filePath, relativePath);
    EXPECT_EQ(result, "GHY.txt");

    // test case 3 no dot in path and empty relativePath
    filePath = "/data/test/GYH/GYHmp3";
    result = BackupFileUtils::ConvertLowQualityPath(sceneCode, filePath, relativePath);
    EXPECT_EQ(result, "/Documents/cameradata/GYHmp3");

    // test case 4 normal path and empty relativePath
    filePath = "/data/test/GYH/GYH.mp3";
    result = BackupFileUtils::ConvertLowQualityPath(sceneCode, filePath, relativePath);
    EXPECT_EQ(result, "/Documents/cameradata/GYH.camera");

    // test case 5 normal relativePath
    filePath = "/test/GYH/GYH.mp3";
    relativePath = "/GYH";
    result = BackupFileUtils::ConvertLowQualityPath(sceneCode, filePath, relativePath);
    EXPECT_EQ(result, "/test/Documents/cameradata/GYH.camera");
    MEDIA_INFO_LOG("medialib_backup_test_ConvertLowQualityPath end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_restore_from_cloud_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_restore_from_cloud_test start");
    int32_t countBefore = restoreServiceCloud->totalNumber_;
    restoreServiceCloud->RestoreCloudFromGallery();
    EXPECT_EQ((restoreServiceCloud->totalNumber_ - countBefore), 0);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_query_cloud_infos_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_query_cloud_infos_test start");
    auto result = restoreService->QueryCloudFileInfos(0);
    EXPECT_EQ(result.empty(), true);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_restore_mode_test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_restore_mode_test_001 start");
    restoreService->restoreInfo_ = R"([{"type":"appTwinDataRestoreState", "detail":"2"}])";
    std::string restoreExInfo;
    restoreService->BaseRestore::StartRestoreEx("", "", restoreExInfo);
    EXPECT_EQ(restoreService->restoreMode_, 2);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_restore_mode_test_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_restore_mode_test_002 start");
    restoreService->restoreInfo_ = R"([{"type":"appTwinDataRestoreState", "detail":"5"}])";
    std::string restoreExInfo;
    restoreService->BaseRestore::StartRestoreEx("", "", restoreExInfo);
    EXPECT_EQ(restoreService->restoreMode_, 0);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_account_valid_test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_account_valid_test_001 start");
    restoreService->sceneCode_ = DUAL_FRAME_CLONE_RESTORE_ID;
    restoreService->restoreInfo_ = R"([{"type":"dualAccountId", "detail":"oldId"}])";
    (void)restoreService->BaseRestore::GetAccountValid();
    EXPECT_FALSE(restoreService->isAccountValid_);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_account_valid_test_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_account_valid_test_002 start");
    restoreService->sceneCode_ = DUAL_FRAME_CLONE_RESTORE_ID;
    restoreService->restoreInfo_ = R"([{"type":"test", "detail":"oldId"}])";
    (void)restoreService->BaseRestore::GetAccountValid();
    EXPECT_FALSE(restoreService->isAccountValid_);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_source_device_info_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_source_device_info_test start");
    restoreService->restoreInfo_ = R"([{"type":"dualDeviceSoftName", "detail":"0"}])";
    (void)restoreService->BaseRestore::GetSourceDeviceInfo();
    EXPECT_EQ(restoreService->dualDeviceSoftName_, "0");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_is_restore_photo_test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_is_restore_photo_test_001 start");
    restoreService->restoreInfo_ = R"([{"type":"backupInfo", "detail":"0"}])";
    bool restorePhoto = restoreService->BaseRestore::IsRestorePhoto();
    EXPECT_FALSE(restorePhoto);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_is_restore_photo_test_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_is_restore_photo_test_002 start");
    restoreService->restoreInfo_ = R"([{"type":"backupInfo", "detail":"galleryData"}])";
    bool restorePhoto = restoreService->BaseRestore::IsRestorePhoto();
    EXPECT_TRUE(restorePhoto);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_query_sql_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_query_sql_test start");
    std::string sql = "";
    std::vector<std::string> selectionArgs = {""};
    auto result = restoreService->BaseRestore::QuerySql(sql, selectionArgs);
    EXPECT_EQ(result, nullptr);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_insert_date_taken_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_insert_date_taken_test start");
    FileInfo fileInfo;
    int64_t dateModified = 1741351029532;
    fileInfo.dateModified = dateModified;
    NativeRdb::ValuesBucket values;
    restoreService->BaseRestore::SetValueFromMetaData(fileInfo, values);
    EXPECT_EQ(fileInfo.dateTaken, dateModified);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_cloud_insert_values_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_cloud_insert_values_test start");
    FileInfo fileInfo;
    fileInfo.displayName = TEST_FILE;
    std::vector<FileInfo> fileInfos = {fileInfo};
    auto result = restoreService->BaseRestore::GetCloudInsertValues(0, fileInfos, 0);
    EXPECT_EQ(result.size(), 0);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_insert_orientation_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_insert_orientation_test start");
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_ORIENTATION, 90);
    FileInfo fileInfo;
    fileInfo.fileType = MEDIA_TYPE_VIDEO;
    fileInfo.localMediaId = -1;
    restoreService->BaseRestore::SetValueFromMetaData(fileInfo, values);
    int32_t exifRotate;
    ValueObject valueObject;
    values.GetObject(PhotoColumn::PHOTO_EXIF_ROTATE, valueObject);
    valueObject.GetInt(exifRotate);
    EXPECT_EQ(exifRotate, 0);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_set_cover_position_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_set_cover_position_test start");
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    int32_t orientation = 180;
    data->SetOrientation(orientation);
    FileInfo fileInfo;
    fileInfo.specialFileType = LIVE_PHOTO_TYPE;
    NativeRdb::ValuesBucket values;
    restoreService->BaseRestore::SetValueFromMetaData(fileInfo, values);
    int32_t coverPosition;
    ValueObject valueObject;
    values.GetObject(PhotoColumn::PHOTO_COVER_POSITION, valueObject);
    valueObject.GetInt(coverPosition);
    EXPECT_EQ(coverPosition, 0);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_move_migrate_file_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_move_migrate_file_test start");
    FileInfo fileInfo;
    fileInfo.needMove = true;
    fileInfo.filePath = TEST_DIR_PATH;
    fileInfo.specialFileType = LIVE_PHOTO_TYPE;
    std::vector<FileInfo> fileInfos = {fileInfo};
    int32_t fileMoveCount = 0;
    int32_t videoFileMoveCount = 0;
    int32_t sceneCode = 0;
    restoreService->BaseRestore::MoveMigrateFile(fileInfos, fileMoveCount, videoFileMoveCount, sceneCode);
    EXPECT_FALSE(fileInfos[0].needVisible);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_move_migrate_cloud_file_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_move_migrate_cloud_file_test start");
    FileInfo fileInfo;
    fileInfo.needMove = true;
    fileInfo.filePath = TEST_DIR_PATH;
    fileInfo.specialFileType = LIVE_PHOTO_TYPE;
    std::vector<FileInfo> fileInfos = {fileInfo};
    int32_t fileMoveCount = 0;
    int32_t videoFileMoveCount = 0;
    int32_t sceneCode = 0;
    restoreService->BaseRestore::MoveMigrateCloudFile(fileInfos, fileMoveCount, videoFileMoveCount, sceneCode);
    EXPECT_EQ(restoreService->migrateFileNumber_, 0);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_batch_create_dentry_file_test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_batch_create_dentry_file_test_001 start");
    FileInfo fileInfo;
    fileInfo.needMove = true;
    std::vector<FileInfo> fileInfos = {fileInfo};
    std::vector<std::string> failCloudIds;
    std::string fileType = DENTRY_INFO_ORIGIN;
    int32_t ret = restoreService->BaseRestore::BatchCreateDentryFile(fileInfos, failCloudIds, fileType);
    EXPECT_NE(ret, E_OK);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_batch_create_dentry_file_test_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_batch_create_dentry_file_test_002 start");
    FileInfo fileInfo;
    fileInfo.needMove = true;
    std::vector<FileInfo> fileInfos = {fileInfo};
    std::vector<std::string> failCloudIds;
    std::string fileType = DENTRY_INFO_LCD;
    int32_t ret = restoreService->BaseRestore::BatchCreateDentryFile(fileInfos, failCloudIds, fileType);
    EXPECT_NE(ret, E_OK);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_batch_create_dentry_file_test_003, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_batch_create_dentry_file_test_003 start");
    FileInfo fileInfo;
    fileInfo.needMove = true;
    std::vector<FileInfo> fileInfos = {fileInfo};
    std::vector<std::string> failCloudIds;
    std::string fileType = DENTRY_INFO_THM;
    int32_t ret = restoreService->BaseRestore::BatchCreateDentryFile(fileInfos, failCloudIds, fileType);
    EXPECT_NE(ret, E_OK);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_restore_lcd_and_thumb_from_cloud_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_restore_lcd_and_thumb_from_cloud_test start");
    FileInfo fileInfo;
    int32_t type = TEST_MIGRATE_CLOUD_LCD_TYPE;
    bool ret = restoreService->BaseRestore::RestoreLcdAndThumbFromCloud(fileInfo, type, 0);
    EXPECT_EQ(ret, false);

    fileInfo.localBigThumbPath = TEST_DIR_PATH;
    ret = restoreService->BaseRestore::RestoreLcdAndThumbFromCloud(fileInfo, type, 0);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_handle_fail_data_test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_handle_fail_data_test_001 start");
    FileInfo fileInfoOrigin;
    fileInfoOrigin.uniqueId = TEST_TRUE_UNIQUEID;
    std::vector<FileInfo> fileInfos = {fileInfoOrigin};
    std::vector<std::string> failCloudIds = {TEST_TRUE_UNIQUEID};
    std::string fileType = DENTRY_INFO_ORIGIN;
    restoreService->BaseRestore::HandleFailData(fileInfos, failCloudIds, fileType);
    EXPECT_EQ(fileInfos.size(), 0);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_handle_fail_data_test_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_handle_fail_data_test_002 start");
    FileInfo fileInfoLcd;
    fileInfoLcd.uniqueId = TEST_TRUE_UNIQUEID;
    std::vector<FileInfo> fileInfos = {fileInfoLcd};
    std::vector<std::string> failCloudIds = {TEST_TRUE_UNIQUEID};
    std::string fileType = DENTRY_INFO_LCD;
    restoreService->BaseRestore::HandleFailData(fileInfos, failCloudIds, fileType);
    EXPECT_EQ(fileInfos.size(), 1);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_handle_fail_data_test_003, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_handle_fail_data_test_003 start");
    FileInfo fileInfoThm;
    fileInfoThm.uniqueId = TEST_TRUE_UNIQUEID;
    std::vector<FileInfo> fileInfos = {fileInfoThm};
    std::vector<std::string> failCloudIds = {TEST_TRUE_UNIQUEID};
    std::string fileType = DENTRY_INFO_THM;
    restoreService->BaseRestore::HandleFailData(fileInfos, failCloudIds, fileType);
    EXPECT_EQ(fileInfos.size(), 1);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_handle_fail_data_test_004, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_handle_fail_data_test_004 start");
    FileInfo fileInfoNotFail;
    fileInfoNotFail.uniqueId = TEST_FALSE_UNIQUEID;
    std::vector<FileInfo> fileInfos = {fileInfoNotFail};
    std::vector<std::string> failCloudIds = {TEST_TRUE_UNIQUEID};
    std::string fileType = DENTRY_INFO_THM;
    restoreService->BaseRestore::HandleFailData(fileInfos, failCloudIds, fileType);
    EXPECT_EQ(fileInfos.size(), 1);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_set_visible_photo_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_set_visible_photo_test start");
    FileInfo fileInfo;
    fileInfo.needVisible = true;
    fileInfo.needMove = true;
    std::vector<FileInfo> fileInfos = {fileInfo};
    bool ret = restoreService->BaseRestore::SetVisiblePhoto(fileInfos);
    EXPECT_EQ(fileInfos.size(), 1);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_batch_insert_map_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_batch_insert_map_test start");
    FileInfo fileInfo1;
    fileInfo1.packageName = "0";
    FileInfo fileInfo2;
    std::vector<FileInfo> fileInfos = {fileInfo1, fileInfo2};
    int64_t totalNum = 0;
    restoreService->BaseRestore::BatchInsertMap(fileInfos, totalNum);
    EXPECT_EQ(totalNum, 0);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_unique_id_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_unique_id_test start");
    int32_t fileType = MediaType::MEDIA_TYPE_IMAGE;
    int32_t uniqueId = restoreService->BaseRestore::GetUniqueId(fileType);
    EXPECT_EQ(uniqueId, restoreService->imageNumber_ - 1);

    fileType = MediaType::MEDIA_TYPE_VIDEO;
    uniqueId = restoreService->BaseRestore::GetUniqueId(fileType);
    EXPECT_EQ(uniqueId, restoreService->videoNumber_ - 1);

    fileType = MediaType::MEDIA_TYPE_AUDIO;
    uniqueId = restoreService->BaseRestore::GetUniqueId(fileType);
    EXPECT_EQ(uniqueId, restoreService->audioNumber_ - 1);

    fileType = -1;
    uniqueId = restoreService->BaseRestore::GetUniqueId(fileType);
    EXPECT_EQ(uniqueId, - 1);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_progress_info_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_progress_info_test start");
    std::string progressInfo = restoreService->BaseRestore::GetProgressInfo();
    EXPECT_FALSE(progressInfo.empty());
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_update_database_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_update_database_test start");
    restoreService->BaseRestore::UpdateDatabase();
    EXPECT_EQ(restoreService->updateProcessStatus_, ProcessStatus::STOP);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_extra_check_for_clone_same_file_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_extra_check_for_clone_same_file_test start");
    PhotosDao::PhotosRowData rowData;
    FileInfo fileInfo;
    rowData.cleanFlag = 1;
    rowData.position = static_cast<int32_t>(PhotoPositionType::CLOUD);
    rowData.fileId = 1;
    bool ret = restoreService->BaseRestore::ExtraCheckForCloneSameFile(fileInfo, rowData);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_insert_value_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_insert_value_test start");
    FileInfo fileInfo;
    std::string newPath;
    fileInfo.firstUpdateTime = 1;
    fileInfo.dateTaken = 1;
    fileInfo.isFavorite = 1;
    fileInfo.hidden = 1;
    fileInfo.packageName = TEST_DIR_PATH;
    fileInfo.localMediaId = TEST_FALSE_MEDIAID;
    auto values = restoreService->GetInsertValue(fileInfo, newPath, 0);
    std::string packageName;
    ValueObject valueObject;
    values.GetObject(PhotoColumn::MEDIA_PACKAGE_NAME, valueObject);
    valueObject.GetString(packageName);
    EXPECT_EQ(packageName, TEST_DIR_PATH);

    fileInfo.firstUpdateTime = 0;
    values = restoreService->GetInsertValue(fileInfo, newPath, 0);
    int32_t dateAdded;
    values.GetObject(PhotoColumn::MEDIA_DATE_ADDED, valueObject);
    valueObject.GetInt(dateAdded);
    EXPECT_EQ(dateAdded, 1);
}

HWTEST_F(MediaLibraryBackupTest, medialibrary_backup_test_geo_knowledge_test1, TestSize.Level0)
{
    MEDIA_INFO_LOG("medialibrary_backup_test_geo_knowledge_test1 start");
    std::unique_ptr<UpgradeRestore> restoreService = std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME,
        UPGRADE_RESTORE_ID);
    (void)restoreService->Init(TEST_BACKUP_PATH, TEST_UPGRADE_FILE_DIR, false);

    NativeRdb::ValuesBucket valuesBucket;
    EXPECT_EQ(valuesBucket.HasColumn("address_description"), false);
    EXPECT_EQ(valuesBucket.HasColumn("language"), false);
    std::vector<GeoKnowledgeRestore::GeoKnowledgeInfo> albumInfo;
    GeoKnowledgeRestore::GeoKnowledgeInfo info;
    info.adminArea = "test";
    info.locality = "test";
    info.language = "zh";
    albumInfo.push_back(info);
    valuesBucket = restoreService->geoKnowledgeRestore_.GetMapInsertValue(albumInfo.begin(), 0);
    EXPECT_EQ(valuesBucket.HasColumn("address_description"), true);
    EXPECT_EQ(valuesBucket.HasColumn("language"), true);
    MEDIA_INFO_LOG("medialibrary_backup_test_geo_knowledge_test1 end");
}

HWTEST_F(MediaLibraryBackupTest, medialibrary_backup_test_geo_knowledge_test2, TestSize.Level0)
{
    MEDIA_INFO_LOG("medialibrary_backup_test_geo_knowledge_test2 start");
    std::unique_ptr<UpgradeRestore> restoreService = std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME,
        UPGRADE_RESTORE_ID);
    (void)restoreService->Init(TEST_BACKUP_PATH, TEST_UPGRADE_FILE_DIR, false);

    NativeRdb::ValuesBucket valuesBucket;
    EXPECT_EQ(valuesBucket.HasColumn("address_description"), false);
    EXPECT_EQ(valuesBucket.HasColumn("language"), false);
    std::vector<GeoKnowledgeRestore::GeoKnowledgeInfo> albumInfo;
    GeoKnowledgeRestore::GeoKnowledgeInfo info;
    info.adminArea = "test";
    info.locality = "test123";
    info.language = "en";
    albumInfo.push_back(info);
    valuesBucket = restoreService->geoKnowledgeRestore_.GetMapInsertValue(albumInfo.begin(), 0);
    EXPECT_EQ(valuesBucket.HasColumn("address_description"), true);
    EXPECT_EQ(valuesBucket.HasColumn("language"), true);
    MEDIA_INFO_LOG("medialibrary_backup_test_geo_knowledge_test2 end");
}

HWTEST_F(MediaLibraryBackupTest, medialibrary_backup_test_geo_knowledge_test3, TestSize.Level0)
{
    MEDIA_INFO_LOG("medialibrary_backup_test_geo_knowledge_test3 start");
    std::unique_ptr<UpgradeRestore> restoreService = std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME,
        UPGRADE_RESTORE_ID);
    (void)restoreService->Init(TEST_BACKUP_PATH, TEST_UPGRADE_FILE_DIR, false);
    restoreService->geoKnowledgeRestore_.mediaLibraryRdb_ = nullptr;
    restoreService->geoKnowledgeRestore_.batchCnt_ = 0xFF;

    std::unordered_map<int32_t, PhotoInfo> photoInfoMap;
    restoreService->geoKnowledgeRestore_.RestoreMaps(photoInfoMap);
    EXPECT_EQ(restoreService->geoKnowledgeRestore_.batchCnt_, 0);
    MEDIA_INFO_LOG("medialibrary_backup_test_geo_knowledge_test3 end");
}

HWTEST_F(MediaLibraryBackupTest, medialibrary_backup_test_geo_knowledge_test4, TestSize.Level0)
{
    MEDIA_INFO_LOG("medialibrary_backup_test_geo_knowledge_test4 start");
    std::unique_ptr<UpgradeRestore> restoreService = std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME,
        UPGRADE_RESTORE_ID);
    (void)restoreService->Init(TEST_BACKUP_PATH, TEST_UPGRADE_FILE_DIR, false);
    restoreService->geoKnowledgeRestore_.mediaLibraryRdb_ = photosStorePtr;
    restoreService->geoKnowledgeRestore_.batchCnt_ = 0xFF;

    std::unordered_map<int32_t, PhotoInfo> photoInfoMap;
    restoreService->geoKnowledgeRestore_.RestoreMaps(photoInfoMap);
    EXPECT_EQ(restoreService->geoKnowledgeRestore_.batchCnt_, 0);
    MEDIA_INFO_LOG("medialibrary_backup_test_geo_knowledge_test4 end");
}

HWTEST_F(MediaLibraryBackupTest, medialibrary_backup_test_geo_knowledge_test5, TestSize.Level0)
{
    MEDIA_INFO_LOG("medialibrary_backup_test_geo_knowledge_test5 start");
    std::unique_ptr<UpgradeRestore> restoreService = std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME,
        UPGRADE_RESTORE_ID);
    (void)restoreService->Init(TEST_BACKUP_PATH, TEST_UPGRADE_FILE_DIR, false);
    restoreService->geoKnowledgeRestore_.mediaLibraryRdb_ = photosStorePtr;
    restoreService->geoKnowledgeRestore_.batchCnt_ = 0;

    constexpr double DOUBLE_EPSILON = 1e-15;
    int64_t rowNum = 0;
    GeoKnowledgeRestore::GeoMapInfo geoMapInfo1;
    std::vector<NativeRdb::ValuesBucket> values1;
    geoMapInfo1.photoInfo.fileIdNew = 0;
    restoreService->geoKnowledgeRestore_.UpdateMapInsertValues(values1, geoMapInfo1);
    restoreService->geoKnowledgeRestore_.BatchInsertWithRetry("tab_analysis_geo_knowledge", values1, rowNum);
    GeoKnowledgeRestore::GeoMapInfo geoMapInfo2;
    std::vector<NativeRdb::ValuesBucket> values2;
    geoMapInfo2.photoInfo.fileIdNew = 1;
    geoMapInfo2.latitude = DOUBLE_EPSILON + 1.0;
    geoMapInfo2.longitude = DOUBLE_EPSILON + 1.0;
    GeoKnowledgeRestore::GeoKnowledgeInfo albumInfo;
    albumInfo.language = "zh";
    albumInfo.latitude = DOUBLE_EPSILON + 1.0;
    albumInfo.longitude = DOUBLE_EPSILON + 1.0;
    restoreService->geoKnowledgeRestore_.albumInfos_.emplace_back(albumInfo);
    restoreService->geoKnowledgeRestore_.UpdateMapInsertValues(values2, geoMapInfo2);
    restoreService->geoKnowledgeRestore_.BatchInsertWithRetry("tab_analysis_geo_knowledge", values2, rowNum);
    EXPECT_EQ(restoreService->geoKnowledgeRestore_.batchCnt_, 1);
    MEDIA_INFO_LOG("medialibrary_backup_test_geo_knowledge_test5 end");
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_data_longitude_test1, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_data_longitude_test1 start");
    restoreService->sceneCode_ = I_PHONE_CLONE_RESTORE;
    double setLongitude = 2.0;
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    data->SetLongitude(setLongitude);
    FileInfo fileInfo;
    fileInfo.longitude = 1.0;
    double ret = restoreService->BaseRestore::GetDataLongitude(fileInfo, data);
    EXPECT_EQ(ret, fileInfo.longitude);

    fileInfo.longitude = 0.0;
    ret = restoreService->BaseRestore::GetDataLongitude(fileInfo, data);
    EXPECT_EQ(ret, setLongitude);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_data_longitude_test2, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_data_longitude_test2 start");
    restoreService->sceneCode_ = UPGRADE_RESTORE_ID;
    double setLongitude = 0.0;
    double setLatitude = 0.0;
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    FileInfo fileInfo;
    fileInfo.longitude = 1.0;
    double ret = restoreService->BaseRestore::GetDataLongitude(fileInfo, data);
    EXPECT_EQ(ret, fileInfo.longitude);

    setLongitude = 2.0;
    setLatitude = 0.0;
    data->SetLongitude(setLongitude);
    data->SetLatitude(setLatitude);
    ret = restoreService->BaseRestore::GetDataLongitude(fileInfo, data);
    EXPECT_EQ(ret, setLongitude);

    setLongitude = 0.0;
    setLatitude = 2.0;
    data->SetLongitude(setLongitude);
    data->SetLatitude(setLatitude);
    ret = restoreService->BaseRestore::GetDataLongitude(fileInfo, data);
    EXPECT_EQ(ret, setLongitude);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_data_longitude_test3, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_data_longitude_test3 start");
    restoreService->sceneCode_ = DUAL_FRAME_CLONE_RESTORE_ID;
    double setLongitude = 0.0;
    double setLatitude = 0.0;
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    FileInfo fileInfo;
    fileInfo.longitude = 1.0;
    double ret = restoreService->BaseRestore::GetDataLongitude(fileInfo, data);
    EXPECT_EQ(ret, fileInfo.longitude);

    setLongitude = 2.0;
    setLatitude = 0.0;
    data->SetLongitude(setLongitude);
    data->SetLatitude(setLatitude);
    ret = restoreService->BaseRestore::GetDataLongitude(fileInfo, data);
    EXPECT_EQ(ret, setLongitude);

    setLongitude = 0.0;
    setLatitude = 2.0;
    data->SetLongitude(setLongitude);
    data->SetLatitude(setLatitude);
    ret = restoreService->BaseRestore::GetDataLongitude(fileInfo, data);
    EXPECT_EQ(ret, setLongitude);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_data_latitude_test1, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_data_latitude_test1 start");
    restoreService->sceneCode_ = I_PHONE_CLONE_RESTORE;
    double setLatitude = 2.0;
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    data->SetLatitude(setLatitude);
    FileInfo fileInfo;
    fileInfo.latitude= 1.0;
    double ret = restoreService->BaseRestore::GetDataLatitude(fileInfo, data);
    EXPECT_EQ(ret, fileInfo.latitude);

    fileInfo.latitude = 0.0;
    ret = restoreService->BaseRestore::GetDataLatitude(fileInfo, data);
    EXPECT_EQ(ret, setLatitude);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_data_latitude_test2, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_data_latitude_test2 start");
    restoreService->sceneCode_ = UPGRADE_RESTORE_ID;
    double setLongitude = 0.0;
    double setLatitude = 0.0;
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    FileInfo fileInfo;
    fileInfo.latitude = 1.0;
    double ret = restoreService->BaseRestore::GetDataLatitude(fileInfo, data);
    EXPECT_EQ(ret, fileInfo.latitude);

    setLongitude = 2.0;
    setLatitude = 0.0;
    data->SetLongitude(setLongitude);
    data->SetLatitude(setLatitude);
    ret = restoreService->BaseRestore::GetDataLatitude(fileInfo, data);
    EXPECT_EQ(ret, setLatitude);

    setLongitude = 0.0;
    setLatitude = 2.0;
    data->SetLongitude(setLongitude);
    data->SetLatitude(setLatitude);
    ret = restoreService->BaseRestore::GetDataLatitude(fileInfo, data);
    EXPECT_EQ(ret, setLatitude);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_data_latitude_test3, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_data_latitude_test3 start");
    restoreService->sceneCode_ = DUAL_FRAME_CLONE_RESTORE_ID;
    double setLongitude = 0.0;
    double setLatitude = 0.0;
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    FileInfo fileInfo;
    fileInfo.latitude = 1.0;
    double ret = restoreService->BaseRestore::GetDataLatitude(fileInfo, data);
    EXPECT_EQ(ret, fileInfo.latitude);

    setLongitude = 2.0;
    setLatitude = 0.0;
    data->SetLongitude(setLongitude);
    data->SetLatitude(setLatitude);
    ret = restoreService->BaseRestore::GetDataLatitude(fileInfo, data);
    EXPECT_EQ(ret, setLatitude);

    setLongitude = 0.0;
    setLatitude = 2.0;
    data->SetLongitude(setLongitude);
    data->SetLatitude(setLatitude);
    ret = restoreService->BaseRestore::GetDataLatitude(fileInfo, data);
    EXPECT_EQ(ret, setLatitude);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_current_device_restore_config_info_test001, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_current_device_restore_config_info_test001 start");

    RestoreConfigInfo config = restoreService->GetCurrentDeviceRestoreConfigInfo();
    bool flag = config.photoPositionType == PhotoPositionType::CLOUD ||
        config.southDeviceType == SouthDeviceType::SOUTH_DEVICE_NULL;
    EXPECT_TRUE(flag);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_cloud_query_sql_test001, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_cloud_query_sql_test001 start");

    restoreService->restoreConfig_.restoreSwitchType = SwitchStatus::CLOUD;
    std::string result = restoreService->GetCloudQuerySql();
    std::string expected = "SELECT _id FROM ("
                           "SELECT _id, ROW_NUMBER() OVER (ORDER BY _id ASC) AS row_num FROM gallery_media "
                           "WHERE (local_media_id == -1) AND COALESCE(uniqueId,'') <> '' "
                           "AND (relative_bucket_id IS NULL OR relative_bucket_id NOT IN ("
                           "SELECT DISTINCT relative_bucket_id FROM garbage_album WHERE type = 1)) "
                           "AND (_size > 0 OR (1 = ? AND _size = 0 AND photo_quality = 0)) "
                           "AND _data NOT LIKE '/storage/emulated/0/Pictures/cloud/Imports%' "
                           "AND COALESCE(_data, '') <> '' "
                           "AND (1 = ? OR COALESCE(storage_id, 0) IN (0, 65537))) AS numbered "
                           "WHERE (row_num - 1) % 200 = 0 ;";
    bool flag = result == expected;
    EXPECT_TRUE(flag);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_cloud_query_sql_test002, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_cloud_query_sql_test002 start");

    restoreService->restoreConfig_.restoreSwitchType = SwitchStatus::HDC;
    std::string result = restoreService->GetCloudQuerySql();
    std::string expected = "SELECT _id FROM ("
                           "SELECT _id, ROW_NUMBER() OVER (ORDER BY _id ASC) AS row_num FROM gallery_media "
                           "WHERE (local_media_id == -1) AND COALESCE(uniqueId,'') = '' "
                           "AND COALESCE(hdc_unique_id,'') <> '' "
                           "AND (relative_bucket_id IS NULL OR relative_bucket_id NOT IN ("
                           "SELECT DISTINCT relative_bucket_id FROM garbage_album WHERE type = 1)) "
                           "AND (_size > 0 OR (1 = ? AND _size = 0 AND photo_quality = 0)) "
                           "AND _data NOT LIKE '/storage/emulated/0/Pictures/cloud/Imports%' "
                           "AND COALESCE(_data, '') <> '' "
                           "AND (1 = ? OR COALESCE(storage_id, 0) IN (0, 65537))) AS numbered "
                           "WHERE (row_num - 1) % 200 = 0 ;";
    bool flag = result == expected;
    EXPECT_TRUE(flag);
}

HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_cloud_query_sql_test003, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_cloud_query_sql_test003 start");

    restoreService->restoreConfig_.restoreSwitchType = SwitchStatus::NONE;
    std::string result = restoreService->GetCloudQuerySql();
    std::string expected = "";
    bool flag = result == expected;
    EXPECT_TRUE(flag);
}
} // namespace Media
} // namespace OHOS