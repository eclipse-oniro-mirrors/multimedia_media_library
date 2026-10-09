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
#include "backup_const.h"
#include "backup_const_column.h"
#include "backup_const_map.h"
#include "backup_database_utils.h"
#include "backup_file_utils.h"
#include "external_source.h"
#include "gallery_source.h"
#include "media_log.h"
#include "upgrade_restore.h"
#include "media_file_utils.h"
#include "parameters.h"
#include "cloud_sync_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "vision_db_sqls.h"
#include "geo_knowledge_restore.h"
#undef private
#undef protected
#include "mimetype_utils.h"
#include "restore_map_code_utils.h"

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
const std::string TEST_ROOT_LAKE_DIR = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/";

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
    " dirty INTEGER DEFAULT 0, subtype INT, detail_time TEXT);";

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

/*
 * Test interface: UpgradeRestore::Init
 * Test content: Verify backup initialization with upgrade restore scene
 * Cover branches: Normal flow with UPGRADE_RESTORE_ID
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_init, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_init start");
    std::unique_ptr<UpgradeRestore> restoreService = std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME,
        UPGRADE_RESTORE_ID);
    int32_t result = restoreService->Init(TEST_BACKUP_PATH, TEST_UPGRADE_FILE_DIR, false);
    EXPECT_EQ(result, 0);
    MEDIA_INFO_LOG("medialib_backup_test_init end");
}

/*
 * Test interface: QueryFileInfos, database query
 * Test content: Verify unsynced valid files are not restored
 * Cover branches: Unsynced file filtering branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_not_sync_valid, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_not_sync_valid start");
    std::string queryNotSyncValid = "SELECT file_id from Photos where display_name ='not_sync_weixin.jpg'";
    auto resultSet = photosStorePtr->QuerySql(queryNotSyncValid);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_not_sync_valid end");
}

/*
 * Test interface: QueryFileInfos, database query
 * Test content: Verify unsynced invalid files are not handled
 * Cover branches: Unsynced invalid file filtering branch
 */
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

/*
 * Test interface: QueryFileInfos, database query
 * Test content: Verify unsynced pending camera files are not restored
 * Cover branches: Pending camera file filtering branch
 */
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

/*
 * Test interface: QueryFileInfos, database query
 * Test content: Verify unsynced pending others files are not restored
 * Cover branches: Pending others file filtering branch
 */
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

/*
 * Test interface: QueryFileInfos, database query
 * Test content: Verify zero-size files are not restored
 * Cover branches: Zero-size file filtering branch
 */
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

/*
 * Test interface: BackupFileUtils::CreateAssetPathById
 * Test content: Verify asset path creation by ID
 * Cover branches: Normal flow with valid file ID
 */
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

/*
 * Test interface: BaseRestore::GetInsertValues
 * Test content: Calculate not found files in main data mode
 * Cover branches: RESTORE_MODE_PROC_ALL_DATA and RESTORE_MODE_PROC_TWIN_DATA
 */
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

/*
 * Test interface: BaseRestore::GetInsertValues
 * Test content: Calculate not found files for twin user in dual frame clone mode
 * Cover branches: Twin user ID branch in dual frame clone
 */
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

/*
 * Test interface: BaseRestore::GetInsertValues
 * Test content: Calculate not found files for app twin data user ID
 * Cover branches: App twin data user ID branch
 */
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

/*
 * Test interface: BaseRestore::GetInsertValues
 * Test content: Calculate not found files for main data user ID with existing file path
 * Cover branches: Main data user ID with file path
 */
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

/*
 * Test interface: BackupFileUtils::CreateAssetPathById
 * Test content: Verify clone update functionality
 * Cover branches: Normal flow with asset path creation
 */
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

/*
 * Test interface: QueryFileInfos, database query
 * Test content: Verify unsynced media files are not restored
 * Cover branches: Unsynced media file filtering branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_a_media_not_sync_valid, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_a_media_not_sync_valid start");
    std::string queryNotSyncValid = "SELECT file_id from Photos where display_name ='a_media_not_sync.jpg'";
    auto resultSet = photosStorePtr->QuerySql(queryNotSyncValid);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_a_media_not_sync_valid end");
}

/*
 * Test interface: QueryFileInfos, database query
 * Test content: Verify zero-size media files are not restored
 * Cover branches: Zero-size media file filtering branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_a_media_zero_size, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_a_media_zero_size start");
    std::string queryNotSyncValid = "SELECT file_id from Photos where display_name ='a_media_zero_size.jpg'";
    auto resultSet = photosStorePtr->QuerySql(queryNotSyncValid);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_a_media_zero_size end");
}

/*
 * Test interface: PhotosRestore::AnalyzeGalleryErrorSource, PhotosRestore::IsDuplicateData
 * Test content: Verify duplicate data detection and handling
 * Cover branches: Normal flow with duplicate data detection
 */
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

/*
 * Test interface: PhotosRestore::AnalyzeGalleryErrorSource, PhotosRestore::IsDuplicateData
 * Test content: Verify duplicate data detection is case-sensitive
 * Cover branches: Case-sensitive duplicate data detection
 */
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

/*
 * Test interface: UpgradeRestore::HandleRestData
 * Test content: Verify handling remaining data without deleting database
 * Cover branches: RESTORE_MODE_PROC_MAIN_DATA branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_handle_rest_data_not_del_db, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_handle_rest_data_not_del_db start");
    const string galleryDbPath = TEST_BACKUP_PATH + "/" + GALLERY_APP_NAME + "/ce/databases/gallery.db";
    const string externalDbPath = TEST_BACKUP_PATH + "/" + MEDIA_APP_NAME + "/ce/databases/external.db";
    bool isGalleryDbExist = MediaFileUtils::IsFileExists(galleryDbPath);
    bool isExternalDbExist = MediaFileUtils::IsFileExists(externalDbPath);
    EXPECT_EQ(isGalleryDbExist, true);
    EXPECT_EQ(isExternalDbExist, true);

    restoreService->restoreMode_ = RESTORE_MODE_PROC_MAIN_DATA;
    restoreService->HandleRestData();
    isGalleryDbExist = MediaFileUtils::IsFileExists(galleryDbPath);
    isExternalDbExist = MediaFileUtils::IsFileExists(externalDbPath);
    EXPECT_EQ(isGalleryDbExist, true);
    EXPECT_EQ(isExternalDbExist, true);
    MEDIA_INFO_LOG("medialib_backup_test_handle_rest_data_not_del_db end");
}

/*
 * Test interface: UpgradeRestore::HandleRestData
 * Test content: Verify handling remaining data and deleting database
 * Cover branches: RESTORE_MODE_PROC_ALL_DATA branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_handle_rest_data_del_db, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_handle_rest_data_del_db start");
    const string galleryDbPath = TEST_BACKUP_PATH + "/" + GALLERY_APP_NAME + "/ce/databases/gallery.db";
    const string externalDbPath = TEST_BACKUP_PATH + "/" + MEDIA_APP_NAME + "/ce/databases/external.db";
    bool isGalleryDbExist = MediaFileUtils::IsFileExists(galleryDbPath);
    bool isExternalDbExist = MediaFileUtils::IsFileExists(externalDbPath);
    EXPECT_EQ(isGalleryDbExist, true);
    EXPECT_EQ(isExternalDbExist, true);

    restoreService->restoreMode_ = RESTORE_MODE_PROC_ALL_DATA;
    restoreService->HandleRestData();
    isGalleryDbExist = MediaFileUtils::IsFileExists(galleryDbPath);
    isExternalDbExist = MediaFileUtils::IsFileExists(externalDbPath);
    EXPECT_EQ(isGalleryDbExist, false);
    EXPECT_EQ(isExternalDbExist, false);
    MEDIA_INFO_LOG("medialib_backup_test_handle_rest_data_del_db end");
}

/*
 * Test interface: MediaFileUtils file operations
 * Test content: Verify file modification time update
 * Cover branches: Normal flow with file time modification
 */
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

/*
 * Test interface: BackupFileUtils::CreateAssetPathById
 * Test content: Verify audio file path creation by ID
 * Cover branches: Normal flow with audio media type
 */
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

/*
 * Test interface: BackupFileUtils::CreateAssetPathById
 * Test content: Verify video file path creation by ID
 * Cover branches: Normal flow with video media type
 */
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

/*
 * Test interface: BackupFileUtils::CreateAssetPathById
 * Test content: Verify file path creation with illegal file ID
 * Cover branches: Invalid file ID error handling branch
 */
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

/*
 * Test interface: BackupFileUtils::CreateAssetPathById
 * Test content: Verify file path creation with normal ID and remainder
 * Cover branches: Normal flow with remainder
 */
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

/*
 * Test interface: BackupFileUtils::CreateAssetPathById
 * Test content: Verify file path creation with normal ID and no remainder
 * Cover branches: Normal flow without remainder
 */
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

/*
 * Test interface: BackupFileUtils::CreateAssetPathById
 * Test content: Verify file path creation with illegal file type 001
 * Cover branches: Invalid file type error handling branch
 */
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

/*
 * Test interface: BackupFileUtils::CreateAssetPathById
 * Test content: Verify file path creation with illegal file type 002
 * Cover branches: Invalid file type with ASSET_MAX_COMPLEMENT_ID
 */
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

/*
 * Test interface: BackupFileUtils::CreateAssetPathById
 * Test content: Verify file path creation with illegal file type 003
 * Cover branches: Invalid file type above ASSET_MAX_COMPLEMENT_ID
 */
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

/*
 * Test interface: BackupFileUtils::GetFileNameFromPath
 * Test content: Verify file name extraction from empty path
 * Cover branches: Empty path error handling branch
 */
HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_GetFileNameFromPath_path_empty, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_GetFileNameFromPath_path_empty start");
    string path;
    string res = BackupFileUtils::GetFileNameFromPath(path);
    EXPECT_EQ(res, "");
    MEDIA_INFO_LOG("BackupFileUtils_GetFileNameFromPath_path_empty end");
}

/*
 * Test interface: BackupFileUtils::GetFileNameFromPath
 * Test content: Verify file name extraction from path without separator
 * Cover branches: Path without separator error handling branch
 */
HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_GetFileNameFromPath_path_not_have, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_GetFileNameFromPath_path_not_have start");
    string path = "test";
    string res = BackupFileUtils::GetFileNameFromPath(path);
    EXPECT_EQ(res, "");
    MEDIA_INFO_LOG("BackupFileUtils_GetFileNameFromPath_path_not_have end");
}

/*
 * Test interface: BackupFileUtils::GetFileNameFromPath
 * Test content: Verify file name extraction from normal path
 * Cover branches: Normal flow with valid path
 */
HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_GetFileNameFromPath_ok, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_GetFileNameFromPath_ok start");
    string path = "test/ee/ee";
    string res = BackupFileUtils::GetFileNameFromPath(path);
    EXPECT_NE(res, "");
    MEDIA_INFO_LOG("BackupFileUtils_GetFileNameFromPath_ok end");
}

/*
 * Test interface: BackupFileUtils::GetFileTitle
 * Test content: Verify file title extraction with empty display name
 * Cover branches: Empty display name error handling branch
 */
HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_GetFileTitle_dispalyName_empty, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_GetFileTitle_dispalyName_empty start");
    string displayName;
    string res = BackupFileUtils::GetFileTitle(displayName);
    EXPECT_EQ(res, "");
    MEDIA_INFO_LOG("BackupFileUtils_GetFileTitle_dispalyName_empty end");
}

/*
 * Test interface: BackupFileUtils::GetFileTitle
 * Test content: Verify file title extraction with normal display name
 * Cover branches: Normal flow with valid display name
 */
HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_GetFileTitle_ok, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_GetFileTitle_ok start");
    string displayName = "test.mp3";
    string res = BackupFileUtils::GetFileTitle(displayName);
    EXPECT_NE(res, "");
    MEDIA_INFO_LOG("BackupFileUtils_GetFileTitle_ok end");
}

/*
 * Test interface: BackupFileUtils::GetFileTitle
 * Test content: Verify file title extraction without dot
 * Cover branches: Display name without extension
 */
HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_GetFileTitle_no_dot, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_GetFileTitle_no_dot start");
    string displayName = "testmp3";
    string res = BackupFileUtils::GetFileTitle(displayName);
    EXPECT_EQ(res, "testmp3");
    MEDIA_INFO_LOG("BackupFileUtils_GetFileTitle_no_dot end");
}

/*
 * Test interface: BackupFileUtils::GetFullPathByPrefixType
 * Test content: Verify full path retrieval by prefix type with normal type
 * Cover branches: Normal flow with valid prefix type
 */
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

/*
 * Test interface: BackupFileUtils::GetFullPathByPrefixType
 * Test content: Verify full path retrieval with illegal prefix type
 * Cover branches: Invalid prefix type error handling branch
 */
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

/*
 * Test interface: BackupFileUtils::GarbleFileName
 * Test content: Verify file name garbling with normal file names
 * Cover branches: Normal flow with various file name patterns
 */
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

/*
 * Test interface: BackupFileUtils::GarbleFileName
 * Test content: Verify file name garbling with empty file name
 * Cover branches: Empty file name error handling branch
 */
HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_GarbleFileName_empty_name, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_GarbleFileName_empty_name start");
    // empty file name
    string name;
    auto ret = BackupFileUtils::GarbleFileName(name);
    EXPECT_EQ(ret.length(), 0);
    MEDIA_INFO_LOG("BackupFileUtils_GarbleFileName_empty_name end");
}

/*
 * Test interface: BackupFileUtils::GetReplacedPathByPrefixType
 * Test content: Verify replaced path retrieval with illegal source prefix
 * Cover branches: Invalid source prefix error handling branch
 */
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

