/*
 * Copyright (C) 2024-2025 Huawei Device Co., Ltd.
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
 
#define MLOG_TAG "CloudCleanerTest"
 
#include "cloud_cleaner_test.h"
#include "cloud_data_cleaner.h"
#include "media_log.h"
#include "medialibrary_unittest_utils.h"
#include "rdb_predicates.h"
#include "result_set_utils.h"
#include <sys/stat.h>
 
using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::Media::TestUtils;
 
namespace OHOS {
namespace Media {
 
static constexpr int32_t SLEEP_TWO_SECONDS = 2;
 
// ===================== Test Fixture Implementation =====================
 
void CloudDataCleanerTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("CloudDataCleanerTest SetUpTestCase");
    struct stat st;
    if (stat(TEST_DIR, &st) != 0) {
        mkdir(TEST_DIR, 0755);
    }
}
 
void CloudDataCleanerTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("CloudDataCleanerTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_TWO_SECONDS));
}
 
void CloudDataCleanerTest::SetUp()
{
    MEDIA_INFO_LOG("CloudDataCleanerTest SetUp");
}
 
void CloudDataCleanerTest::TearDown()
{
    MEDIA_INFO_LOG("CloudDataCleanerTest TearDown");
    CloseAndDeleteDatabase(CLOUD_CLEANER_TEST_DB);
}
 
std::shared_ptr<RdbStore> CloudDataCleanerTest::CreateTestDatabase(const string& dbName)
{
    string dbPath = string(TEST_DIR) + dbName;
    RdbStoreConfig config(dbName);
    config.SetPath(dbPath);
    config.SetSecurityLevel(SecurityLevel::S3);
 
    class TestCallback : public RdbOpenCallback {
    public:
        int32_t OnCreate(RdbStore& rdb) override { return E_OK; }
        int32_t OnUpgrade(RdbStore& rdb, int32_t oldVersion, int32_t newVersion) override { return E_OK; }
    };
 
    int32_t errCode = 0;
    TestCallback callback;
    auto store = RdbHelper::GetRdbStore(config, 1, callback, errCode);
    if (store == nullptr) {
        MEDIA_ERR_LOG("Failed to create test database: %{public}s, errCode=%{public}d", dbName.c_str(), errCode);
    }
    return store;
}
 
void CloudDataCleanerTest::CloseAndDeleteDatabase(const string& dbName)
{
    string dbPath = string(TEST_DIR) + dbName;
    unlink(dbPath.c_str());
    string walPath = dbPath + "-wal";
    unlink(walPath.c_str());
    string shmPath = dbPath + "-shm";
    unlink(shmPath.c_str());
}
 
int32_t CloudDataCleanerTest::CreatePhotosTable(shared_ptr<RdbStore> store)
{
    const string CREATE_PHOTOS_TABLE =
        "CREATE TABLE IF NOT EXISTS Photos ("
        "    file_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "    data TEXT, "
        "    display_name TEXT, "
        "    position INT DEFAULT 1, "
        "    clean_flag INT DEFAULT 0, "
        "    dirty INT DEFAULT 0, "
        "    cloud_id TEXT, "
        "    cloud_version BIGINT DEFAULT 0, "
        "    real_lcd_visit_time BIGINT DEFAULT 0, "
        "    south_device_type INT DEFAULT 0, "
        "    owner_album_id INT DEFAULT 0, "
        "    associate_file_id INT DEFAULT 0, "
        "    date_added BIGINT DEFAULT 0, "
        "    date_modified BIGINT DEFAULT 0, "
        "    media_type INT DEFAULT 0, "
        "    size BIGINT DEFAULT 0, "
        "    title TEXT DEFAULT '', "
        "    mime_type TEXT DEFAULT '');";
 
    return ExecuteSql(store, CREATE_PHOTOS_TABLE);
}
 
int32_t CloudDataCleanerTest::CreatePhotoAlbumTable(shared_ptr<RdbStore> store)
{
    const string CREATE_PHOTO_ALBUM_TABLE =
        "CREATE TABLE IF NOT EXISTS PhotoAlbum ("
        "    album_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "    album_is_local INT DEFAULT 0, "
        "    dirty INT DEFAULT 0, "
        "    cloud_id TEXT, "
        "    upload_status INT DEFAULT 0, "
        "    lpath TEXT, "
        "    album_type INT DEFAULT 0, "
        "    cover_uri TEXT DEFAULT '', "
        "    album_name TEXT DEFAULT '', "
        "    date_added BIGINT DEFAULT 0, "
        "    date_modified BIGINT DEFAULT 0);";
 
    return ExecuteSql(store, CREATE_PHOTO_ALBUM_TABLE);
}
 
int32_t CloudDataCleanerTest::CreateBackupTable(shared_ptr<RdbStore> store)
{
    const string CREATE_BACKUP_TABLE =
        "CREATE TABLE IF NOT EXISTS PhotosAlbumBackupForSaveAnalysisData ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "    data TEXT);";
 
    return ExecuteSql(store, CREATE_BACKUP_TABLE);
}
 
int32_t CloudDataCleanerTest::ExecuteSql(shared_ptr<RdbStore> store, const string& sql)
{
    if (store == nullptr) {
        MEDIA_ERR_LOG("ExecuteSql: store is null");
        return E_ERR;
    }
    return store->ExecuteSql(sql, {});
}
 
// ===================== Test Cases =====================
 
/*
 * Test Case Description:
 * - Coverage: Clean pure cloud data (position=2, display_name != 'cloud_media_asset_deleted')
 * - Branch point: UpdateCloudMediaAssets - batch update condition
 * - Trigger condition: Insert cloud-only records with clean_flag=0, dirty=0
 * - Business validation: Records should be updated with clean_flag=1, dirty=-1,
 * - display_name='cloud_media_asset_deleted', cloud_id=NULL
 */