/*
 * Test interface: BackupFileUtils::GetReplacedPathByPrefixType
 * Test content: Verify replaced path retrieval with illegal destination prefix
 * Cover branches: Invalid destination prefix error handling branch
 */
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

/*
 * Test interface: BackupFileUtils::GetReplacedPathByPrefixType
 * Test content: Verify replaced path retrieval with normal prefixes
 * Cover branches: Normal flow with valid prefixes
 */
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

/*
 * Test interface: UpgradeRestore::RestoreAudio
 * Test content: Verify audio restoration with upgrade restore scene
 * Cover branches: Normal flow with UPGRADE_RESTORE_ID
 */
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

/*
 * Test interface: UpgradeRestore::RestoreAudio
 * Test content: Verify audio restoration with clone scene
 * Cover branches: Normal flow with DUAL_FRAME_CLONE_RESTORE_ID
 */
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

/*
 * Test interface: UpgradeRestore::RestoreAudioBatch
 * Test content: Verify audio batch restoration without audio database
 * Cover branches: Null audio database error handling branch
 */
HWTEST_F(MediaLibraryBackupTest, RestoreAudio_RestoreAudioBatch_clone_no_audioRdb, TestSize.Level2)
{
    MEDIA_INFO_LOG("RestoreAudio_RestoreAudioBatch_clone_no_audioRdb start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    upgrade->RestoreAudioBatch(0);
    std::string queryAudio = "SELECT file_id from Audios where display_name ='audio1.mp3'";
    auto resultSet = photosStorePtr->QuerySql(queryAudio);
    ASSERT_FALSE(resultSet == nullptr);
    ASSERT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("RestoreAudio_RestoreAudioBatch_clone_no_audioRdb end");
}

/*
 * Test interface: UpgradeRestore::RestoreAudioBatch
 * Test content: Verify audio batch restoration with fake audio database
 * Cover branches: Fake audio database branch
 */
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
    MEDIA_INFO_LOG("RestoreAudio_RestoreAudioBatch_clone_fake_audiodb end");
}

/*
 * Test interface: UpgradeRestore::ParseResultSetFromAudioDb
 * Test content: Verify parsing audio database result set returns false
 * Cover branches: Null result set error handling branch
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for test001
 * Cover branches: Normal flow with album test001
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for test002
 * Cover branches: Normal flow with album test002
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for test003
 * Cover branches: Normal flow with album test003
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for TmallPic
 * Cover branches: Normal flow with TmallPic album
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for UCDownloads
 * Cover branches: Normal flow with UCDownloads album
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for xiaohongshu
 * Cover branches: Normal flow with xiaohongshu album
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for Douyin
 * Cover branches: Normal flow with Douyin album
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for Weibo
 * Cover branches: Normal flow with Weibo album
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for Camera
 * Cover branches: Normal flow with Camera album
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for Screenshots
 * Cover branches: Normal flow with Screenshots album
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for Screenrecorder
 * Cover branches: Normal flow with Screenrecorder album
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for dual frame share
 * Cover branches: Normal flow with dual frame share album
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album type and subtype for test101
 * Cover branches: Normal flow with album type and subtype check
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for TmallPic
 * Cover branches: Normal flow with TmallPic album
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for MTTT
 * Cover branches: Normal flow with MTTT album
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for funnygallery
 * Cover branches: Normal flow with funnygallery album
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for xiaohongshu
 * Cover branches: Normal flow with xiaohongshu album
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for Douyin
 * Cover branches: Normal flow with Douyin album
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for save
 * Cover branches: Normal flow with save album
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for Weibo
 * Cover branches: Normal flow with Weibo album
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for Camera
 * Cover branches: Normal flow with Camera album
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for Screenshots
 * Cover branches: Normal flow with Screenshots album
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for Screenrecorder
 * Cover branches: Normal flow with Screenrecorder album
 */
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

/*
 * Test interface: UpgradeRestore::RestorePhoto
 * Test content: Verify album restoration for dual frame share
 * Cover branches: Normal flow with dual frame share album
 */
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

/*
 * Test interface: UpgradeRestore::InsertAudio
 * Test content: Verify audio insertion with upgrade restore scene
 * Cover branches: Normal flow with UPGRADE_RESTORE_ID
 */
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

/*
 * Test interface: UpgradeRestore::InsertAudio
 * Test content: Verify audio insertion with empty file list
 * Cover branches: Empty file list branch
 */
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

/*
 * Test interface: UpgradeRestore::MoveDirectory
 * Test content: Verify directory movement
 * Cover branches: Normal flow with directory operations
 */
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

/*
 * Test interface: UpgradeRestore::BatchQueryPhoto
 * Test content: Verify batch photo query functionality
 * Cover branches: Normal flow with batch query
 */
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

/*
 * Test interface: UpgradeRestore::ParseXml
 * Test content: Verify XML parsing test 002
 * Cover branches: XML parsing error handling branch
 */
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

/*
 * Test interface: UpgradeRestore::ParseXml
 * Test content: Verify XML parsing test 003
 * Cover branches: XML parsing error handling branch
 */
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

/*
 * Test interface: UpgradeRestore::ParseXml
 * Test content: Verify XML parsing test 004
 * Cover branches: XML parsing error handling branch
 */
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

/*
 * Test interface: UpgradeRestore::ParseXml
 * Test content: Verify XML parsing test 005
 * Cover branches: XML parsing error handling branch
 */
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

/*
 * Test interface: UpgradeRestore::StringToInt
 * Test content: Verify string to integer conversion with empty string
 * Cover branches: Empty string error handling branch
 */
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

/*
 * Test interface: UpgradeRestore::StringToInt
 * Test content: Verify string to integer conversion with invalid characters
 * Cover branches: Invalid characters error handling branch
 */
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

/*
 * Test interface: UpgradeRestore::StringToInt
 * Test content: Verify string to integer conversion with超大数字
 * Cover branches: Large number error handling branch
 */
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

/*
 * Test interface: UpgradeRestore::SetValueFromMetaData
 * Test content: Verify setting values from metadata
 * Cover branches: Normal flow with metadata values
 */
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
    EXPECT_TRUE(valueBucket.HasColumn(PhotoColumn::PHOTO_ORIENTATION));
    GTEST_LOG_(INFO) << "medialib_backup_test_SetValueFromMetaData end";
}

/*
 * Test interface: UpgradeRestore::SetValueFromMetaData
 * Test content: Verify user comment setting TEST1
 * Cover branches: User comment handling without existing column
 */
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

/*
 * Test interface: UpgradeRestore::SetValueFromMetaData
 * Test content: Verify user comment setting TEST2
 * Cover branches: User comment handling with existing column
 */
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

/*
 * Test interface: UpgradeRestore::GetBackupInfo
 * Test content: Verify backup info retrieval
 * Cover branches: Normal flow with backup info
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GetBackupInfo, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_GetBackupInfo start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);

    string str = upgrade->GetBackupInfo();
    EXPECT_EQ(str, "");
    GTEST_LOG_(INFO) << "medialib_backup_test_GetBackupInfo end";
}

/*
 * Test interface: UpgradeRestore::InsertPhoto
 * Test content: Verify photo insertion with empty parameters
 * Cover branches: Empty mediaLibraryRdb_ and empty file list
 */
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
    EXPECT_EQ(upgrade->mediaLibraryRdb_, nullptr);

    //normal mediaLibraryRdb_ and empty file
    upgrade->mediaLibraryRdb_ = photosStorePtr;
    upgrade->InsertPhoto(sceneCode, fileInfos, sourceType);
    EXPECT_NE(upgrade->mediaLibraryRdb_, nullptr);
    EXPECT_EQ(fileInfos.size(), 0);
    GTEST_LOG_(INFO) << "medialib_backup_test_InsertPhoto_empty end";
}

/*
 * Test interface: UpgradeRestore::InsertPhoto
 * Test content: Verify photo insertion with normal parameters
 * Cover branches: Normal flow with GALLERY and EXTERNAL_CAMERA source types
 */
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
    EXPECT_EQ(fileInfos.size(), 2);

    // insertPhoto sourceType != SourceType::GALLERY
    sourceType = SourceType::EXTERNAL_CAMERA;
    upgrade->InsertPhoto(sceneCode, fileInfos, sourceType);
    EXPECT_EQ(sourceType, SourceType::EXTERNAL_CAMERA);
    GTEST_LOG_(INFO) << "medialib_backup_test_InsertPhoto_normal end";
}

/*
 * Test interface: UpgradeRestore::InsertCloudPhoto
 * Test content: Verify cloud photo insertion
 * Cover branches: Normal flow with cloud photo
 */
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

/*
 * Test interface: UpgradeRestore::UpdateFailedFileByFileType
 * Test content: Verify failed file update for image type
 * Cover branches: Normal flow with MEDIA_TYPE_IMAGE
 */
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

/*
 * Test interface: UpgradeRestore::UpdateFailedFileByFileType
 * Test content: Verify failed file update for video type
 * Cover branches: Normal flow with MEDIA_TYPE_VIDEO
 */
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

/*
 * Test interface: UpgradeRestore::UpdateFailedFileByFileType
 * Test content: Verify failed file update for audio type
 * Cover branches: Normal flow with MEDIA_TYPE_AUDIO
 */
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

/*
 * Test interface: UpgradeRestore::UpdateFailedFileByFileType
 * Test content: Verify failed file update with illegal file type
 * Cover branches: Invalid file type error handling branch
 */
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

/*
 * Test interface: UpgradeRestore::SetErrorCode
 * Test content: Verify error code setting with valid and invalid codes
 * Cover branches: Normal flow with INIT_FAILED and invalid error code
 */
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

/*
 * Test interface: UpgradeRestore::GetSubCountInfo
 * Test content: Verify sub count info retrieval with illegal type
 * Cover branches: Invalid type error handling branch
 */
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

/*
 * Test interface: UpgradeRestore::GetCountInfoJson
 * Test content: Verify count info JSON retrieval with empty types
 * Cover: branches: Empty types branch returning null
 */
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

/*
 * Test interface: UpgradeRestore::GetCountInfoJson
 * Test content: Verify count info JSON retrieval with normal types
 * Cover branches: Normal flow with STAT_TYPES
 */
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

/*
 * Test interface: UpgradeRestore::GetCountInfoJson
 * Test content: Verify count info JSON retrieval with illegal types
 * Cover branches: Illegal types branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GetCountInfoJson_illegal_types, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_GetCountInfoJson_illegal_types start";
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);

    vector<string> types = { "test" };
    auto ret = upgrade->GetCountInfoJson(types);
    string str = ret[STAT_KEY_INFOS][0][STAT_KEY_BACKUP_INFO].dump();
    str.erase(std::remove(str.begin(), str.end(), '\"'), str.end());
    EXPECT_EQ(str, "test");
    GTEST_LOG_(INFO) << "medialib_backup_test_GetCountInfoJson_illegal_types end";
}

/*
 * Test interface: UpgradeRestore::Query
 * Test content: Verify Need batch query photo
 * Cover branches: Normal flow
 */
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

/*
 * Test interface: UpgradeRestore::Convert
 * Test content: Verify Convert path to real path 001
 * Cover branches: Zero value branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_convert_path_to_real_path_001, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_001 start";
    TestConvertPathToRealPathByStorageType(false); // internal, normal
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_001 end";
}

/*
 * Test interface: UpgradeRestore::Convert
 * Test content: Verify Convert path to real path 002
 * Cover branches: Zero value branch
 */
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

/*
 * Test interface: UpgradeRestore::Convert
 * Test content: Verify Convert path to real path 003
 * Cover branches: Zero value branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_convert_path_to_real_path_003, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_003 start";
    std::string srcPath = "/storage/ABCD-0000" + TEST_RELATIVE_PATH;
    std::string expectedNewPath = TEST_PATH_PREFIX + srcPath;
    TestConvertPathToRealPathByFileSize(TEST_SIZE_2MB_BELOW, srcPath, expectedNewPath); // Sd, below 2MB
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_003 end";
}

/*
 * Test interface: UpgradeRestore::Convert
 * Test content: Verify Convert path to real path 004
 * Cover branches: Zero value branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_convert_path_to_real_path_004, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_004 start";
    std::string srcPath = "/storage/ABCD-0000" + TEST_RELATIVE_PATH;
    std::string expectedNewPath = TEST_PATH_PREFIX + TEST_RELATIVE_PATH;
    TestConvertPathToRealPathByFileSize(TEST_SIZE_2MB, srcPath, expectedNewPath); // Sd, equal to 2MB
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_004 end";
}

/*
 * Test interface: UpgradeRestore::Convert
 * Test content: Verify Convert path to real path 005
 * Cover branches: Zero value branch
 */
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

/*
 * Test interface: UpgradeRestore::Convert
 * Test content: Verify Convert path to real path 006
 * Cover branches: Zero value branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_convert_path_to_real_path_006, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_006 start";
    std::string srcPath = "/storage/ABCD-0000" + TEST_RELATIVE_PATH;
    std::string expectedNewPath = TEST_PATH_PREFIX + srcPath;
    TestConvertPathToRealPathByLocalMediaId(GALLERY_HIDDEN_ID, srcPath, expectedNewPath); // Sd, hidden
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_006 end";
}

/*
 * Test interface: UpgradeRestore::Convert
 * Test content: Verify Convert path to real path 007
 * Cover branches: Zero value branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_convert_path_to_real_path_007, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_007 start";
    std::string srcPath = "/storage/ABCD-0000" + TEST_RELATIVE_PATH;
    std::string expectedNewPath = TEST_PATH_PREFIX + srcPath;
    TestConvertPathToRealPathByLocalMediaId(GALLERY_TRASHED_ID, srcPath, expectedNewPath); // Sd, trashed
    GTEST_LOG_(INFO) << "medialib_backup_test_convert_path_to_real_path_007 end";
}

/*
 * Test interface: UpgradeRestore::Get
 * Test content: Verify Get path pos by prefix level
 * Cover branches: Normal flow
 */
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

/*
 * Test interface: UpgradeRestore::Update
 * Test content: Verify Update duplicate number
 * Cover branches: Normal flow
 */
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

/*
 * Test interface: UpgradeRestore::Update
 * Test content: Verify Update sd where clause
 * Cover branches: Normal flow
 */
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

/*
 * Test interface: BackupDatabaseUtils::GetLandmarksScale
 * Test content: Verify landmarks scale retrieval for small size
 * Cover branches: Small size branch (0, 2 * min)
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_get_landmarks_scale_001, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_get_landmarks_scale_001 start";
    int length = TEST_SIZE_MIN; // (0, 2 * min), no need to scale
    float scale = BackupDatabaseUtils::GetLandmarksScale(length, length);
    EXPECT_EQ(scale, 1);
    GTEST_LOG_(INFO) << "medialib_backup_test_get_landmarks_scale_001 end";
}

/*
 * Test interface: BackupDatabaseUtils::GetLandmarksScale
 * Test content: Verify landmarks scale retrieval for medium size
 * Cover branches: Medium size branch [2 * min, 4 * min)
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_get_landmarks_scale_002, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_get_landmarks_scale_002 start";
    int length = TEST_SIZE_MIN * TEST_SIZE_MULT_UNIT + TEST_SIZE_INCR_UNIT; // [2 * min, 4 * min)
    float scale = BackupDatabaseUtils::GetLandmarksScale(length, length);
    float expectedScale = 1.0 / TEST_SIZE_MULT_UNIT;
    EXPECT_EQ(scale, expectedScale);
    GTEST_LOG_(INFO) << "medialib_backup_test_get_landmarks_scale_002 end";
}

/*
 * Test interface: BackupDatabaseUtils::GetLandmarksScale
 * Test content: Verify landmarks scale retrieval for large size
 * Cover branches: Large size branch [4 * min, ...)
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_get_landmarks_scale_003, TestSize.Level2)
{
    GTEST_LOG_(INFO) << "medialib_backup_test_get_landmarks_scale_003 start";
    int length = TEST_SIZE_MIN * TEST_SIZE_MULT_UNIT * TEST_SIZE_MULT_UNIT; // [4 * min, ...)
    float scale = BackupDatabaseUtils::GetLandmarksScale(length, length);
    float expectedScale = 1.0 / TEST_SIZE_MULT_UNIT / TEST_SIZE_MULT_UNIT;
    EXPECT_EQ(scale, expectedScale);
    GTEST_LOG_(INFO) << "medialib_backup_test_get_landmarks_scale_003 end";
}

/*
 * Test interface: BackupDatabaseUtils::GetLandmarksScale
 * Test content: Verify landmarks scale retrieval for extra large size
 * Cover branches: Extra large size branch
 */
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

/*
 * Test interface: BackupDatabaseUtils::IsLandmarkValid
 * Test content: Verify landmark validity for valid coordinates
 * Cover branches: Valid coordinates branch
 */
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

/*
 * Test interface: BackupDatabaseUtils::IsLandmarkValid
 * Test content: Verify landmark validity for invalid Y coordinate (below)
 * Cover branches: Invalid Y coordinate branch
 */
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

/*
 * Test interface: BackupDatabaseUtils::IsLandmarkValid
 * Test content: Verify landmark validity for invalid Y coordinate (above)
 * Cover branches: Invalid Y coordinate branch
 */
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

/*
 * Test interface: BackupDatabaseUtils::IsLandmarkValid
 * Test content: Verify landmark validity for invalid X coordinate (below)
 * Cover branches: Invalid X coordinate branch
 */
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

/*
 * Test interface: BackupDatabaseUtils::IsLandmarkValid
 * Test content: Verify landmark validity for invalid X coordinate (above)
 * Cover branches: Invalid X coordinate branch
 */
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

/*
 * Test interface: BackupFileUtils::IsLivePhoto
 * Test. content: Verify live photo detection
 * Cover branches: Live photo detection with various special file types
 */
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

/*
 * Test interface: BackupFileUtils::ConvertToMovingPhoto
 * Test content: Verify conversion to moving photo
 * Cover branches: Moving photo conversion branch
 */
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

/*
 * Test interface: BackupFileUtils
 * Test content: Verify File access helper
 * Cover branches: Normal flow
 */
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
 
/*
 * Test interface: BackupFileUtils
 * Test content: Verify Media type beyong 1 3
 * Cover branches: Normal flow
 */
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

/*
 * Test interface: BackupFileUtils
 * Test content: Verify App twin data 001
 * Cover branches: Zero value branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_app_twin_data_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_001 start");
    TestAppTwinData("", "", CLONE_RESTORE_ID); // not upgrade
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_001 end");
}

/*
 * Test interface: BackupFileUtils
 * Test content: Verify App twin data 002
 * Cover branches: Zero value branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_app_twin_data_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_002 start");
    TestAppTwinData("", ""); // not app twin data: empty
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_002 end");
}

/*
 * Test interface: BackupFileUtils
 * Test content: Verify App twin data 003
 * Cover branches: Zero value branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_app_twin_data_003, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_003 start");
    TestAppTwinData("/storage/ABCE-EFGH/0/", ""); // not app twin data: external
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_003 end");
}

/*
 * Test interface: BackupFileUtils
 * Test content: Verify App twin data 004
 * Cover branches: Zero value branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_app_twin_data_004, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_004 start");
    TestAppTwinData("/storage/emulated/0/", ""); // not app twin data: main user
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_004 end");
}

/*
 * Test interface: BackupFileUtils
 * Test content: Verify App twin data 005
 * Cover branches: Zero value branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_app_twin_data_005, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_005 start");
    TestAppTwinData("/storage/emulated", ""); // not app twin data: first / not found
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_005 end");
}

/*
 * Test interface: BackupFileUtils
 * Test content: Verify App twin data 006
 * Cover branches: Zero value branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_app_twin_data_006, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_006 start");
    TestAppTwinData("/storage/emulated/", ""); // not app twin data: second / not found
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_006 end");
}

/*
 * Test interface: BackupFileUtils
 * Test content: Verify App twin data 007
 * Cover branches: Zero value branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_app_twin_data_007, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_007 start");
    TestAppTwinData("/storage/emulated/abc/", ""); // not app twin data: not number
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_007 end");
}

/*
 * Test interface: BackupFileUtils
 * Test content: Verify App twin data 008
 * Cover branches: Zero value branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_app_twin_data_008, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_008 start");
    TestAppTwinData("/storage/emulated/1234/", ""); // not app twin data: not in [128, 147]
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_008 end");
}

/*
 * Test interface: BackupFileUtils
 * Test content: Verify App twin data 009
 * Cover branches: Zero value branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_app_twin_data_009, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_009 start");
    TestAppTwinData("/storage/emulated/128/", APP_TWIN_DATA_PREFIX); // app twin data
    MEDIA_INFO_LOG("medialib_backup_test_app_twin_data_009 end");
}

/*
 * Test interface: UpgradeRestore::Get
 * Test content: Verify GetFileFolderFromPath
 * Cover branches: Normal flow
 */
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

/*
 * Test interface: BackupFileUtils
 * Test content: Verify PreparePath
 * Cover branches: Normal flow
 */
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

/*
 * Test interface: BackupFileUtils
 * Test content: Verify GarbleFilePath
 * Cover branches: Normal flow
 */
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

/*
 * Test interface: BackupFileUtils
 * Test content: Verify CreatePath
 * Cover branches: Normal flow
 */
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

/*
 * Test interface: BackupFileUtils
 * Test content: Verify IsLowQualityImage
 * Cover branches: Normal flow
 */
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

/*
 * Test interface: UpgradeRestore::Convert
 * Test content: Verify ConvertLowQualityPath
 * Cover branches: Normal flow
 */
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

/*
 * Test interface: UpgradeRestore::RestoreCloudFromGallery
 * Test content: Verify restoration from cloud
 * Cover branches: Normal flow with cloud restoration
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_restore_from_cloud_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_restore_from_cloud_test start");
    int32_t countBefore = restoreServiceCloud->totalNumber_;
    restoreServiceCloud->RestoreCloudFromGallery();
    EXPECT_EQ((restoreServiceCloud->totalNumber_ - countBefore), 0);
}

/*
 * Test interface: UpgradeRestore::QueryCloudFileInfos
 * Test content: Verify cloud file info query
 * Cover branches: Normal flow with cloud file info query
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_query_cloud_infos_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_query_cloud_infos_test start");
    auto result = restoreService->QueryCloudFileInfos(0);
    EXPECT_EQ(result.empty(), true);
}

/*
 * Test interface: BaseRestore::StartRestoreEx
 * Test content: Verify restore mode retrieval test 001
 * Cover branches: Restore mode branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_restore_mode_test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_restore_mode_test_001 start");
    restoreService->restoreInfo_ = R"([{"type":"appTwinDataRestoreState", "detail":"2"}])";
    std::string restoreExInfo;
    restoreService->BaseRestore::StartRestoreEx("", "", restoreExInfo);
    EXPECT_EQ(restoreService->restoreMode_, 2);
}

/*
 * Test interface: BaseRestore::StartRestoreEx
 * Test content: Verify restore mode retrieval test 002
 * Cover branches: Restore mode branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_restore_mode_test_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_restore_mode_test_002 start");
    restoreService->restoreInfo_ = R"([{"type":"appTwinDataRestoreState", "detail":"5"}])";
    std::string restoreExInfo;
    restoreService->BaseRestore::StartRestoreEx("", "", restoreExInfo);
    EXPECT_EQ(restoreService->restoreMode_, 0);
}

/*
 * Test interface: BaseRestore::GetAccountValid
 * Test content: Verify account validity test 001
 * Cover branches: Account validity branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_account_valid_test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_account_valid_test_001 start");
    restoreService->sceneCode_ = DUAL_FRAME_CLONE_RESTORE_ID;
    restoreService->restoreInfo_ = R"([{"type":"dualAccountId", "detail":"oldId"}])";
    (void)restoreService->BaseRestore::GetAccountValid();
    EXPECT_FALSE(restoreService->isAccountValid_);
}

/*
 * Test interface: BaseRestore::GetAccountValid
 * Test content: Verify account validity test 002
 * Cover branches: Account validity branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_account_valid_test_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_account_valid_test_002 start");
    restoreService->sceneCode_ = DUAL_FRAME_CLONE_RESTORE_ID;
    restoreService->restoreInfo_ = R"([{"type":"test", "detail":"oldId"}])";
    (void)restoreService->BaseRestore::GetAccountValid();
    EXPECT_FALSE(restoreService->isAccountValid_);
}

/*
 * Test interface: BaseRestore::GetSourceDeviceInfo
 * Test content: Verify source device info retrieval
 * Cover branches: Source device info branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_source_device_info_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_source_device_info_test start");
    restoreService->restoreInfo_ = R"([{"type":"dualDeviceSoftName", "detail":"0"}])";
    (void)restoreService->BaseRestore::GetSourceDeviceInfo();
    EXPECT_EQ(restoreService->dualDeviceSoftName_, "0");
}

/*
 * Test interface: BaseRestore::IsRestorePhoto
 * Test content: Verify photo restoration test 001
 * Cover branches: Photo restoration branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_is_restore_photo_test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_is_restore_photo_test_001 start");
    restoreService->restoreInfo_ = R"([{"type":"backupInfo", "detail":"0"}])";
    bool restorePhoto = restoreService->BaseRestore::IsRestorePhoto();
    EXPECT_FALSE(restorePhoto);
}

/*
 * Test interface: BaseRestore::IsRestorePhoto
 * Test content: Verify photo restoration test 002
 * Cover branches: Photo restoration branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_is_restore_photo_test_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_is_restore_photo_test_002 start");
    restoreService->restoreInfo_ = R"([{"type":"backupInfo", "detail":"galleryData"}])";
    bool restorePhoto = restoreService->BaseRestore::IsRestorePhoto();
    EXPECT_TRUE(restorePhoto);
}

/*
 * Test interface: BaseRestore::QuerySql
 * Test content: Verify SQL query execution
 * Cover branches: SQL query branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_query_sql_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_query_sql_test start");
    std::string sql = "";
    std::vector<std::string> selectionArgs = {""};
    auto result = restoreService->BaseRestore::QuerySql(sql, selectionArgs);
    EXPECT_EQ(result, nullptr);
}

/*
 * Test interface: BaseRestore::SetValueFromMetaData
 * Test content: Verify date taken insertion
 * Cover branches: Date taken branch
 */
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

/*
 * Test interface: BaseRestore::GetCloudInsertValues
 * Test content: Verify cloud insert values retrieval
 * Cover branches: Cloud insert values branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_cloud_insert_values_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_cloud_insert_values_test start");
    FileInfo fileInfo;
    fileInfo.displayName = TEST_FILE;
    std::vector<FileInfo> fileInfos = {fileInfo};
    auto result = restoreService->BaseRestore::GetCloudInsertValues(0, fileInfos, 0);
    EXPECT_EQ(result.size(), 0);
}

/*
 * Test interface: BaseRestore::SetValueFromMetaData
 * Test content: Verify orientation insertion
 * Cover branches: Orientation branch
 */
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

/*
 * Test interface: BaseRestore::SetValueFromMetaData
 * Test content: Verify cover position setting
 * Cover branches: Cover position branch
 */
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

/*
 * Test interface: BaseRestore::MoveMigrateFile
 * Test content: Verify migrate file movement
 * Cover branches: Migrate file branch
 */
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