HWTEST_F(CloudDataCleanerTest, CleanCloudData_PureCloudData_Test, TestSize.Level1)
{
    auto store = CreateTestDatabase(CLOUD_CLEANER_TEST_DB);
    ASSERT_NE(store, nullptr);
 
    CreatePhotosTable(store);
 
    // Insert pure cloud data one row at a time (to avoid multi‑statement issues)
    const string INSERT1 = "INSERT INTO Photos (display_name, position, clean_flag, dirty, cloud_id, cloud_version, "
                           "real_lcd_visit_time, south_device_type, owner_album_id) "
                           "VALUES ('test_cloud_1', 2, 0, 0, 'cloud_id_1', 1, 100, 0, 1);";
    const string INSERT2 = "INSERT INTO Photos (display_name, position, clean_flag, dirty, cloud_id, cloud_version, "
                           "real_lcd_visit_time, south_device_type, owner_album_id) "
                           "VALUES ('test_cloud_2', 2, 0, 0, 'cloud_id_2', 2, 200, 0, 2);";
    EXPECT_EQ(ExecuteSql(store, INSERT1), E_OK);
    EXPECT_EQ(ExecuteSql(store, INSERT2), E_OK);
 
    // Verify initial state
    auto rs = store->QuerySql("SELECT COUNT(*) FROM Photos WHERE position=2 AND "
        "display_name != 'cloud_media_asset_deleted';");
    ASSERT_NE(rs, nullptr);
    int64_t count = 0;
    if (rs->GoToNextRow() == E_OK) {
        rs->GetLong(0, count);
    }
    rs->Close();
    EXPECT_EQ(count, 2);
 
    CloudDataCleaner cleaner(store);
    EXPECT_TRUE(cleaner.CleanCloudData());
 
    // Check that pure cloud data was updated
    rs = store->QuerySql("SELECT clean_flag, dirty, display_name, "
        "cloud_id FROM Photos WHERE position=2 ORDER BY file_id;");
    ASSERT_NE(rs, nullptr);
    int idx = 0;
    while (rs->GoToNextRow() == E_OK && idx < 2) {
        int64_t cleanFlag = 0;
        int32_t dirty = 0;
        string displayName, cloudId;
        rs->GetLong(0, cleanFlag);
        rs->GetInt(1, dirty);
        rs->GetString(2, displayName);
        rs->GetString(3, cloudId);
 
        EXPECT_EQ(cleanFlag, 1);
        EXPECT_EQ(dirty, -1);
        EXPECT_EQ(displayName, "cloud_media_asset_deleted");
        EXPECT_TRUE(cloudId.empty());
        idx++;
    }
    rs->Close();
    EXPECT_EQ(idx, 2);
}
 
/*
 * Test Case Description:
 * - Coverage: Clean local-and-cloud data (position=3, has cloud_id and cloud_version)
 * - Branch point: UpdateBothLocalAndCloudAssets - convert to local-only
 * - Trigger condition: Insert records with position=3 and cloud_id != NULL
 * - Business validation: Records should be updated with position=1, cloud_id=NULL, cloud_version=0, south_device_type=0
 */