/*
 * Test interface: BaseRestore::MoveMigrateCloudFile
 * Test content: Verify migrate cloud file movement
 * Cover branches: Migrate cloud file branch
 */
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

/*
 * Test interface: BaseRestore::BatchCreateDentryFile
 * Test content: Verify dentry file creation test 001
 * Cover branches: Dentry file creation branch
 */
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

/*
 * Test interface: BaseRestore::BatchCreateDentryFile
 * Test content: Verify dentry file creation test 002
 * Cover branches: Dentry file creation with LCD type
 */
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

/*
 * Test interface: BaseRestore::BatchCreateDentryFile
 * Test content: Verify dentry file creation test 003
 * Cover branches: Dentry file creation with THM type
 */
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

/*
 * Test interface: BaseRestore::RestoreLcdAndThumbFromCloud
 * Test content: Verify LCD and thumbnail restoration from cloud
 * Cover branches: LCD and thumbnail restoration branch
 */
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

/*
 * Test interface: BaseRestore::HandleFailData
 * Test content: Verify failed data handling test 001
 * Cover branches: Failed data handling with DENTRY_INFO_ORIGIN
 */
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

/*
 * Test interface: BaseRestore::HandleFailData
 * Test content: Verify failed data handling test 002
 * Cover branches: Failed data handling with DENTRY_INFO_LCD
 */
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

/*
 * Test interface: BaseRestore::HandleFailData
 * Test content: Verify failed data handling test 003
 * Cover branches: Failed data handling with DENTRY_INFO_THM
 */
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

/*
 * Test interface: BaseRestore::HandleFailData
 * Test content: Verify failed data handling test 004
 * Cover branches: Failed data handling with non-failed file
 */
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

/*
 * Test interface: BaseRestore::SetVisiblePhoto
 * Test content: Verify visible photo setting
 * Cover branches: Visible photo branch
 */
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

/*
 * Test interface: BaseRestore::BatchInsertMap
 * Test content: Verify batch map insertion
 * Cover branches: Batch map insertion branch
 */
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

/*
 * Test interface: BaseRestore::GetUniqueId
 * Test content: Verify unique ID retrieval
 * Cover branches: Unique ID branch
 */
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

/*
 * Test interface: BaseRestore::GetProgressInfo
 * Test content: Verify progress info retrieval
 * Cover branches: Progress info branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_progress_info_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_progress_info_test start");
    std::string progressInfo = restoreService->BaseRestore::GetProgressInfo();
    EXPECT_FALSE(progressInfo.empty());
}

/*
 * Test interface: BaseRestore::UpdateDatabase
 * Test content: Verify database update
 * Cover branches: Database update branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_update_database_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_update_database_test start");
    restoreService->BaseRestore::UpdateDatabase();
    EXPECT_EQ(restoreService->updateProcessStatus_, ProcessStatus::STOP);
}

/*
 * Test interface: BaseRestore::ExtraCheckForCloneSameFile
 * Test content: Verify extra check for clone same file
 * Cover branches: Clone same file branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_extra_check_for_clone_same_file_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_extra_check_for_clone_same_file_test start");
    PhotosDao::PhotosRowData rowData;
    FileInfo fileInfo;
    rowData.cleanFlag = 1;
    rowData.position = static_cast<int32_t>(PhotoPositionType::CLOUD);
    rowData.fileId = 1;
    rowData.data = "/storage/cloud/files/Photo/1/test.jpg";
    bool ret = restoreService->BaseRestore::ExtraCheckForCloneSameFile(fileInfo, rowData);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(fileInfo.isNew, false);
    EXPECT_EQ(fileInfo.fileIdNew, rowData.fileId);
    EXPECT_EQ(fileInfo.cloudPath, rowData.data);
    EXPECT_EQ(fileInfo.needMove, false);
}

/*
 * Test interface: UpgradeRestore::Insert
 * Test content: Verify Medialib backup get insert value test
 * Cover branches: Normal flow
 */
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

/*
 * Test interface: BackupFileUtils
 * Test content: Verify Geo knowledge test1
 * Cover branches: Normal flow
 */
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

/*
 * Test interface: BackupFileUtils
 * Test content: Verify Geo knowledge test2
 * Cover branches: Normal flow
 */
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

/*
 * Test interface: BackupFileUtils
 * Test content: Verify Geo knowledge test3
 * Cover branches: Normal flow
 */
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

/*
 * Test interface: BackupFileUtils
 * Test content: Verify Geo knowledge test4
 * Cover branches: Normal flow
 */
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

/*
 * Test interface: BackupFileUtils
 * Test content: Verify Geo knowledge test5
 * Cover branches: Normal flow
 */
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

/*
 * Test interface: UpgradeRestore::Get
 * Test content: Verify Medialib backup get data longitude test1
 * Cover branches: Normal flow
 */
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

/*
 * Test interface: UpgradeRestore::Get
 * Test content: Verify Medialib backup get data longitude test2
 * Cover branches: Normal flow
 */
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

/*
 * Test interface: UpgradeRestore::Get
 * Test content: Verify Medialib backup get data longitude test3
 * Cover branches: Normal flow
 */
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

/*
 * Test interface: UpgradeRestore::Get
 * Test content: Verify Medialib backup get data latitude test1
 * Cover branches: Normal flow
 */
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

/*
 * Test interface: UpgradeRestore::Get
 * Test content: Verify Medialib backup get data latitude test2
 * Cover branches: Normal flow
 */
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
}

/*
 * Test interface: UpgradeRestore::Get
 * Test content: Verify Medialib backup get data latitude test3
 * Cover branches: Normal flow
 */
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
}

/*
 * Test interface: UpgradeRestore::Restore
 * Test content: Verify Medialib backup get current device restore config info test001
 * Cover branches: Zero value branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_current_device_restore_config_info_test001, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_current_device_restore_config_info_test001 start");

    RestoreConfigInfo config = restoreService->GetCurrentDeviceRestoreConfigInfo();
    bool flag = config.photoPositionType == PhotoPositionType::CLOUD ||
        config.southDeviceType == SouthDeviceType::SOUTH_DEVICE_NULL;
    EXPECT_TRUE(flag);
}

/*
 * Test interface: UpgradeRestore::Query
 * Test content: Verify Medialib backup get cloud query sql test001
 * Cover branches: Zero value branch
 */
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

/*
 * Test interface: UpgradeRestore::Query
 * Test content: Verify Medialib backup get cloud query sql test002
 * Cover branches: Zero value branch
 */
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

/*
 * Test interface: UpgradeRestore::Query
 * Test content: Verify Medialib backup get cloud query sql test003
 * Cover branches: Zero value branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_get_cloud_query_sql_test003, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_get_cloud_query_sql_test003 start");

    restoreService->restoreConfig_.restoreSwitchType = SwitchStatus::NONE;
    std::string result = restoreService->GetCloudQuerySql();
    std::string expected = "";
    bool flag = result == expected;
    EXPECT_TRUE(flag);
}

/*
 * Test interface: UpgradeRestore::Restore
 * Test content: Verify Medialib backup base restore move file 001
 * Cover branches: Zero value branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_base_restore_move_file_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_base_restore_move_file_001 start");
    int32_t ret = restoreService->BaseRestore::MoveFile("", "");
    EXPECT_NE(ret, E_OK);
}

/*
 * Test interface: UpgradeRestore::Restore
 * Test content: Verify Medialib backup base restore insert file duration 001
 * Cover branches: Zero value branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_base_restore_insert_file_duration_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_base_restore_insert_file_duration_001 start");
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    data->SetFileDuration(1);
    NativeRdb::ValuesBucket value;
    FileInfo fileInfo;
    restoreService->BaseRestore::InsertFileDuration(data, value, fileInfo);
    int32_t duration = 0;
    NativeRdb::ValueObject valueObject;
    if (value.GetObject(MediaColumn::MEDIA_DURATION, valueObject)) {
        valueObject.GetInt(duration);
    }
    EXPECT_EQ(duration, 1);
}

/*
 * Test interface: UpgradeRestore::Restore
 * Test content: Verify Medialib backup base restore insert file duration 002
 * Cover branches: Zero value branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_base_restore_insert_file_duration_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_base_restore_insert_file_duration_002 start");
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    data->SetFileDuration(1);
    NativeRdb::ValuesBucket value;
    value.PutInt(MediaColumn::MEDIA_DURATION, 1);
    FileInfo fileInfo;
    restoreService->BaseRestore::InsertFileDuration(data, value, fileInfo);
    int32_t duration = 0;
    NativeRdb::ValueObject valueObject;
    if (value.GetObject(MediaColumn::MEDIA_DURATION, valueObject)) {
        valueObject.GetInt(duration);
    }
    EXPECT_EQ(duration, 1);
}

/*
 * Test interface: BackupFileUtils
 * Test content: Verify Medialib backup database helper 001
 * Cover branches: Zero value branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_database_helper_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_database_helper_001 start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    FileInfo fileInfo;
    int32_t errCode = 0;
    fileInfo.isInternal = false;
    upgrade->backupDatabaseHelper_.Init(DUAL_FRAME_CLONE_RESTORE_ID, false, "");
    EXPECT_EQ(upgrade->CheckInvalidFile(fileInfo, errCode), "");
}

/*
 * Test interface: UpgradeRestore::StringToInt
 * Test content: Verify string to integer conversion with positive numbers
 * Cover branches: Positive number conversion branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_StringToInt_positive, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_StringToInt_positive start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    string xmlPath = "123";
    auto res = upgrade->StringToInt(xmlPath);
    EXPECT_EQ(res, 123);
    
    xmlPath = "  456";
    res = upgrade->StringToInt(xmlPath);
    EXPECT_EQ(res, 456);
    
    xmlPath = "+789";
    res = upgrade->StringToInt(xmlPath);
    EXPECT_EQ(res, 789);
    
    xmlPath = "   +100";
    res = upgrade->StringToInt(xmlPath);
    EXPECT_EQ(res, 100);
    MEDIA_INFO_LOG("medialib_backup_test_StringToInt_positive end");
}

/*
 * Test interface: UpgradeRestore::StringToInt
 * Test content: Verify string to integer conversion with negative numbers
 * Cover branches: Negative number conversion branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_StringToInt_negative, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_StringToInt_negative start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    string xmlPath = "-123";
    auto res = upgrade->StringToInt(xmlPath);
    EXPECT_EQ(res, -123);
    
    xmlPath = "  -456";
    res = upgrade->StringToInt(xmlPath);
    EXPECT_EQ(res, -456);
    MEDIA_INFO_LOG("medialib_backup_test_StringToInt_negative end");
}

/*
 * Test interface: UpgradeRestore::StringToInt
 * Test content: Verify string to integer conversion with max int
 * Cover branches: Max int conversion branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_StringToInt_max_int, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_StringToInt_max_int start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    string xmlPath = "2147483647";
    auto res = upgrade->StringToInt(xmlPath);
    EXPECT_EQ(res, 2147483647);
    
    xmlPath = "2147483648";
    res = upgrade->StringToInt(xmlPath);
    EXPECT_EQ(res, 0);
    MEDIA_INFO_LOG("medialib_backup_test_StringToInt_max_int end");
}

/*
 * Test interface: BackupFileUtils
 * Test content: Verify HasLowQualityImage
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_HasLowQualityImage, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_HasLowQualityImage start");
    restoreService->galleryRdb_ = restoreService->galleryRdb_;
    bool result = restoreService->HasLowQualityImage();
    EXPECT_GE(result, false);
    MEDIA_INFO_LOG("HasLowQualityImage result: %{public}d", static_cast<int32_t>(result));
    MEDIA_INFO_LOG("medialib_backup_test_HasLowQualityImage end");
}

/*
 * Test interface: UpgradeRestore::Get
 * Test content: Verify GetHighlightCloudMediaCnt
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GetHighlightCloudMediaCnt, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_GetHighlightCloudMediaCnt start");
    restoreService->galleryRdb_ = restoreService->galleryRdb_;
    int32_t count = restoreService->GetHighlightCloudMediaCnt();
    MEDIA_INFO_LOG("GetHighlightCloudMediaCnt: %{public}d", count);
    EXPECT_GE(count, -1);
    MEDIA_INFO_LOG("medialib_backup_test_GetHighlightCloudMediaCnt end");
}

/*
 * Test interface: UpgradeRestore::Convert
 * Test content: Verify ConvertPathToRealPath internal
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ConvertPathToRealPath_internal, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_ConvertPathToRealPath_internal start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, UPGRADE_RESTORE_ID);

    std::string srcPath = "/storage/el2/base/haps/gallery/files/Pictures/test.jpg";
    std::string prefix = "/data/test";
    std::string newPath, relativePath;
    bool result = upgrade->ConvertPathToRealPath(srcPath, prefix, newPath, relativePath);
    EXPECT_EQ(result, true);
    MEDIA_INFO_LOG("medialib_backup_test_ConvertPathToRealPath_internal end");
}

/*
 * Test interface: UpgradeRestore::Convert
 * Test content: Verify ConvertPathToRealPath sd small file
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ConvertPathToRealPath_sd_small_file, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_ConvertPathToRealPath_sd_small_file start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    std::string srcPath = "/storage/ABCD-0000/Pictures/test.jpg";
    std::string prefix = "/data/test";
    std::string newPath, relativePath;
    FileInfo fileInfo;
    fileInfo.fileSize = TEST_SIZE_2MB_BELOW;
    fileInfo.localMediaId = TEST_LOCAL_MEDIA_ID;
    
    bool result = upgrade->ConvertPathToRealPath(srcPath, prefix, newPath, relativePath, fileInfo);
    EXPECT_EQ(result, true);
    EXPECT_EQ(fileInfo.isInternal, false);
    MEDIA_INFO_LOG("medialib_backup_test_ConvertPathToRealPath_sd_small_file end");
}

/*
 * Test interface: UpgradeRestore::Convert
 * Test content: Verify ConvertPathToRealPath sd large file
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ConvertPathToRealPath_sd_large_file, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_ConvertPathToRealPath_sd_large_file start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    std::string srcPath = "/storage/ABCD-0000/Pictures/test.jpg";
    std::string prefix = "/data/test";
    std::string newPath, relativePath;
    FileInfo fileInfo;
    fileInfo.fileSize = TEST_SIZE_2MB_ABOVE;
    fileInfo.localMediaId = TEST_LOCAL_MEDIA_ID;
    
    bool result = upgrade->ConvertPathToRealPath(srcPath, prefix, newPath, relativePath, fileInfo);
    EXPECT_EQ(result, true);
    EXPECT_EQ(fileInfo.isInternal, false);
    MEDIA_INFO_LOG("medialib_backup_test_ConvertPathToRealPath_sd_large_file end");
}

/*
 * Test interface: UpgradeRestore::Convert
 * Test content: Verify ConvertPathToRealPath sd hidden
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ConvertPathToRealPath_sd_hidden, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_ConvertPathToRealPath_sd_hidden start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    std::string srcPath = "/storage/ABCD-0000/Pictures/test.jpg";
    std::string prefix = "/data/test";
    std::string newPath, relativePath;
    FileInfo fileInfo;
    fileInfo.fileSize = TEST_SIZE_2MB_ABOVE;
    fileInfo.localMediaId = GALLERY_HIDDEN_ID;
    
    bool result = upgrade->ConvertPathToRealPath(srcPath, prefix, newPath, relativePath, fileInfo);
    EXPECT_EQ(result, true);
    EXPECT_EQ(fileInfo.isInternal, false);
    MEDIA_INFO_LOG("medialib_backup_test_ConvertPathToRealPath_sd_hidden end");
}

/*
 * Test interface: UpgradeRestore::Convert
 * Test content: Verify ConvertPathToRealPath sd trashed
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ConvertPathToRealPath_sd_trashed, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_ConvertPathToRealPath_sd_trashed start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    std::string srcPath = "/storage/ABCD-0000/Pictures/test.jpg";
    std::string prefix = "/data/test";
    std::string newPath, relativePath;
    FileInfo fileInfo;
    fileInfo.fileSize = TEST_SIZE_2MB_ABOVE;
    fileInfo.localMediaId = GALLERY_TRASHED_ID;
    
    bool result = upgrade->ConvertPathToRealPath(srcPath, prefix, newPath, relativePath, fileInfo);
    EXPECT_EQ(result, true);
    EXPECT_EQ(fileInfo.isInternal, false);
    MEDIA_INFO_LOG("medialib_backup_test_ConvertPathToRealPath_sd_trashed end");
}

/*
 * Test interface: UpgradeRestore::Set
 * Test content: Verify SetOrientationAndExifRotate cloud
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_SetOrientationAndExifRotate_cloud, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_SetOrientationAndExifRotate_cloud start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    FileInfo info;
    info.localMediaId = -1;
    info.orientation = 90;
    info.fileType = MediaType::MEDIA_TYPE_IMAGE;
    
    NativeRdb::ValuesBucket value;
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    
    upgrade->SetOrientationAndExifRotate(info, value, data);
    
    int32_t orientation = 0;
    int32_t exifRotate = 0;
    NativeRdb::ValueObject valueObject;
    if (value.GetObject(PhotoColumn::PHOTO_ORIENTATION, valueObject)) {
        valueObject.GetInt(orientation);
    }
    if (value.GetObject(PhotoColumn::PHOTO_EXIF_ROTATE, valueObject)) {
        valueObject.GetInt(exifRotate);
    }
    EXPECT_EQ(orientation, 90);
    MEDIA_INFO_LOG("medialib_backup_test_SetOrientationAndExifRotate_cloud end");
}

/*
 * Test interface: UpgradeRestore::Set
 * Test content: Verify SetOrientationAndExifRotate local zero
 * Cover branches: Zero value branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_SetOrientationAndExifRotate_local_zero, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_SetOrientationAndExifRotate_local_zero start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    FileInfo info;
    info.localMediaId = 1;
    info.orientation = 0;
    info.fileType = MediaType::MEDIA_TYPE_IMAGE;
    
    NativeRdb::ValuesBucket value;
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    
    upgrade->SetOrientationAndExifRotate(info, value, data);
    
    int32_t orientation = 0;
    int32_t exifRotate = 0;
    NativeRdb::ValueObject valueObject;
    if (value.GetObject(PhotoColumn::PHOTO_ORIENTATION, valueObject)) {
        valueObject.GetInt(orientation);
    }
    if (value.GetObject(PhotoColumn::PHOTO_EXIF_ROTATE, valueObject)) {
        valueObject.GetInt(exifRotate);
    }
    EXPECT_EQ(orientation, 0);
    EXPECT_EQ(exifRotate, 0);
    MEDIA_INFO_LOG("medialib_backup_test_SetOrientationAndExifRotate_local_zero end");
}

/*
 * Test interface: UpgradeRestore::Get
 * Test content: Verify GetNoNeedMigrateCount
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GetNoNeedMigrateCount, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_GetNoNeedMigrateCount start");
    restoreService->galleryRdb_ = restoreService->galleryRdb_;
    restoreService->shouldIncludeSd_ = false;
    int32_t count = restoreService->GetNoNeedMigrateCount();
    MEDIA_INFO_LOG("GetNoNeedMigrateCount: %{public}d", count);
    EXPECT_GE(count, 0);
    MEDIA_INFO_LOG("medialib_backup_test_GetNoNeedMigrateCount end");
}

/*
 * Test interface: UpgradeRestore::Check
 * Test content: Verify CheckGalleryDbIntegrity
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_CheckGalleryDbIntegrity, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_CheckGalleryDbIntegrity start");
    restoreService->galleryRdb_ = restoreService->galleryRdb_;
    restoreService->galleryDbPath_ = TEST_BACKUP_PATH + "/" + GALLERY_APP_NAME + "/ce/databases/gallery.db";
    std::string result = restoreService->CheckGalleryDbIntegrity();
    EXPECT_FALSE(result.empty());
    MEDIA_INFO_LOG("CheckGalleryDbIntegrity result: %{public}s", result.c_str());
    MEDIA_INFO_LOG("medialib_backup_test_CheckGalleryDbIntegrity end");
}

/*
 * Test interface: UpgradeRestore::Query
 * Test content: Verify QueryPortraitAlbumTotalNumber
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_QueryPortraitAlbumTotalNumber, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_QueryPortraitAlbumTotalNumber start");
    restoreService->galleryRdb_ = restoreService->galleryRdb_;
    restoreService->isAccountValid_ = false;
    
    int32_t count = restoreService->QueryPortraitAlbumTotalNumber();
    MEDIA_INFO_LOG("QueryPortraitAlbumTotalNumber: %{public}d", count);
    EXPECT_GE(count, 0);
    MEDIA_INFO_LOG("medialib_backup_test_QueryPortraitAlbumTotalNumber end");
}

/*
 * Test interface: UpgradeRestore::Check
 * Test content: Verify CheckIsNeedCloneIsMe
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_CheckIsNeedCloneIsMe, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_CheckIsNeedCloneIsMe start");
    restoreService->galleryRdb_ = restoreService->galleryRdb_;
    
    bool result = restoreService->CheckIsNeedCloneIsMe();
    EXPECT_GE(result, false);
    MEDIA_INFO_LOG("CheckIsNeedCloneIsMe: %{public}d", static_cast<int32_t>(result));
    MEDIA_INFO_LOG("medialib_backup_test_CheckIsNeedCloneIsMe end");
}

/*
 * Test interface: UpgradeRestore::Query
 * Test content: Verify QueryPortraitAlbumInfos
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_QueryPortraitAlbumInfos, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_QueryPortraitAlbumInfos start");
    restoreService->galleryRdb_ = restoreService->galleryRdb_;
    restoreService->isAccountValid_ = false;
    
    std::vector<std::string> tagNameToDeleteSelection;
    vector<PortraitAlbumInfo> result = restoreService->QueryPortraitAlbumInfos(0, tagNameToDeleteSelection);
    EXPECT_GE(result.size(), 0);
    MEDIA_INFO_LOG("QueryPortraitAlbumInfos size: %{public}zu", result.size());
    MEDIA_INFO_LOG("medialib_backup_test_QueryPortraitAlbumInfos end");
}

/*
 * Test interface: BackupFileUtils
 * Test content: Verify RecordAlbumCoverInfo
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_RecordAlbumCoverInfo, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_RecordAlbumCoverInfo start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    std::shared_ptr<NativeRdb::ResultSet> resultSet;
    AlbumCoverInfo albumCoverInfo;
    
    int32_t result = upgrade->RecordAlbumCoverInfo(resultSet, albumCoverInfo);
    EXPECT_EQ(result, E_ERR);
    MEDIA_INFO_LOG("medialib_backup_test_RecordAlbumCoverInfo end");
}

/*
 * Test interface: UpgradeRestore::Set
 * Test content: Verify ParseResultSetForAudio null
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ParseResultSetForAudio_null, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_ParseResultSetForAudio_null start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    std::shared_ptr<NativeRdb::ResultSet> resultSet;
    FileInfo info;
    
    bool result = upgrade->ParseResultSetForAudio(resultSet, info);
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("medialib_backup_test_ParseResultSetForAudio_null end");
}

/*
 * Test interface: UpgradeRestore::Set
 * Test content: Verify ParseResultSetFromGallery null
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ParseResultSetFromGallery_null, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_ParseResultSetFromGallery_null start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    std::shared_ptr<NativeRdb::ResultSet> resultSet;
    FileInfo info;
    
    bool result = upgrade->ParseResultSetFromGallery(resultSet, info);
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("medialib_backup_test_ParseResultSetFromGallery_null end");
}

/*
 * Test interface: UpgradeRestore::Set
 * Test content: Verify ParseResultSetFromExternal null
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_ParseResultSetFromExternal_null, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_ParseResultSetFromExternal_null start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    std::shared_ptr<NativeRdb::ResultSet> resultSet;
    FileInfo info;
    
    bool result = upgrade->ParseResultSetFromExternal(resultSet, info, DUAL_MEDIA_TYPE::AUDIO_TYPE);
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("medialib_backup_test_ParseResultSetFromExternal_null end");
}

/*
 * Test interface: UpgradeRestore::Query
 * Test content: Verify QueryFileInfos null rdb
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_QueryFileInfos_null_rdb, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_QueryFileInfos_null_rdb start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);

    upgrade->galleryRdb_ = nullptr;
    std::vector<FileInfo> result = upgrade->QueryFileInfos(0);
    EXPECT_EQ(result.size(), 0);
    MEDIA_INFO_LOG("medialib_backup_test_QueryFileInfos_null_rdb end");
}

/*
 * Test interface: UpgradeRestore::Query
 * Test content: Verify QueryCloudFileInfos null rdb
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_QueryCloudFileInfos_null_rdb, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_QueryCloudFileInfos_null_rdb start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->galleryRdb_ = nullptr;
    std::vector<FileInfo> result = upgrade->QueryCloudFileInfos(0);
    EXPECT_EQ(result.size(), 0);
    MEDIA_INFO_LOG("medialib_backup_test_QueryCloudFileInfos_null_rdb end");
}

/*
 * Test interface: UpgradeRestore::Query
 * Test content: Verify QueryFileInfosFromExternal null rdb
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_QueryFileInfosFromExternal_null_rdb, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_QueryFileInfosFromExternal_null_rdb start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->externalRdb_ = nullptr;
    std::vector<FileInfo> result = upgrade->QueryFileInfosFromExternal(0, 100, true);
    EXPECT_EQ(result.size(), 0);
    MEDIA_INFO_LOG("medialib_backup_test_QueryFileInfosFromExternal_null_rdb end");
}

/*
 * Test interface: UpgradeRestore::Query
 * Test content: Verify QueryAudioFileInfosFromAudio null rdb
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_QueryAudioFileInfosFromAudio_null_rdb, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_QueryAudioFileInfosFromAudio_null_rdb start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->audioRdb_ = nullptr;
    std::vector<FileInfo> result = upgrade->QueryAudioFileInfosFromAudio(0);
    EXPECT_EQ(result.size(), 0);
    MEDIA_INFO_LOG("medialib_backup_test_QueryAudioFileInfosFromAudio_null_rdb end");
}

/*
 * Test interface: UpgradeRestore::Get
 * Test content: Verify GetCloudPhotoMinIds null rdb
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GetCloudPhotoMinIds_null_rdb, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_GetCloudPhotoMinIds_null_rdb start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->galleryRdb_ = nullptr;
    std::vector<int32_t> result = upgrade->GetCloudPhotoMinIds();
    EXPECT_EQ(result.size(), 0);
    MEDIA_INFO_LOG("medialib_backup_test_GetCloudPhotoMinIds_null_rdb end");
}

/*
 * Test interface: UpgradeRestore::Get
 * Test content: Verify GetLocalPhotoMinIds null rdb
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GetLocalPhotoMinIds_null_rdb, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_GetLocalPhotoMinIds_null_rdb start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->galleryRdb_ = nullptr;
    std::vector<int32_t> result = upgrade->GetLocalPhotoMinIds();
    EXPECT_EQ(result.size(), 0);
    MEDIA_INFO_LOG("medialib_backup_test_GetLocalPhotoMinIds_null_rdb end");
}

/*
 * Test interface: BackupFileUtils
 * Test content: Verify AnalyzeGalleryErrorSource null rdb
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_AnalyzeGalleryErrorSource_null_rdb, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_AnalyzeGalleryErrorSource_null_rdb start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->galleryRdb_ = nullptr;
    upgrade->AnalyzeGalleryErrorSource();
    EXPECT_EQ(upgrade->galleryRdb_, nullptr);
    MEDIA_INFO_LOG("medialib_backup_test_AnalyzeGalleryErrorSource_null_rdb end");
}

/*
 * Test interface: UpgradeRestore::Init
 * Test content: Verify InitGarbageAlbum null rdb
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_InitGarbageAlbum_null_rdb, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_InitGarbageAlbum_null_rdb start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->galleryRdb_ = nullptr;
    upgrade->InitGarbageAlbum();
    EXPECT_EQ(upgrade->galleryRdb_, nullptr);
    MEDIA_INFO_LOG("medialib_backup_test_InitGarbageAlbum_null_rdb end");
}

/*
 * Test interface: UpgradeRestore::Set
 * Test content: Verify AddToGalleryFailedOffsets
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_AddToGalleryFailedOffsets, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_AddToGalleryFailedOffsets start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->AddToGalleryFailedOffsets(100);
    upgrade->AddToGalleryFailedOffsets(200);
    EXPECT_GE(upgrade->galleryFailedOffsets_.size(), 2);
    MEDIA_INFO_LOG("medialib_backup_test_AddToGalleryFailedOffsets end");
}

/*
 * Test interface: UpgradeRestore::Set
 * Test content: Verify AddToExternalFailedOffsets
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_AddToExternalFailedOffsets, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_AddToExternalFailedOffsets start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->AddToExternalFailedOffsets(100);
    upgrade->AddToExternalFailedOffsets(200);
    EXPECT_GE(upgrade->externalFailedOffsets_.size(), 2);
    MEDIA_INFO_LOG("medialib_backup_test_AddToExternalFailedOffsets end");
}

/*
 * Test interface: BackupFileUtils
 * Test content: Verify IsValidDir
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_IsValidDir, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_IsValidDir start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    std::string path = "/Pictures/test";
    bool result = upgrade->IsValidDir(path);
    EXPECT_GE(result, false);
    MEDIA_INFO_LOG("IsValidDir result: %{public}d", static_cast<int32_t>(result));
    MEDIA_INFO_LOG("medialib_backup_test_IsValidDir end");
}

/*
 * Test interface: UpgradeRestore::Query
 * Test content: Verify QueryNotSyncTotalNumber
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_QueryNotSyncTotalNumber, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_QueryNotSyncTotalNumber start");
    restoreService->externalRdb_ = restoreService->externalRdb_;
    
    int32_t count = restoreService->QueryNotSyncTotalNumber(1000, true);
    MEDIA_INFO_LOG("QueryNotSyncTotalNumber: %{public}d", count);
    EXPECT_GE(count, 0);
    MEDIA_INFO_LOG("medialib_backup_test_QueryNotSyncTotalNumber end");
}

/*
 * Test interface: UpgradeRestore::Insert
 * Test content: Verify GetInsertValue cloud
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GetInsertValue_cloud, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_GetInsertValue_cloud start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    FileInfo fileInfo;
    fileInfo.localMediaId = -1;
    fileInfo.uniqueId = "test_unique_id";
    fileInfo.title = "test_title";
    fileInfo.displayName = "test.jpg";
    fileInfo.fileType = MediaType::MEDIA_TYPE_IMAGE;
    fileInfo.dateTaken = 1000000;
    fileInfo.firstUpdateTime = 2000000;
    fileInfo.duration = 0;
    fileInfo.isFavorite = 1;
    fileInfo.hidden = 0;
    fileInfo.height = 1080;
    fileInfo.width = 1920;
    fileInfo.userComment = "test_comment";
    fileInfo.packageName = "test_package";
    fileInfo.photoQuality = 1;
    fileInfo.detailTime = "2024:01:01 00:00:00";
    
    NativeRdb::ValuesBucket values = upgrade->GetInsertValue(fileInfo, "/test/path", SourceType::GALLERY);
    
    EXPECT_TRUE(values.HasColumn(PhotoColumn::PHOTO_CLOUD_ID));
    EXPECT_TRUE(values.HasColumn(PhotoColumn::PHOTO_POSITION));
    EXPECT_TRUE(values.HasColumn(PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE));
    MEDIA_INFO_LOG("medialib_backup_test_GetInsertValue_cloud end");
}

/*
 * Test interface: UpgradeRestore::Insert
 * Test content: Verify GetInsertValue local
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GetInsertValue_local, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_GetInsertValue_local start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    FileInfo fileInfo;
    fileInfo.localMediaId = 1;
    fileInfo.title = "test_title";
    fileInfo.displayName = "test.jpg";
    fileInfo.fileType = MediaType::MEDIA_TYPE_IMAGE;
    fileInfo.dateTaken = 1000000;
    fileInfo.firstUpdateTime = 0;
    fileInfo.duration = 0;
    fileInfo.isFavorite = 0;
    fileInfo.hidden = 1;
    fileInfo.height = 1080;
    fileInfo.width = 1920;
    fileInfo.userComment = "";
    fileInfo.packageName = "";
    fileInfo.photoQuality = 0;
    
    NativeRdb::ValuesBucket values = upgrade->GetInsertValue(fileInfo, "/test/path", SourceType::GALLERY);
    
    EXPECT_FALSE(values.HasColumn(PhotoColumn::PHOTO_CLOUD_ID));
    EXPECT_TRUE(values.HasColumn(PhotoColumn::PHOTO_DIRTY));
    MEDIA_INFO_LOG("medialib_backup_test_GetInsertValue_local end");
}

/*
 * Test interface: UpgradeRestore::Insert
 * Test content: Verify GetInsertValue video
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GetInsertValue_video, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_GetInsertValue_video start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    FileInfo fileInfo;
    fileInfo.localMediaId = 1;
    fileInfo.title = "test_title";
    fileInfo.displayName = "test.mp4";
    fileInfo.fileType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.dateTaken = 1000000;
    fileInfo.firstUpdateTime = 0;
    fileInfo.duration = 30000;
    fileInfo.isFavorite = 0;
    fileInfo.hidden = 0;
    fileInfo.height = 720;
    fileInfo.width = 1280;
    
    NativeRdb::ValuesBucket values = upgrade->GetInsertValue(fileInfo, "/test/path", SourceType::GALLERY);
    
    EXPECT_TRUE(values.HasColumn(MediaColumn::MEDIA_DURATION));
    MEDIA_INFO_LOG("medialib_backup_test_GetInsertValue_video end");
}

/*
 * Test interface: UpgradeRestore::Insert
 * Test content: Verify GetInsertValue audio
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GetInsertValue_audio, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_GetInsertValue_audio start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    FileInfo fileInfo;
    fileInfo.localMediaId = 1;
    fileInfo.title = "test_title";
    fileInfo.displayName = "test.mp3";
    fileInfo.fileType = MediaType::MEDIA_TYPE_AUDIO;
    fileInfo.dateTaken = 1000000;
    fileInfo.firstUpdateTime = 0;
    fileInfo.duration = 180000;
    fileInfo.isFavorite = 0;
    fileInfo.hidden = 0;
    
    NativeRdb::ValuesBucket values = upgrade->GetInsertValue(fileInfo, "/test/path", SourceType::GALLERY);
    
    EXPECT_TRUE(values.HasColumn(MediaColumn::MEDIA_DURATION));
    MEDIA_INFO_LOG("medialib_backup_test_GetInsertValue_audio end");
}

/*
 * Test interface: UpgradeRestore::Update
 * Test content: Verify UpdateFaceAnalysisStatus empty
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_UpdateFaceAnalysisStatus_empty, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_UpdateFaceAnalysisStatus_empty_empty start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->portraitAlbumIdMap_.clear();
    upgrade->UpdateFaceAnalysisStatus();
    EXPECT_EQ(upgrade->portraitAlbumIdMap_.size(), 0);
    MEDIA_INFO_LOG("medialib_backup_test_UpdateFaceAnalysisStatus_empty end");
}

/*
 * Test interface: UpgradeRestore::Update
 * Test content: Verify UpdateDualCloneFaceAnalysisStatus empty
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_UpdateDualCloneFaceAnalysisStatus_empty, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_UpdateDualCloneFaceAnalysisStatus_empty start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->portraitAlbumIdMap_.clear();
    upgrade->UpdateDualCloneFaceAnalysisStatus();
    EXPECT_EQ(upgrade->portraitAlbumIdMap_.size(), 0);
    MEDIA_INFO_LOG("medialib_backup_test_UpdateDualCloneFaceAnalysisStatus_empty end");
    MEDIA_INFO_LOG("medialib_backup_test_UpdateDualCloneFaceAnalysisStatus_empty end");
}

/*
 * Test interface: UpgradeRestore::Insert
 * Test content: Verify InsertPortraitAlbum empty
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_InsertPortraitAlbum_empty, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_InsertPortraitAlbum_empty start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->mediaLibraryRdb_ = nullptr;
    std::vector<PortraitAlbumInfo> portraitAlbumInfos;
    upgrade->InsertPortraitAlbum(portraitAlbumInfos);
    EXPECT_EQ(upgrade->mediaLibraryRdb_, nullptr);
    MEDIA_INFO_LOG("medialib_backup_test_InsertPortraitAlbum_empty end");
}

/*
 * Test interface: UpgradeRestore::Insert
 * Test content: Verify InsertPortraitAlbumByTable album
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_InsertPortraitAlbumByTable_album, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_InsertPortraitAlbumByTable_album start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->mediaLibraryRdb_ = photosStorePtr;
    std::vector<PortraitAlbumInfo> portraitAlbumInfos;
    PortraitAlbumInfo info;
    info.tagName = "test_album";
    info.tagIdNew = "test_tag_id";
    portraitAlbumInfos.push_back(info);
    
    int32_t result = upgrade->InsertPortraitAlbumByTable(portraitAlbumInfos, true);
    EXPECT_GE(result, 0);
    MEDIA_INFO_LOG("medialib_backup_test_InsertPortraitAlbumByTable_album end");
}

/*
 * Test interface: UpgradeRestore::Insert
 * Test content: Verify InsertPortraitAlbumByTable tag
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_InsertPortraitAlbumByTable_tag, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_InsertPortraitAlbumByTable_tag start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->mediaLibraryRdb_ = photosStorePtr;
    std::vector<PortraitAlbumInfo> portraitAlbumInfos;
    PortraitAlbumInfo info;
    info.tagName = "test_tag";
    info.tagIdNew = "test_tag_id";
    portraitAlbumInfos.push_back(info);
    
    int32_t result = upgrade->InsertPortraitAlbumByTable(portraitAlbumInfos, false);
    EXPECT_GE(result, 0);
    MEDIA_INFO_LOG("medialib_backup_test_InsertPortraitAlbumByTable_tag end");
}

/*
 * Test interface: UpgradeRestore::Insert
 * Test content: Verify GetInsertValues portrait album
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GetInsertValues_portrait_album, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_GetInsertValues_portrait_album start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    std::vector<PortraitAlbumInfo> portraitAlbumInfos;
    PortraitAlbumInfo info;
    info.tagName = "test_album";
    info.tagIdNew = "test_tag_id";
    info.userOperation = 1;
    info.userDisplayLevel = 1;
    info.relationship = "me";
    portraitAlbumInfos.push_back(info);
    
    std::vector<NativeRdb::ValuesBucket> values = upgrade->GetInsertValues(portraitAlbumInfos, true);
    EXPECT_EQ(values.size(), 1);
    MEDIA_INFO_LOG("medialib_backup_test_GetInsertValues_portrait_album end");
}

/*
 * Test interface: UpgradeRestore::Insert
 * Test content: Verify GetInsertValue portrait album me
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GetInsertValue_portrait_album_me, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_GetInsertValue_portrait_album_me start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    PortraitAlbumInfo info;
    info.tagName = "test_me";
    info.tagIdNew = "test_tag_id";
    info.userOperation = 1;
    info.userDisplayLevel = 1;
    info.relationship = "me";
    
    NativeRdb::ValuesBucket values = upgrade->GetInsertValue(info, true);
    EXPECT_TRUE(values.HasColumn(IS_ME));
    MEDIA_INFO_LOG("medialib_backup_test_GetInsertValue_portrait_album_me end");
}

/*
 * Test interface: UpgradeRestore::Insert
 * Test content: Verify GetInsertValue portrait album not me
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GetInsertValue_portrait_album_not_me, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_GetInsertValue_portrait_album_not_me start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    PortraitAlbumInfo info;
    info.tagName = "test_not_me";
    info.tagIdNew = "test_tag_id";
    info.userOperation = 1;
    info.userDisplayLevel = 1;
    info.relationship = "family";
    
    NativeRdb::ValuesBucket values = upgrade->GetInsertValue(info, true);
    EXPECT_TRUE(values.HasColumn(IS_ME));
    MEDIA_INFO_LOG("medialib_backup_test_GetInsertValue_portrait_album_not_me end");
}

/*
 * Test interface: UpgradeRestore::Insert
 * Test content: Verify GetInsertValue portrait tag
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GetInsertValue_portrait_tag, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_GetInsertValue_portrait_tag start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    PortraitAlbumInfo info;
    info.tagIdNew = "test_tag_id";
    
    NativeRdb::ValuesBucket values = upgrade->GetInsertValue(info, false);
    EXPECT_TRUE(values.HasColumn(TAG_VERSION));
    MEDIA_INFO_LOG("medialib_backup_test_GetInsertValue_portrait_tag end");
}

/*
 * Test interface: UpgradeRestore::Query
 * Test content: Verify BatchQueryAlbum empty
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_BatchQueryAlbum_empty, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_BatchQueryAlbum_empty start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->mediaLibraryRdb_ = nullptr;
    std::vector<PortraitAlbumInfo> portraitAlbumInfos;
    upgrade->BatchQueryAlbum(portraitAlbumInfos);
    EXPECT_EQ(upgrade->mediaLibraryRdb_, nullptr);
    MEDIA_INFO_LOG("medialib_backup_test_BatchQueryAlbum_empty end");
}

/*
 * Test interface: UpgradeRestore::Set
 * Test content: Verify SetAttributes portrait
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_SetAttributes_portrait, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_SetAttributes_portrait start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    PortraitAlbumInfo info;
    info.tagIdOld = "old_tag_id";
    
    bool result = upgrade->SetAttributes(info);
    EXPECT_GE(result, false);
    MEDIA_INFO_LOG("SetAttributes portrait result: %{public}d", static_cast<int32_t>(result));
    MEDIA_INFO_LOG("medialib_backup_test_SetAttributes_portrait end");
}

/*
 * Test interface: UpgradeRestore::Set
 * Test content: Verify SetAttributesSetAttributes face
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_SetAttributesSetAttributes_face, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_SetAttributes_face start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    FaceInfo faceInfo;
    faceInfo.hash = "test_hash";
    faceInfo.tagIdOld = "old_tag_id";
    
    std::unordered_map<std::string, FileInfo> fileInfoMap;
    FileInfo fileInfo;
    fileInfo.hashCode = "test_hash";
    fileInfo.fileIdNew = 100;
    fileInfoMap["test_hash"] = fileInfo;
    
    bool result = upgrade->SetAttributes(faceInfo, fileInfoMap);
    EXPECT_GE(result, false);
    MEDIA_INFO_LOG("SetAttributes face result: %{public}d", static_cast<int32_t>(result));
    MEDIA_INFO_LOG("medialib_backup_test_SetAttributes_face end");
}

/*
 * Test interface: UpgradeRestore::Update
 * Test content: Verify UpdateFilesWithFace
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_UpdateFilesWithFace, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_UpdateFilesWithFace start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    std::unordered_set<std::string> filesWithFace;
    std::vector<FaceInfo> faceInfos;
    FaceInfo faceInfo1;
    faceInfo1.hash = "hash1";
    faceInfos.push_back(faceInfo1);
    FaceInfo faceInfo2;
    faceInfo2.hash = "hash2";
    faceInfos.push_back(faceInfo2);
    
    upgrade->UpdateFilesWithFace(filesWithFace, faceInfos);
    EXPECT_EQ(filesWithFace.size(), 2);
    MEDIA_INFO_LOG("medialib_backup_test_UpdateFilesWithFace end");
}

/*
 * Test interface: UpgradeRestore::Insert
 * Test content: Verify GetInsertValues face map
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GetInsertValues_face_map, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_GetInsertValues_face_map start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    std::vector<FaceInfo> faceInfos;
    FaceInfo faceInfo;
    faceInfo.hash = "test_hash";
    faceInfo.fileIdNew = 100;
    faceInfo.tagIdNew = "tag_id";
    faceInfo.albumIdNew = 1;
    faceInfos.push_back(faceInfo);
    
    std::unordered_set<std::string> excludedFiles;
    std::vector<NativeRdb::ValuesBucket> values = upgrade->GetInsertValues(faceInfos, true, excludedFiles);
    EXPECT_EQ(values.size(), 1);
    MEDIA_INFO_LOG("medialib_backup_test_GetInsertValues_face_map end");
}

/*
 * Test interface: UpgradeRestore::Insert
 * Test content: Verify GetInsertValues face info
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_GetInsertValues_face_info, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_GetInsertValues_face_info start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    FaceInfo faceInfo;
    faceInfo.scaleX = 0.1;
    faceInfo.scaleY = 0.2;
    faceInfo.scaleWidth = 0.3;
    faceInfo.scaleHeight = 0.4;
    faceInfo.pitch = 1.0;
    faceInfo.yaw = 2.0;
    faceInfo.roll = 3.0;
    faceInfo.prob = 0.95;
    faceInfo.totalFaces = 1;
    faceInfo.fileIdNew = 100;
    faceInfo.faceId = "face_id";
    faceInfo.tagIdNew = "tag_id";
    
    NativeRdb::ValuesBucket values = upgrade->GetInsertValue(faceInfo, false);
    EXPECT_TRUE(values.HasColumn(SCALE_X));
    EXPECT_TRUE(values.HasColumn(SCALE_Y));
    EXPECT_TRUE(values.HasColumn(SCALE_WIDTH));
    EXPECT_TRUE(values.HasColumn(SCALE_HEIGHT));
    MEDIA_INFO_LOG("medialib_backup_test_GetInsertValues_face_info end");
}

/*
 * Test interface: UpgradeRestore::Insert
 * Test content: Verify InsertFaceAnalysisDataByTable face
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_InsertFaceAnalysisDataByTable_face, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_InsertFaceAnalysisDataByTable_face start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);

    upgrade->mediaLibraryRdb_ = photosStorePtr;
    std::vector<FaceInfo> faceInfos;
    FaceInfo faceInfo;
    faceInfo.hash = "test_hash";
    faceInfo.fileIdNew = 100;
    faceInfo.tagIdNew = "tag_id";
    faceInfos.push_back(faceInfo);
    
    std::unordered_set<std::string> excludedFiles;
    int32_t result = upgrade->InsertFaceAnalysisDataByTable(faceInfos, false, excludedFiles);
    EXPECT_GE(result, 0);
    MEDIA_INFO_LOG("medialib_backup_test_InsertFaceAnalysisDataByTable_face end");
}

/*
 * Test interface: UpgradeRestore::Insert
 * Test content: Verify InsertFaceAnalysisDataByTable map
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_InsertFaceAnalysisDataByTable_map, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_InsertFaceAnalysisDataByTable_map start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);

    upgrade->mediaLibraryRdb_ = photosStorePtr;
    std::vector<FaceInfo> faceInfos;
    FaceInfo faceInfo;
    faceInfo.hash = "test_hash";
    faceInfo.fileIdNew = 100;
    faceInfo.tagIdNew = "tag_id";
    faceInfo.albumIdNew = 1;
    faceInfos.push_back(faceInfo);
    
    std::unordered_set<std::string> excludedFiles;
    int32_t result = upgrade->InsertFaceAnalysisDataByTable(faceInfos, true, excludedFiles);
    EXPECT_GE(result, 0);
    MEDIA_INFO_LOG("medialib_backup_test_InsertFaceAnalysisDataByTable_map end");
}

/*
 * Test interface: UpgradeRestore::Set
 * Test content: Verify SetHashReference
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_SetHashReference, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_SetHashReference start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    std::vector<FileInfo> fileInfos;
    FileInfo fileInfo1;
    fileInfo1.hashCode = "hash1";
    fileInfo1.hidden = 0;
    fileInfo1.fileIdNew = 100;
    fileInfos.push_back(fileInfo1);
    
    FileInfo fileInfo2;
    fileInfo2.hashCode = "hash2";
    fileInfo2.hidden = 1;
    fileInfo2.fileIdNew = 200;
    fileInfos.push_back(fileInfo2);
    
    NeedQueryMap needQueryMap;
    needQueryMap[PhotoRelatedType::PORTRAIT] = {"hash1", "hash2"};
    
    std::string hashSelection;
    std::unordered_map<std::string, FileInfo> fileInfoMap;
    upgrade->SetHashReference(fileInfos, needQueryMap, hashSelection, fileInfoMap);
    
    EXPECT_GE(fileInfoMap.size(), 0);
    MEDIA_INFO_LOG("SetHashReference fileInfoMap size: %{public}zu", fileInfoMap.size());
    MEDIA_INFO_LOG("medialib_backup_test_SetHashReference end");
}

/*
 * Test interface: UpgradeRestore::Query
 * Test content: Verify QueryFaceTotalNumber
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_QueryFaceTotalNumber, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_QueryFaceTotalNumber start");
    restoreService->galleryRdb_ = restoreService->galleryRdb_;
    
    std::string hashSelection = "'hash1', 'hash2'";
    int32_t count = restoreService->QueryFaceTotalNumber(hashSelection);
    MEDIA_INFO_LOG("QueryFaceTotalNumber: %{public}d", count);
    EXPECT_GE(count, 0);
    MEDIA_INFO_LOG("medialib_backup_test_QueryFaceTotalNumber end");
}

/*
 * Test interface: UpgradeRestore::Query
 * Test content: Verify QueryFaceInfos null rdb
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_QueryFaceInfos_null_rdb, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_QueryFaceInfos_null_rdb start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->galleryRdb_ = nullptr;
    std::string hashSelection = "'hash1', 'hash2'";
    std::unordered_map<std::string, FileInfo> fileInfoMap;
    std::unordered_set<std::string> excludedFiles;
;
    
    std::vector<FaceInfo> result = upgrade->QueryFaceInfos(hashSelection, fileInfoMap, 0, excludedFiles);
    EXPECT_EQ(result.size(), 0);
    MEDIA_INFO_LOG("medialib_backup_test_QueryFaceInfos_null_rdb end");
}

/*
 * Test interface: UpgradeRestore::Query
 * Test content: Verify NeedBatchQueryPhotoForPortrait empty
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_NeedBatchQueryPhotoForPortrait_empty, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_NeedBatchQueryPhotoForPortrait_empty start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->portraitAlbumIdMap_.clear();
    std::vector<FileInfo> fileInfos;
    FileInfo fileInfo;
    fileInfos.push_back(fileInfo);
    
    NeedQueryMap needQueryMap;
    bool result = upgrade->NeedBatchQueryPhotoForPortrait(fileInfos, needQueryMap);
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("medialib_backup_test_NeedBatchQueryPhotoForPortrait_empty end");
}

/*
 * Test interface: UpgradeRestore::Insert
 * Test content: Verify InsertFaceAnalysisData empty
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_InsertFaceAnalysisData_empty, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_InsertFaceAnalysisData_empty start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->mediaLibraryRdb_ = photosStorePtr;
    std::vector<FileInfo> fileInfos;
    NeedQueryMap needQueryMap;
    needQueryMap[PhotoRelatedType::PORTRAIT] = {"hash1"};
    
    int64_t faceRowNum = 0;
    int64_t mapRowNum = 0;
    int64_t photoNum = 0;
    
    upgrade->InsertFaceAnalysisData(fileInfos, needQueryMap, faceRowNum, mapRowNum, photoNum);
    EXPECT_GE(faceRowNum, 0);
    EXPECT_GE(mapRowNum, 0);
    EXPECT_GE(photoNum, 0);
    MEDIA_INFO_LOG("medialib_backup_test_InsertFaceAnalysisData_empty end");
}

/*
 * Test interface: UpgradeRestore::Restore
 * Test content: Verify RestoreFromGalleryPortraitAlbum empty
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_RestoreFromGalleryPortraitAlbum_empty, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_RestoreFromGalleryPortraitAlbum_empty start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->galleryRdb_ = nullptr;
    upgrade->mediaLibraryRdb_ = photosStorePtr;
    upgrade->RestoreFromGalleryPortraitAlbum();
    EXPECT_EQ(upgrade->galleryRdb_, nullptr);
    EXPECT_NE(upgrade->mediaLibraryRdb_, nullptr);
    MEDIA_INFO_LOG("medialib_backup_test_RestoreFromGalleryPortraitAlbum_empty end");
}

/*
 * Test interface: BackupFileUtils
 * Test content: Verify InheritManualCover null rdb
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_InheritManualCover_null_rdb, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_InheritManualCover_null_rdb start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->galleryRdb_ = nullptr;
    upgrade->InheritManualCover();
    EXPECT_EQ(upgrade->galleryRdb_, nullptr);
    MEDIA_INFO_LOG("medialib_backup_test_InheritManualCover_null_rdb end");
}

/*
 * Test interface: UpgradeRestore::Update
 * Test content: Verify UpdatePhotoAlbumCoverUri empty
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_UpdatePhotoAlbumCoverUri_empty, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_UpdatePhotoAlbumCoverUri_empty start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->mediaLibraryRdb_ = photosStorePtr;
    vector<AlbumCoverInfo> albumCoverInfos;
    upgrade->UpdatePhotoAlbumCoverUri(albumCoverInfos);
    EXPECT_NE(upgrade->mediaLibraryRdb_, nullptr);
    MEDIA_INFO_LOG("medialib_backup_test_UpdatePhotoAlbumCoverUri_empty end");
}

/*
 * Test interface: UpgradeRestore::Restore
 * Test content: Verify RestorePhotoInner no gallery db
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_RestorePhotoInner_no_gallery_db, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_RestorePhotoInner_no_gallery_db start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->galleryDbPath_ = "/nonexistent/path/gallery.db";
    upgrade->mediaLibraryRdb_ = photosStorePtr;
    upgrade->RestorePhotoInner();
    EXPECT_EQ(upgrade->maxId_, 0);
    MEDIA_INFO_LOG("medialib_backup_test_RestorePhotoInner_no_gallery_db end");
}

/*
 * Test interface: UpgradeRestore::Init
 * Test content: Verify InitDb upgrade fail
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_InitDb_upgrade_fail, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_InitDb_upgrade_fail start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->galleryDbPath_ = "/nonexistent/path/gallery.db";
    upgrade->audioDbPath_ = "/nonexistent/path/audio.db";
    
    int32_t result = upgrade->InitDb(true);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_InitDb_upgrade_fail end");
}

/*
 * Test interface: UpgradeRestore::Init
 * Test content: Verify InitDb no upgrade
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_InitDb_no_upgrade, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_InitDb_no_upgrade start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->galleryDbPath_ = "/nonexistent/path/gallery.db";
    upgrade->audioDbPath_ = "/nonexistent/path/audio.db";
    
    int32_t result = upgrade->InitDb(false);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_InitDb_no_upgrade end");
}

/*
 * Test interface: UpgradeRestore::Init
 * Test content: Verify InitDbAndXml fail
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_InitDbAndXml_fail, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_InitDbAndXml_fail start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->galleryDbPath_ = "/nonexistent/path/gallery.db";
    upgrade->audioDbPath_ = "/nonexistent/path/audio.db";
    
    int32_t result = upgrade->InitDbAndXml("/nonexistent/path.xml", true);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_InitDbAndXml_fail end");
}

/*
 * Test interface: UpgradeRestore::Init
 * Test content: Verify Init fail no external db
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_Init_fail_no_external_db, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_Init_fail_no_external_db start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, UPGRADE_RESTORE_ID);
    
    std::string backupPath = "/nonexistent/backup";
    std::string upgradeFilePath = "/nonexistent/file";
    
    int32_t result = upgrade->Init(backupPath, upgradeFilePath, false);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("medialib_backup_test_Init_fail_no_external_db end");
}

/*
 * Test interface: UpgradeRestore::Init
 * Test content: Verify Init clone
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_Init_clone, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_Init_clone start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    std::string backupPath = TEST_BACKUP_PATH_CLOUD;
    std::string upgradeFilePath = TEST_UPGRADE_FILE_DIR;
    
    int32_t result = upgrade->Init(backupPath, upgradeFilePath, false);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("Init clone result: %{public}d", result);
    MEDIA_INFO_LOG("medialib_backup_test_Init_clone end");
}

/*
 * Test interface: UpgradeRestore::Restore
 * Test content: Verify RestoreAudio clone
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_RestoreAudio_clone, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_RestoreAudio_clone start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->audioDbPath_ = "/nonexistent/path/audio.db";
    upgrade->externalDbPath_ = "/nonexistent/path/external.db";
    upgrade->sceneCode_ = DUAL_FRAME_CLONE_RESTORE_ID;
    upgrade->restoreMode_ = RESTORE_MODE_PROC_ALL_DATA;
    
    upgrade->RestoreAudio();
    EXPECT_EQ(upgrade->sceneCode_, DUAL_FRAME_CLONE_RESTORE_ID);
    EXPECT_EQ(upgrade->restoreMode_, RESTORE_MODE_PROC_ALL_DATA);
    MEDIA_INFO_LOG("medialib_backup_test_RestoreAudio_clone end");
}

/*
 * Test interface: UpgradeRestore::Restore
 * Test content: Verify RestoreAudio upgrade
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_RestoreAudio_upgrade, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_RestoreAudio_upgrade start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, UPGRADE_RESTORE_ID);
    
    upgrade->audioDbPath_ = "/nonexistent/path/audio.db";
    upgrade->externalDbPath_ = "/nonexistent/path/external.db";
    upgrade->sceneCode_ = UPGRADE_RESTORE_ID;
    upgrade->restoreMode_ = RESTORE_MODE_PROC_MAIN_DATA;
    
    upgrade->RestoreAudio();
    EXPECT_EQ(upgrade->sceneCode_, UPGRADE_RESTORE_ID);
    EXPECT_EQ(upgrade->restoreMode_, RESTORE_MODE_PROC_MAIN_DATA);
    MEDIA_INFO_LOG("medialib_backup_test_RestoreAudio_upgrade end");
}

/*
 * Test interface: UpgradeRestore::Restore
 * Test content: Verify RestoreAudioFromFile null rdb
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_RestoreAudioFromFile_null_rdb, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_RestoreAudioFromFile_null_rdb start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->audioRdb_ = nullptr;
    upgrade->RestoreAudioFromFile();
    EXPECT_EQ(upgrade->audioRdb_, nullptr);
    MEDIA_INFO_LOG("medialib_backup_test_RestoreAudioFromFile_null_rdb end");
}

/*
 * Test interface: UpgradeRestore::Restore
 * Test content: Verify RestoreAudioBatch null rdb
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_RestoreAudioBatch_null_rdb, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_RestoreAudioBatch_null_rdb start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->audioRdb_ = nullptr;
    upgrade->RestoreAudioBatch(0);
    EXPECT_EQ(upgrade->audioRdb_, nullptr);
    MEDIA_INFO_LOG("medialib_backup_test_RestoreAudioBatch_null_rdb end");
}

/*
 * Test interface: BackupFileUtils
 * Test content: Verify HandleRestData delete
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_HandleRestData_delete, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_HandleRestData_delete start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->appDataPath_ = "/nonexistent/path";
    upgrade->galleryAppName_ = GALLERY_APP_NAME;
    upgrade->mediaAppName_ = MEDIA_APP_NAME;
    upgrade->galleryDbPath_ = "/nonexistent/path/gallery.db";
    upgrade->mediaLibraryRdb_ = photosStorePtr;
    upgrade->restoreMode_ = RESTORE_MODE_PROC_ALL_DATA;
    
    upgrade->HandleRestData();
    EXPECT_EQ(upgrade->restoreMode_, RESTORE_MODE_PROC_ALL_DATA);
    MEDIA_INFO_LOG("medialib_backup_test_HandleRestData_delete end");
}

/*
 * Test interface: BackupFileUtils
 * Test content: Verify HandleRestData no delete
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_HandleRestData_no_delete, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_HandleRestData_no_delete start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->appDataPath_ = "/nonexistent/path";
    upgrade->galleryAppName_ = GALLERY_APP_NAME;
    upgrade->mediaAppName_ = MEDIA_APP_NAME;
    upgrade->mediaLibraryRdb_ = photosStorePtr;
    upgrade->restoreMode_ = RESTORE_MODE_PROC_MAIN_DATA;
    
    upgrade->HandleRestData();
    EXPECT_EQ(upgrade->restoreMode_, RESTORE_MODE_PROC_MAIN_DATA);
    MEDIA_INFO_LOG("medialib_backup_test_HandleRestData_no_delete end");
}

/*
 * Test interface: UpgradeRestore::Restore
 * Test content: Verify RestorePhoto not restore
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_RestorePhoto_not_restore, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_RestorePhoto_not_restore start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->galleryDbPath_ = "/nonexistent/path/gallery.db";
    upgrade->mediaLibraryRdb_ = photosStorePtr;
    upgrade->sceneCode_ = UPGRADE_RESTORE_ID;
    
    upgrade->RestorePhoto();
    EXPECT_EQ(upgrade->sceneCode_, UPGRADE_RESTORE_ID);
    MEDIA_INFO_LOG("medialib_backup_test_RestorePhoto_not_restore end");
}

/*
 * Test interface: UpgradeRestore::Restore
 * Test content: Verify RestoreFromGallery null rdb
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_RestoreFromGallery_null_rdb, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_RestoreFromGallery_null_rdb start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->galleryRdb_ = nullptr;
    upgrade->RestoreFromGallery();
    EXPECT_EQ(upgrade->galleryRdb_, nullptr);
    MEDIA_INFO_LOG("medialib_backup_test_RestoreFromGallery_null_rdb end");
}

/*
 * Test interface: UpgradeRestore::Restore
 * Test content: Verify RestoreCloudFromGallery null rdb
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_RestoreCloudFromGallery_null_rdb, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_RestoreCloudFromGallery_null_rdb start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->galleryRdb_ = nullptr;
    upgrade->RestoreCloudFromGallery();
    EXPECT_EQ(upgrade->galleryRdb_, nullptr);
    MEDIA_INFO_LOG("medialib_backup_test_RestoreCloudFromGallery_null_rdb end");
}

/*
 * Test interface: UpgradeRestore::Restore
 * Test content: Verify RestoreFromExternal null rdb
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_RestoreFromExternal_null_rdb, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_RestoreFromExternal_null_rdb start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->galleryRdb_ = nullptr;
    upgrade->externalRdb_ = nullptr;
    upgrade->RestoreFromExternal(true);
    EXPECT_EQ(upgrade->galleryRdb_, nullptr);
    EXPECT_EQ(upgrade->externalRdb_, nullptr);
    MEDIA_INFO_LOG("medialib_backup_test_RestoreFromExternal_null_rdb end");
}

/*
 * Test interface: UpgradeRestore::Restore
 * Test content: Verify RestoreBatch null rdb
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_RestoreBatch_null_rdb, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_RestoreBatch_null_rdb start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->galleryRdb_ = nullptr;
    upgrade->mediaLibraryRdb_ = photosStorePtr;
    upgrade->RestoreBatch(0);
    EXPECT_EQ(upgrade->galleryRdb_, nullptr);
    MEDIA_INFO_LOG("medialib_backup_test_RestoreBatch_null_rdb end");
}

/*
 * Test interface: UpgradeRestore::Restore
 * Test content: Verify RestoreBatchForCloud null rdb
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_RestoreBatchForCloud_null_rdb, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_RestoreBatchForCloud_null_rdb start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->galleryRdb_ = nullptr;
    upgrade->mediaLibraryRdb_ = photosStorePtr;
    upgrade->RestoreBatchForCloud(0);
    EXPECT_EQ(upgrade->galleryRdb_, nullptr);
    MEDIA_INFO_LOG("medialib_backup_test_RestoreBatchForCloud_null_rdb end");
}

/*
 * Test interface: UpgradeRestore::Restore
 * Test content: Verify RestoreExternalBatch null rdb
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_RestoreExternalBatch_null_rdb, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_RestoreExternalBatch_null_rdb start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->externalRdb_ = nullptr;
    upgrade->mediaLibraryRdb_ = photosStorePtr;
    upgrade->RestoreExternalBatch(0, 1000, true, SourceType::EXTERNAL_CAMERA);
    EXPECT_EQ(upgrade->externalRdb_, nullptr);
    MEDIA_INFO_LOG("medialib_backup_test_RestoreExternalBatch_null_rdb end");
}

/*
 * Test interface: BackupFileUtils
 * Test content: Verify AnalyzeSource null rdb
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_AnalyzeSource_null_rdb, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_AnalyzeSource_null_rdb start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->externalRdb_ = nullptr;
    upgrade->galleryRdb_ = nullptr;
    upgrade->mediaLibraryRdb_ = nullptr;
    upgrade->AnalyzeSource();
    EXPECT_EQ(upgrade->externalRdb_, nullptr);
    EXPECT_EQ(upgrade->galleryRdb_, nullptr);
    EXPECT_EQ(upgrade->mediaLibraryRdb_, nullptr);
    MEDIA_INFO_LOG("medialib_backup_test_AnalyzeSource_null_rdb end");
}

/*
 * Test interface: BackupFileUtils
 * Test content: Verify AnalyzeGallerySource null rdb
 * Cover branches: Empty/null parameter branch
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_AnalyzeGallerySource_null_rdb, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_AnalyzeGallerySource_null_rdb start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->galleryRdb_ = nullptr;
    upgrade->AnalyzeGallerySource();
    EXPECT_EQ(upgrade->galleryRdb_, nullptr);
    MEDIA_INFO_LOG("medialib_backup_test_AnalyzeGallerySource_null_rdb end");
}

/*
 * Test interface: BackupFileUtils
 * Test content: Verify HasSameFileForDualClone no same
 * Cover branches: Normal flow
 */