HWTEST_F(CloudDataCleanerTest, CleanCloudData_LocalAndCloudData_Test, TestSize.Level1)
{
    auto store = CreateTestDatabase(CLOUD_CLEANER_TEST_DB);
    ASSERT_NE(store, nullptr);
 
    CreatePhotosTable(store);
 
    const string INSERT1 = "INSERT INTO Photos (display_name, position, clean_flag, "
                           "dirty, cloud_id, cloud_version, south_device_type) "
                           "VALUES ('local_cloud_1', 3, 0, 0, 'cloud_id_1', 1, 1);";
    const string INSERT2 = "INSERT INTO Photos (display_name, position, clean_flag, "
                           "dirty, cloud_id, cloud_version, south_device_type) "
                           "VALUES ('local_cloud_2', 3, 0, 0, 'cloud_id_2', 2, 2);";
    EXPECT_EQ(ExecuteSql(store, INSERT1), E_OK);
    EXPECT_EQ(ExecuteSql(store, INSERT2), E_OK);
 
    auto rs = store->QuerySql("SELECT COUNT(*) FROM Photos WHERE position=3 AND cloud_id IS NOT NULL;");
    ASSERT_NE(rs, nullptr);
    int64_t count = 0;
    if (rs->GoToNextRow() == E_OK) rs->GetLong(0, count);
    rs->Close();
    EXPECT_EQ(count, 2);
 
    CloudDataCleaner cleaner(store);
    EXPECT_TRUE(cleaner.CleanCloudData());
 
    rs = store->QuerySql("SELECT position, cloud_id, cloud_version, dirty, south_device_type "
        "FROM Photos WHERE display_name LIKE 'local_cloud%' ORDER BY file_id;");
    ASSERT_NE(rs, nullptr);
    int idx = 0;
    while (rs->GoToNextRow() == E_OK && idx < 2) {
        int32_t position = 0, dirty = 0, southDev = 0;
        string cloudId;
        int64_t cloudVersion = 0;
        rs->GetInt(0, position);
        rs->GetString(1, cloudId);
        rs->GetLong(2, cloudVersion);
        rs->GetInt(3, dirty);
        rs->GetInt(4, southDev);
        EXPECT_EQ(position, 1);
        EXPECT_TRUE(cloudId.empty());
        EXPECT_EQ(cloudVersion, 0);
        EXPECT_EQ(dirty, 1);
        EXPECT_EQ(southDev, 0);
        idx++;
    }
    rs->Close();
    EXPECT_EQ(idx, 2);
}
 
/*
 * Test Case Description:
 * - Coverage: Delete dirty=-1 records (mark-deleted data)
 * - Branch point: ClearDeletedDbData - physical deletion
 * - Trigger condition: Insert records with dirty=-1
 * - Business validation: Records with dirty=-1 should be physically deleted, normal records remain
 */
HWTEST_F(CloudDataCleanerTest, CleanCloudData_DeleteMarkedRecords_Test, TestSize.Level1)
{
    auto store = CreateTestDatabase(CLOUD_CLEANER_TEST_DB);
    ASSERT_NE(store, nullptr);
 
    CreatePhotosTable(store);
 
    EXPECT_EQ(ExecuteSql(store, "INSERT INTO Photos (display_name, position, dirty) VALUES ('deleted_1', 1, -1);"),
        E_OK);
    EXPECT_EQ(ExecuteSql(store, "INSERT INTO Photos (display_name, position, dirty) VALUES ('deleted_2', 2, -1);"),
        E_OK);
    EXPECT_EQ(ExecuteSql(store, "INSERT INTO Photos (display_name, position, dirty) VALUES ('normal_1', 1, 0);"), E_OK);
    EXPECT_EQ(ExecuteSql(store, "INSERT INTO Photos (display_name, position, dirty) VALUES ('normal_2', 3, 0);"), E_OK);
 
    auto rs = store->QuerySql("SELECT COUNT(*) FROM Photos WHERE dirty=-1;");
    ASSERT_NE(rs, nullptr);
    int64_t count = 0;
    if (rs->GoToNextRow() == E_OK) rs->GetLong(0, count);
    rs->Close();
    EXPECT_EQ(count, 2);
 
    CloudDataCleaner cleaner(store);
    EXPECT_TRUE(cleaner.CleanCloudData());
 
    rs = store->QuerySql("SELECT COUNT(*) FROM Photos WHERE dirty=-1;");
    ASSERT_NE(rs, nullptr);
    int64_t remaining = 0;
    if (rs->GoToNextRow() == E_OK) rs->GetLong(0, remaining);
    rs->Close();
    EXPECT_EQ(remaining, 0);
 
    rs = store->QuerySql("SELECT COUNT(*) FROM Photos WHERE dirty >= 0;");
    ASSERT_NE(rs, nullptr);
    int64_t normalCount = 0;
    if (rs->GoToNextRow() == E_OK) rs->GetLong(0, normalCount);
    rs->Close();
    EXPECT_GE(normalCount, 4);
}
 