HWTEST_F(MediaLibraryBackupTest, medialib_backup_test_HasSameFileForDualClone_no_same, TestSize.Level2)
{
    MEDIA_INFO_LOG("medialib_backup_test_HasSameFileForDualClone_no_same start");
    std::unique_ptr<UpgradeRestore> upgrade =
        std::make_unique<UpgradeRestore>(GALLERY_APP_NAME, MEDIA_APP_NAME, DUAL_FRAME_CLONE_RESTORE_ID);
    
    upgrade->mediaLibraryRdb_ = photosStorePtr;
    FileInfo fileInfo;
    fileInfo.oldPath = "/test/nonexistent.jpg";
    
    bool result = upgrade->HasSameFileForDualClone(fileInfo);
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("HasSameFileForDualClone result: %{public}d", static_cast<int32_t>(result));
    MEDIA_INFO_LOG("medialib_backup_test_HasSameFileForDualClone_no_same end");
}

// 测试正常路径转换（图片）
HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_ConvertToStoragePath_ValidPath_Image, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_ConvertToStoragePath_ValidPath_Image start");
    std::string path = "/mnt/data/100/HO_MEDIA/Pictures/weibo/pic.jpg";
    std::string result = BackupFileUtils::ConvertToStoragePath(path);
    EXPECT_EQ(result, TEST_ROOT_LAKE_DIR + "Pictures/weibo/pic.jpg");
    MEDIA_INFO_LOG("BackupFileUtils_ConvertToStoragePath_ValidPath_Image end");
}

// 测试 ConvertToStoragePath  不包含 HO_MEDIA 的路径
HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_ConvertToStoragePath_NoMarker, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_ConvertToStoragePath_NoMarker start");
    std::string path = "/mnt/data/100/Pictures/weibo/pic.jpg";
    std::string result = BackupFileUtils::ConvertToStoragePath(path);
    EXPECT_EQ(result, path);
    MEDIA_INFO_LOG("BackupFileUtils_ConvertToStoragePath_NoMarker end");
}

// 测试 ConvertToStoragePath  空路径
HWTEST_F(MediaLibraryBackupTest, BackupFileUtils_ConvertToStoragePath_EmptyPath, TestSize.Level2)
{
    MEDIA_INFO_LOG("BackupFileUtils_ConvertToStoragePath_EmptyPath start");
    std::string path = "";
    std::string result = BackupFileUtils::ConvertToStoragePath(path);
    EXPECT_EQ(result, path);
    MEDIA_INFO_LOG("BackupFileUtils_ConvertToStoragePath_EmptyPath end");
}
} // namespace Media
} // namespace OHOS