/*
 * Test Case Description:
 * - Coverage: Delete empty cloud albums
 * - Branch point: DeleteEmptyCloudAlbums - delete albums without valid photos
 * - Trigger condition: Insert cloud albums (album_is_local=2) with photos that become cleaned
 * - Business validation: Empty cloud albums should be deleted
 */
HWTEST_F(CloudDataCleanerTest, CleanCloudData_DeleteEmptyCloudAlbums_Test, TestSize.Level1)
{
    auto store = CreateTestDatabase(CLOUD_CLEANER_TEST_DB);
    ASSERT_NE(store, nullptr);
 
    CreatePhotosTable(store);
    CreatePhotoAlbumTable(store);
 
    EXPECT_EQ(ExecuteSql(store, "INSERT INTO PhotoAlbum (album_is_local, cloud_id, dirty, lpath, album_type)"
        "VALUES (2, 'cloud_album_1', 0, '/cloud/path1', 0);"), E_OK);
    EXPECT_EQ(ExecuteSql(store, "INSERT INTO PhotoAlbum (album_is_local, cloud_id, dirty, lpath, album_type) "
        "VALUES (2, 'cloud_album_2', 0, '/cloud/path2', 0);"), E_OK);
    EXPECT_EQ(ExecuteSql(store, "INSERT INTO Photos (display_name, position, clean_flag, dirty, cloud_id, "
        "owner_album_id) VALUES ('cloud_photo_1', 2, 0, 0, 'photo_cloud_1', 1);"), E_OK);
    EXPECT_EQ(ExecuteSql(store, "INSERT INTO Photos (display_name, position, clean_flag, dirty, cloud_id, "
        "owner_album_id) VALUES ('cloud_photo_2', 2, 0, 0, 'photo_cloud_2', 2);"), E_OK);
 
    auto rs = store->QuerySql("SELECT COUNT(*) FROM PhotoAlbum WHERE album_is_local=2;");
    ASSERT_NE(rs, nullptr);
    int64_t albumCount = 0;
    if (rs->GoToNextRow() == E_OK) rs->GetLong(0, albumCount);
    rs->Close();
    EXPECT_EQ(albumCount, 2);
 
    CloudDataCleaner cleaner(store);
    EXPECT_TRUE(cleaner.CleanCloudData());
 
    rs = store->QuerySql("SELECT COUNT(*) FROM PhotoAlbum WHERE album_is_local=2;");
    ASSERT_NE(rs, nullptr);
    int64_t remainingAlbums = 0;
    if (rs->GoToNextRow() == E_OK) rs->GetLong(0, remainingAlbums);
    rs->Close();
    EXPECT_EQ(remainingAlbums, 0);
}
 
/*
 * Test Case Description:
 * - Coverage: Drop backup table PhotosAlbumBackupForSaveAnalysisData
 * - Branch point: DeleteAnalysisBackupAlbums - DROP TABLE
 * - Trigger condition: Create and populate the backup table
 * - Business validation: Table should be dropped after CleanCloudData
 */
HWTEST_F(CloudDataCleanerTest, CleanCloudData_DropBackupTable_Test, TestSize.Level1)
{
    auto store = CreateTestDatabase(CLOUD_CLEANER_TEST_DB);
    ASSERT_NE(store, nullptr);
 
    CreatePhotosTable(store);
    CreateBackupTable(store);
 
    EXPECT_EQ(ExecuteSql(store, "INSERT INTO PhotosAlbumBackupForSaveAnalysisData (data) VALUES ('test_data_1');"),
        E_OK);
    EXPECT_EQ(ExecuteSql(store, "INSERT INTO PhotosAlbumBackupForSaveAnalysisData (data) VALUES ('test_data_2');"),
        E_OK);
 
    auto rs = store->QuerySql("PRAGMA table_info(PhotosAlbumBackupForSaveAnalysisData);");
    ASSERT_NE(rs, nullptr);
    bool existsBefore = (rs->GoToNextRow() == E_OK);
    rs->Close();
    EXPECT_TRUE(existsBefore);
 
    CloudDataCleaner cleaner(store);
    EXPECT_TRUE(cleaner.CleanCloudData());
 
    rs = store->QuerySql("PRAGMA table_info(PhotosAlbumBackupForSaveAnalysisData);");
    ASSERT_NE(rs, nullptr);
    bool existsAfter = (rs->GoToNextRow() == E_OK);
    rs->Close();
    EXPECT_FALSE(existsAfter);
}
 
/*
 * Test Case Description:
 * - Coverage: Update local albums - clear cloud info and reset upload status
 * - Branch point: UpdateLocalAlbums - reset upload_status for system albums
 * - Trigger condition: Insert albums with cloud_id and various paths
 * - Business validation: cloud_id cleared, dirty=1, upload_status set based on path
 */
HWTEST_F(CloudDataCleanerTest, CleanCloudData_UpdateLocalAlbums_Test, TestSize.Level1)
{
    auto store = CreateTestDatabase(CLOUD_CLEANER_TEST_DB);
    ASSERT_NE(store, nullptr);
 
    CreatePhotoAlbumTable(store);
 
    EXPECT_EQ(ExecuteSql(store, "INSERT INTO PhotoAlbum (album_is_local, dirty, cloud_id, upload_status, "
        "lpath, album_type) VALUES (0, 0, 'cloud_id_1', 1, '/DCIM/Camera', 0);"), E_OK);
    EXPECT_EQ(ExecuteSql(store, "INSERT INTO PhotoAlbum (album_is_local, dirty, cloud_id, upload_status, "
        "lpath, album_type) VALUES (0, 0, 'cloud_id_2', 1, '/Pictures/Screenshots', 2048);"), E_OK);
    EXPECT_EQ(ExecuteSql(store, "INSERT INTO PhotoAlbum (album_is_local, dirty, cloud_id, upload_status, "
        "lpath, album_type) VALUES (0, 0, 'cloud_id_3', 1, '/custom/path', 0);"), E_OK);
 
    CloudDataCleaner cleaner(store);
    EXPECT_TRUE(cleaner.CleanCloudData());
 
    auto rs = store->QuerySql("SELECT COUNT(*) FROM PhotoAlbum WHERE cloud_id IS NOT NULL;");
    ASSERT_NE(rs, nullptr);
    int64_t cloudIdCount = 0;
    if (rs->GoToNextRow() == E_OK) rs->GetLong(0, cloudIdCount);
    rs->Close();
    EXPECT_EQ(cloudIdCount, 0);
 
    rs = store->QuerySql("SELECT COUNT(*) FROM PhotoAlbum WHERE dirty=1;");
    ASSERT_NE(rs, nullptr);
    int64_t dirtyCount = 0;
    if (rs->GoToNextRow() == E_OK) rs->GetLong(0, dirtyCount);
    rs->Close();
    EXPECT_EQ(dirtyCount, 3);
 
    rs = store->QuerySql("SELECT upload_status FROM PhotoAlbum WHERE LOWER(lpath) IN "
        "('/dcim/camera', '/pictures/screenshots', '/pictures/screenrecords') ORDER BY album_id;");
    ASSERT_NE(rs, nullptr);
    int idx = 0;
    while (rs->GoToNextRow() == E_OK && idx < 2) {
        int64_t status = 0;
        rs->GetLong(0, status);
        EXPECT_EQ(status, 1);
        idx++;
    }
    rs->Close();
    EXPECT_EQ(idx, 2);
 
    rs = store->QuerySql("SELECT upload_status FROM PhotoAlbum WHERE LOWER(lpath) = '/custom/path';");
    ASSERT_NE(rs, nullptr);
    int64_t customStatus = 0;
    if (rs->GoToNextRow() == E_OK) rs->GetLong(0, customStatus);
    rs->Close();
    EXPECT_EQ(customStatus, 0);
}
 
/*
 * Test Case Description:
 * - Coverage: Full integration test with mixed data types
 * - Branch point: All code paths in CleanCloudData
 * - Trigger condition: Insert mixed cloud, local-and-cloud, normal, and backup data
 * - Business validation: All operations should complete successfully with correct final state
 */
HWTEST_F(CloudDataCleanerTest, CleanCloudData_FullIntegration_Test, TestSize.Level1)
{
    auto store = CreateTestDatabase(CLOUD_CLEANER_TEST_DB);
    ASSERT_NE(store, nullptr);
 
    CreatePhotosTable(store);
    CreatePhotoAlbumTable(store);
    CreateBackupTable(store);
 
    // Pure cloud data
    EXPECT_EQ(ExecuteSql(store, "INSERT INTO Photos (display_name, position, clean_flag, dirty, cloud_id, "
        "cloud_version) VALUES ('pure_cloud_1', 2, 0, 0, 'cid_1', 1);"), E_OK);
    EXPECT_EQ(ExecuteSql(store, "INSERT INTO Photos (display_name, position, clean_flag, dirty, cloud_id, "
        "cloud_version) VALUES ('pure_cloud_2', 2, 0, 0, 'cid_2', 2);"), E_OK);
 
    // Local-and-cloud data
    EXPECT_EQ(ExecuteSql(store, "INSERT INTO Photos (display_name, position, dirty, cloud_id, cloud_version, "
        "south_device_type) VALUES ('lc_1', 3, 0, 'cid_3', 3, 1);"), E_OK);
    EXPECT_EQ(ExecuteSql(store, "INSERT INTO Photos (display_name, position, dirty, cloud_id, cloud_version, "
        "south_device_type) VALUES ('lc_2', 3, 0, 'cid_4', 4, 2);"), E_OK);
 
    // Mark-deleted data
    EXPECT_EQ(ExecuteSql(store, "INSERT INTO Photos (display_name, position, dirty) VALUES ('deleted_1', 1, -1);"),
        E_OK);
 
    // Normal local data
    EXPECT_EQ(ExecuteSql(store, "INSERT INTO Photos (display_name, position, dirty) VALUES ('local_1', 1, 0);"), E_OK);
 
    // Cloud albums
    EXPECT_EQ(ExecuteSql(store, "INSERT INTO PhotoAlbum (album_is_local, cloud_id, dirty, album_type) "
        "VALUES (2, 'album_cid_1', 0, 0);"), E_OK);
    EXPECT_EQ(ExecuteSql(store, "INSERT INTO PhotoAlbum (album_is_local, cloud_id, dirty, album_type) "
        "VALUES (2, 'album_cid_2', 0, 0);"), E_OK);
 
    // Backup data
    EXPECT_EQ(ExecuteSql(store, "INSERT INTO PhotosAlbumBackupForSaveAnalysisData (data) VALUES ('backup_1');"), E_OK);
 
    CloudDataCleaner cleaner(store);
    EXPECT_TRUE(cleaner.CleanCloudData());
 
    // Pure cloud marked
    auto rs = store->QuerySql("SELECT COUNT(*) FROM Photos WHERE display_name LIKE 'pure_cloud%' "
        "AND clean_flag=1 AND dirty=-1;");
    ASSERT_NE(rs, nullptr);
    int64_t count = 0;
    if (rs->GoToNextRow() == E_OK) rs->GetLong(0, count);
    rs->Close();
    EXPECT_EQ(count, 2);
 
    // Local-and-cloud converted
    rs = store->QuerySql("SELECT COUNT(*) FROM Photos WHERE display_name LIKE 'lc_%' "
        "AND position=1 AND cloud_id IS NULL;");
    ASSERT_NE(rs, nullptr);
    count = 0;
    if (rs->GoToNextRow() == E_OK) rs->GetLong(0, count);
    rs->Close();
    EXPECT_EQ(count, 2);
 
    // Mark-deleted removed
    rs = store->QuerySql("SELECT COUNT(*) FROM Photos WHERE dirty=-1;");
    ASSERT_NE(rs, nullptr);
    count = 0;
    if (rs->GoToNextRow() == E_OK) rs->GetLong(0, count);
    rs->Close();
    EXPECT_EQ(count, 0);
 
    // Normal data untouched
    rs = store->QuerySql("SELECT COUNT(*) FROM Photos WHERE display_name='local_1' AND position=1 AND dirty=0;");
    ASSERT_NE(rs, nullptr);
    count = 0;
    if (rs->GoToNextRow() == E_OK) rs->GetLong(0, count);
    rs->Close();
    EXPECT_EQ(count, 1);
 
    // Backup table dropped
    rs = store->QuerySql("PRAGMA table_info(PhotosAlbumBackupForSaveAnalysisData);");
    ASSERT_NE(rs, nullptr);
    bool tableExists = (rs->GoToNextRow() == E_OK);
    rs->Close();
    EXPECT_FALSE(tableExists);
}
 
} // namespace Media
} // namespace OHOS
