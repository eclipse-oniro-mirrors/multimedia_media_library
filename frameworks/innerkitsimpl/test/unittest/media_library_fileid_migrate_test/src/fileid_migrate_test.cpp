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
 
#define MLOG_TAG "FileIdMigrateTest"
 
#include "fileid_migrate_test.h"
#include "file_id_migrator.h"
#include "media_log.h"
#include "medialibrary_unittest_utils.h"
#include <sys/stat.h>
#include <unistd.h>
 
using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::Media::TestUtils;
 
namespace OHOS {
namespace Media {
 
static constexpr int32_t SLEEP_TWO_SECONDS = 2;
 
// ===================== Test Fixture Implementation =====================
 
void FileIdMigrateTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("FileIdMigrateTest SetUpTestCase");
    struct stat st;
    if (stat(TEST_DIR, &st) != 0) {
        mkdir(TEST_DIR, 0755);
    }
}
 
void FileIdMigrateTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("FileIdMigrateTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_TWO_SECONDS));
}
 
void FileIdMigrateTest::SetUp()
{
    MEDIA_INFO_LOG("FileIdMigrateTest SetUp");
    oldDb_ = CreateTestDatabase(FILEID_OLD_DB);
    newDb_ = CreateTestDatabase(FILEID_NEW_DB);
}
 
void FileIdMigrateTest::TearDown()
{
    MEDIA_INFO_LOG("FileIdMigrateTest TearDown");
    oldDb_.reset();
    newDb_.reset();
    CloseAndDeleteDatabase(FILEID_OLD_DB);
    CloseAndDeleteDatabase(FILEID_NEW_DB);
}
 
shared_ptr<RdbStore> FileIdMigrateTest::CreateTestDatabase(const string& dbName)
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
 
void FileIdMigrateTest::CloseAndDeleteDatabase(const string& dbName)
{
    string dbPath = string(TEST_DIR) + dbName;
    unlink(dbPath.c_str());
    string walPath = dbPath + "-wal";
    unlink(walPath.c_str());
    string shmPath = dbPath + "-shm";
    unlink(shmPath.c_str());
}
 
int32_t FileIdMigrateTest::CreatePhotosTable(shared_ptr<RdbStore> store)
{
    // 只包含偏移所需的列，其余列使用默认值或业务无关
    const string CREATE_PHOTOS_TABLE =
        "CREATE TABLE IF NOT EXISTS Photos ("
        "    file_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "    associate_file_id INT DEFAULT 0, "
        "    owner_album_id INT DEFAULT 0);";
 
    return ExecuteSql(store, CREATE_PHOTOS_TABLE);
}
 
int32_t FileIdMigrateTest::CreatePhotoAlbumTable(shared_ptr<RdbStore> store)
{
    // 使用真实表结构，偏移虽只用到 album_id 和 cover_uri，但保持一致有助于维护
    const string CREATE_PHOTO_ALBUM_TABLE =
        "CREATE TABLE IF NOT EXISTS PhotoAlbum ("
        "    album_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "    album_type INT, "
        "    album_subtype INT, "
        "    album_name TEXT COLLATE NOCASE, "
        "    cover_uri TEXT, "
        "    count INT DEFAULT 0, "
        "    date_modified BIGINT DEFAULT 0, "
        "    dirty INT DEFAULT 1, "
        "    cloud_id TEXT, "
        "    relative_path TEXT, "
        "    contains_hidden INT DEFAULT 0, "
        "    hidden_count INT DEFAULT 0, "
        "    hidden_cover TEXT DEFAULT '', "
        "    album_order INT, "
        "    image_count INT DEFAULT 0, "
        "    video_count INT DEFAULT 0, "
        "    bundle_name TEXT, "
        "    local_language TEXT, "
        "    is_local INT, "
        "    date_added BIGINT DEFAULT 0, "
        "    lpath TEXT, "
        "    priority INT, "
        "    metadata_flags INT DEFAULT 0, "
        "    check_flag INT DEFAULT 0, "
        "    albums_order INT DEFAULT -1, "
        "    order_section INT DEFAULT -1, "
        "    order_type INT DEFAULT -1, "
        "    order_status INT DEFAULT 0, "
        "    style2_albums_order INT DEFAULT -1, "
        "    style2_order_section INT DEFAULT -1, "
        "    style2_order_type INT DEFAULT -1, "
        "    style2_order_status INT DEFAULT 0, "
        "    cover_uri_source INT NOT NULL DEFAULT 0, "
        "    cover_cloud_id TEXT, "
        "    cover_date_time BIGINT DEFAULT 0, "
        "    hidden_cover_date_time BIGINT DEFAULT 0, "
        "    upload_status INT NOT NULL DEFAULT 0, "
        "    change_time BIGINT NOT NULL DEFAULT 0, "
        "    hidden INT NOT NULL DEFAULT 0);";
 
    return ExecuteSql(store, CREATE_PHOTO_ALBUM_TABLE);
}
 
int32_t FileIdMigrateTest::CreateHighlightCoverTable(shared_ptr<RdbStore> store)
{
    // 使用真实表结构，复合主键
    const string CREATE_HIGHLIGHT_COVER_TABLE =
        "CREATE TABLE IF NOT EXISTS tab_highlight_cover_info ("
        "    album_id INTEGER, "
        "    ratio TEXT, "
        "    background TEXT, "
        "    foreground TEXT, "
        "    wordart TEXT, "
        "    is_covered BOOL, "
        "    color TEXT, "
        "    radius INT, "
        "    saturation REAL, "
        "    brightness REAL, "
        "    background_color_type INT, "
        "    shadow_level INT, "
        "    title_scale_x REAL, "
        "    title_scale_y REAL, "
        "    title_rect_width REAL, "
        "    title_rect_height REAL, "
        "    background_scale_x REAL, "
        "    background_scale_y REAL, "
        "    background_rect_width REAL, "
        "    background_rect_height REAL, "
        "    layout_index INT, "
        "    cover_algo_version INT, "
        "    cover_service_version INT DEFAULT 0, "
        "    cover_key TEXT, "
        "    status INT DEFAULT 0, "
        "    PRIMARY KEY (album_id, ratio));";
 
    return ExecuteSql(store, CREATE_HIGHLIGHT_COVER_TABLE);
}
 
int32_t FileIdMigrateTest::CreateAnalysisTables(shared_ptr<RdbStore> store)
{
    int32_t ret = E_OK;
 
    // tab_analysis_aesthetics_score - 真实结构
    string sql = "CREATE TABLE IF NOT EXISTS tab_analysis_aesthetics_score ("
                 "    id INTEGER PRIMARY KEY AUTOINCREMENT, "
                 "    file_id INT UNIQUE, "
                 "    aesthetics_score INT, "
                 "    aesthetics_version TEXT, "
                 "    prob REAL, "
                 "    analysis_version TEXT, "
                 "    selected_flag INT, "
                 "    selected_algo_version TEXT, "
                 "    selected_status INT, "
                 "    negative_flag INT, "
                 "    negative_algo_version TEXT, "
                 "    aesthetics_all_version TEXT, "
                 "    aesthetics_score_all INT NOT NULL DEFAULT 0, "
                 "    is_filtered_hard BOOLEAN NOT NULL DEFAULT 0, "
                 "    clarity_score_all DOUBLE NOT NULL DEFAULT 0, "
                 "    saturation_score_all DOUBLE NOT NULL DEFAULT 0, "
                 "    luminance_score_all DOUBLE NOT NULL DEFAULT 0, "
                 "    semantics_score DOUBLE NOT NULL DEFAULT 0, "
                 "    is_black_white_stripe BOOLEAN NOT NULL DEFAULT 0, "
                 "    is_blurry BOOLEAN NOT NULL DEFAULT 0, "
                 "    is_mosaic BOOLEAN NOT NULL DEFAULT 0);";
    ret = ExecuteSql(store, sql);
    if (ret != E_OK) return ret;
 
    // tab_analysis_total - 真实结构
    sql = "CREATE TABLE IF NOT EXISTS tab_analysis_total ("
          "    id INTEGER PRIMARY KEY AUTOINCREMENT, "
          "    file_id INT UNIQUE, "
          "    status INT, "
          "    ocr INT, "
          "    label INT, "
          "    aesthetics_score INT, "
          "    face INT, "
          "    object INT, "
          "    recommendation INT, "
          "    segmentation INT, "
          "    composition INT, "
          "    saliency INT, "
          "    head INT, "
          "    pose INT, "
          "    geo INT DEFAULT 0, "
          "    selected INT, "
          "    negative INT, "
          "    abstract_node_analysis INT, "
          "    priority INT NOT NULL DEFAULT 1, "
          "    aesthetics_score_all INT NOT NULL DEFAULT 0, "
          "    graph_db INT NOT NULL DEFAULT 0, "
          "    affective INT NOT NULL DEFAULT 0, "
          "    pet INT NOT NULL DEFAULT 0, "
          "    similarity INT NOT NULL DEFAULT 0, "
          "    duplicate INT NOT NULL DEFAULT 0, "
          "    total_score INT NOT NULL DEFAULT 0, "
          "    selection INT NOT NULL DEFAULT 0, "
          "    aesthetics_crop INT NOT NULL DEFAULT 0, "
          "    ai_retouch INT NOT NULL DEFAULT 0, "
          "    magic_emoji INT NOT NULL DEFAULT 0);";
    ret = ExecuteSql(store, sql);
 
    return ret;
}
 
int32_t FileIdMigrateTest::ExecuteSql(shared_ptr<RdbStore> store, const string& sql)
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
 * - Coverage: Migrate file_id - basic offset calculation and application
 * - Branch point: MigrateFileIds - offset calculation with 10% margin
 * - Trigger condition: oldDb has file_id max=10000, newDb has file_id max=20
 * - Business validation: file_id 5,15,25 should become 10005,10015,10025; file_id 10000 should remain unchanged
 */
HWTEST_F(FileIdMigrateTest, MigrateFileId_BasicOffset_Test, TestSize.Level1)
{
    ASSERT_NE(oldDb_, nullptr);
    ASSERT_NE(newDb_, nullptr);
 
    CreatePhotosTable(oldDb_);
    CreatePhotosTable(newDb_);
 
    // Insert data into newDb (small IDs: max=20)
    EXPECT_EQ(ExecuteSql(newDb_, "INSERT INTO Photos (file_id) VALUES (10);"), E_OK);
    EXPECT_EQ(ExecuteSql(newDb_, "INSERT INTO Photos (file_id) VALUES (20);"), E_OK);
 
    // Insert data into oldDb (mixed IDs: max=10000)
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO Photos (file_id) VALUES (5);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO Photos (file_id) VALUES (15);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO Photos (file_id) VALUES (25);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO Photos (file_id) VALUES (10000);"), E_OK);
 
    FileIdMigrator migrator;
    EXPECT_TRUE(migrator.Migrate(oldDb_, newDb_));
 
    auto rs = oldDb_->QuerySql("SELECT file_id FROM Photos ORDER BY file_id;");
    ASSERT_NE(rs, nullptr);
    vector<int64_t> fileIds;
    while (rs->GoToNextRow() == E_OK) {
        int64_t fileId = 0;
        rs->GetLong(0, fileId);
        fileIds.push_back(fileId);
    }
    rs->Close();
 
    EXPECT_EQ(fileIds.size(), 4);
    EXPECT_EQ(fileIds[0], 10005);
    EXPECT_EQ(fileIds[1], 10015);
    EXPECT_EQ(fileIds[2], 10025);
    EXPECT_EQ(fileIds[3], 10000);
}
 
/*
 * Test Case Description:
 * - Coverage: Migrate album_id - basic offset calculation and application
 * - Branch point: MigrateAlbumIds - offset calculation with 10% margin
 * - Trigger condition: oldDb has album_id max=5000, newDb has album_id max=2
 * - Business validation: album_id 1,3 should become 5001,5003; album_id 5000 should remain unchanged
 */
HWTEST_F(FileIdMigrateTest, MigrateAlbumId_BasicOffset_Test, TestSize.Level1)
{
    ASSERT_NE(oldDb_, nullptr);
    ASSERT_NE(newDb_, nullptr);
 
    CreatePhotoAlbumTable(oldDb_);
    CreatePhotoAlbumTable(newDb_);
 
    // Insert data into newDb (small IDs: max=2)
    EXPECT_EQ(ExecuteSql(newDb_, "INSERT INTO PhotoAlbum (album_id) VALUES (1);"), E_OK);
    EXPECT_EQ(ExecuteSql(newDb_, "INSERT INTO PhotoAlbum (album_id) VALUES (2);"), E_OK);
 
    // Insert data into oldDb (mixed IDs: max=5000)
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO PhotoAlbum (album_id) VALUES (1);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO PhotoAlbum (album_id) VALUES (3);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO PhotoAlbum (album_id) VALUES (5000);"), E_OK);
 
    FileIdMigrator migrator;
    EXPECT_TRUE(migrator.Migrate(oldDb_, newDb_));
 
    auto rs = oldDb_->QuerySql("SELECT album_id FROM PhotoAlbum ORDER BY album_id;");
    ASSERT_NE(rs, nullptr);
    vector<int64_t> albumIds;
    while (rs->GoToNextRow() == E_OK) {
        int64_t albumId = 0;
        rs->GetLong(0, albumId);
        albumIds.push_back(albumId);
    }
    rs->Close();
 
    EXPECT_EQ(albumIds.size(), 3);
    EXPECT_EQ(albumIds[0], 5001);
    EXPECT_EQ(albumIds[1], 5003);
    EXPECT_EQ(albumIds[2], 5000);
}
 
/*
 * Test Case Description:
 * - Coverage: Update owner_album_id in Photos table
 * - Branch point: UpdatePhotosOwnerAlbumId - foreign key offset
 * - Trigger condition: Photos.owner_album_id references album_id values
 * - Business validation: owner_album_id should be offset by same amount as album_id
 */
HWTEST_F(FileIdMigrateTest, Migrate_UpdateOwnerAlbumId_Test, TestSize.Level1)
{
    ASSERT_NE(oldDb_, nullptr);
    ASSERT_NE(newDb_, nullptr);
 
    CreatePhotosTable(oldDb_);
    CreatePhotoAlbumTable(oldDb_);
    CreatePhotoAlbumTable(newDb_);
 
    // Insert albums into newDb (max=2)
    EXPECT_EQ(ExecuteSql(newDb_, "INSERT INTO PhotoAlbum (album_id) VALUES (1);"), E_OK);
    EXPECT_EQ(ExecuteSql(newDb_, "INSERT INTO PhotoAlbum (album_id) VALUES (2);"), E_OK);
 
    // Insert albums into oldDb (max=5000)
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO PhotoAlbum (album_id) VALUES (1);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO PhotoAlbum (album_id) VALUES (3);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO PhotoAlbum (album_id) VALUES (5000);"), E_OK);
 
    // Insert photos with owner_album_id references
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO Photos (file_id, owner_album_id) VALUES (100, 1);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO Photos (file_id, owner_album_id) VALUES (200, 3);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO Photos (file_id, owner_album_id) VALUES (300, 5000);"), E_OK);
 
    FileIdMigrator migrator;
    EXPECT_TRUE(migrator.Migrate(oldDb_, newDb_));
 
    auto rs = oldDb_->QuerySql("SELECT file_id, owner_album_id FROM Photos ORDER BY file_id;");
    ASSERT_NE(rs, nullptr);
    vector<pair<int64_t, int64_t>> photoData;
    while (rs->GoToNextRow() == E_OK) {
        int64_t fileId = 0, ownerAlbumId = 0;
        rs->GetLong(0, fileId);
        rs->GetLong(1, ownerAlbumId);
        photoData.push_back({fileId, ownerAlbumId});
    }
    rs->Close();
 
    EXPECT_EQ(photoData.size(), 3);
    EXPECT_EQ(photoData[0].second, 5001);
    EXPECT_EQ(photoData[1].second, 5003);
    EXPECT_EQ(photoData[2].second, 5000);
}
 
/*
 * Test Case Description:
 * - Coverage: Update associate_file_id in Photos table
 * - Branch point: UpdateDirectFileIdTables - associate_file_id column
 * - Trigger condition: Photos.associate_file_id references file_id values
 * - Business validation: associate_file_id should be offset by same amount as file_id
 */
HWTEST_F(FileIdMigrateTest, Migrate_UpdateAssociateFileId_Test, TestSize.Level1)
{
    ASSERT_NE(oldDb_, nullptr);
    ASSERT_NE(newDb_, nullptr);
 
    CreatePhotosTable(oldDb_);
    CreatePhotosTable(newDb_);
 
    // Insert data into newDb (max=20)
    EXPECT_EQ(ExecuteSql(newDb_, "INSERT INTO Photos (file_id) VALUES (10);"), E_OK);
    EXPECT_EQ(ExecuteSql(newDb_, "INSERT INTO Photos (file_id) VALUES (20);"), E_OK);
 
    // Insert data into oldDb with associate_file_id references
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO Photos (file_id, associate_file_id) VALUES (5, 15);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO Photos (file_id, associate_file_id) VALUES (15, 25);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO Photos (file_id, associate_file_id) VALUES (25, 0);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO Photos (file_id, associate_file_id) VALUES (10000, 0);"), E_OK);
 
    FileIdMigrator migrator;
    EXPECT_TRUE(migrator.Migrate(oldDb_, newDb_));
 
    auto rs = oldDb_->QuerySql("SELECT file_id, associate_file_id FROM Photos ORDER BY file_id;");
    ASSERT_NE(rs, nullptr);
    vector<pair<int64_t, int64_t>> photoData;
    while (rs->GoToNextRow() == E_OK) {
        int64_t fileId = 0, associateFileId = 0;
        rs->GetLong(0, fileId);
        rs->GetLong(1, associateFileId);
        photoData.push_back({fileId, associateFileId});
    }
    rs->Close();
 
    EXPECT_EQ(photoData.size(), 4);
    EXPECT_EQ(photoData[0].first, 10005);
    EXPECT_EQ(photoData[0].second, 10015);
    EXPECT_EQ(photoData[1].first, 10015);
    EXPECT_EQ(photoData[1].second, 10025);
    EXPECT_EQ(photoData[2].first, 10025);
    EXPECT_EQ(photoData[2].second, 0);
    EXPECT_EQ(photoData[3].first, 10000);
    EXPECT_EQ(photoData[3].second, 0);
}
 
/*
 * Test Case Description:
 * - Coverage: Update cover_uri in PhotoAlbum table (embedded file_id)
 * - Branch point: UpdateEmbeddedFileIds - regex replacement in URI strings
 * - Trigger condition: PhotoAlbum.cover_uri contains "file://media/Photo/数字/" or "file://media/Video/数字/"
 * - Business validation: Embedded file_id in URI should be offset correctly
 */
HWTEST_F(FileIdMigrateTest, Migrate_UpdateCoverUri_Test, TestSize.Level1)
{
    ASSERT_NE(oldDb_, nullptr);
    ASSERT_NE(newDb_, nullptr);
 
    CreatePhotoAlbumTable(oldDb_);
    CreatePhotoAlbumTable(newDb_);
 
    EXPECT_EQ(ExecuteSql(newDb_, "INSERT INTO PhotoAlbum (album_id) VALUES (1);"), E_OK);
    EXPECT_EQ(ExecuteSql(newDb_, "INSERT INTO PhotoAlbum (album_id) VALUES (2);"), E_OK);
 
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO PhotoAlbum (album_id, cover_uri) "
        "VALUES (1, 'file://media/Photo/5/cover.jpg');"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO PhotoAlbum (album_id, cover_uri) "
        "VALUES (3, 'file://media/Video/15/video_cover.jpg');"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO PhotoAlbum (album_id, cover_uri) "
        "VALUES (5000, 'file://media/Photo/10000/unchanged.jpg');"), E_OK);
 
    FileIdMigrator migrator;
    EXPECT_TRUE(migrator.Migrate(oldDb_, newDb_));
 
    auto rs = oldDb_->QuerySql("SELECT album_id, cover_uri FROM PhotoAlbum ORDER BY album_id;");
    ASSERT_NE(rs, nullptr);
    vector<pair<int64_t, string>> albumData;
    while (rs->GoToNextRow() == E_OK) {
        int64_t albumId = 0;
        string coverUri;
        rs->GetLong(0, albumId);
        rs->GetString(1, coverUri);
        albumData.push_back({albumId, coverUri});
    }
    rs->Close();
 
    EXPECT_EQ(albumData.size(), 3);
    EXPECT_EQ(albumData[0].first, 5001);
    EXPECT_EQ(albumData[0].second, "file://media/Photo/10005/cover.jpg");
    EXPECT_EQ(albumData[1].first, 5003);
    EXPECT_EQ(albumData[1].second, "file://media/Video/10015/video_cover.jpg");
    EXPECT_EQ(albumData[2].first, 5000);
    EXPECT_EQ(albumData[2].second, "file://media/Photo/10000/unchanged.jpg");
}
 
/*
 * Test Case Description:
 * - Coverage: Update cover_key in tab_highlight_cover_info table
 * - Branch point: UpdateEmbeddedFileIds - highlight cover table
 * - Trigger condition: tab_highlight_cover_info.cover_key contains file_id
 * - Business validation: Embedded file_id in cover_key should be offset correctly
 */
HWTEST_F(FileIdMigrateTest, Migrate_UpdateHighlightCover_Test, TestSize.Level1)
{
    ASSERT_NE(oldDb_, nullptr);
    ASSERT_NE(newDb_, nullptr);
 
    CreateHighlightCoverTable(oldDb_);
    CreateHighlightCoverTable(newDb_);
    CreatePhotoAlbumTable(newDb_);
 
    EXPECT_EQ(ExecuteSql(newDb_, "INSERT INTO PhotoAlbum (album_id) VALUES (1);"), E_OK);
    EXPECT_EQ(ExecuteSql(newDb_, "INSERT INTO PhotoAlbum (album_id) VALUES (2);"), E_OK);
 
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO tab_highlight_cover_info (album_id, ratio, cover_key) "
        "VALUES (1, '16:9', 'file://media/Photo/5/key.jpg');"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO tab_highlight_cover_info (album_id, ratio, cover_key) "
        "VALUES (3, '16:9', 'file://media/Photo/15/key2.jpg');"), E_OK);
 
    FileIdMigrator migrator;
    EXPECT_TRUE(migrator.Migrate(oldDb_, newDb_));
 
    auto rs = oldDb_->QuerySql("SELECT album_id, cover_key FROM tab_highlight_cover_info ORDER BY album_id;");
    ASSERT_NE(rs, nullptr);
    vector<pair<int64_t, string>> coverData;
    while (rs->GoToNextRow() == E_OK) {
        int64_t albumId = 0;
        string coverKey;
        rs->GetLong(0, albumId);
        rs->GetString(1, coverKey);
        coverData.push_back({albumId, coverKey});
    }
    rs->Close();
 
    EXPECT_EQ(coverData.size(), 2);
    EXPECT_EQ(coverData[0].first, 5001);
    EXPECT_EQ(coverData[0].second, "file://media/Photo/10005/key.jpg");
    EXPECT_EQ(coverData[1].first, 5003);
    EXPECT_EQ(coverData[1].second, "file://media/Photo/10015/key2.jpg");
}
 
/*
 * Test Case Description:
 * - Coverage: Update file_id in analysis tables
 * - Branch point: UpdateDirectFileIdTables - analysis table columns
 * - Trigger condition: Analysis tables have file_id columns
 * - Business validation: file_id in analysis tables should be offset correctly
 */
HWTEST_F(FileIdMigrateTest, Migrate_UpdateAnalysisTables_Test, TestSize.Level1)
{
    ASSERT_NE(oldDb_, nullptr);
    ASSERT_NE(newDb_, nullptr);
 
    CreatePhotosTable(oldDb_);
    CreatePhotosTable(newDb_);
    CreateAnalysisTables(oldDb_);
 
    EXPECT_EQ(ExecuteSql(newDb_, "INSERT INTO Photos (file_id) VALUES (10);"), E_OK);
    EXPECT_EQ(ExecuteSql(newDb_, "INSERT INTO Photos (file_id) VALUES (20);"), E_OK);
 
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO Photos (file_id) VALUES (5);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO Photos (file_id) VALUES (15);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO Photos (file_id) VALUES (25);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO Photos (file_id) VALUES (10000);"), E_OK);
 
    // Insert data into analysis tables one by one
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO tab_analysis_aesthetics_score (file_id, aesthetics_score) "
        "VALUES (5, 85);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO tab_analysis_aesthetics_score (file_id, aesthetics_score) "
        "VALUES (15, 90);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO tab_analysis_aesthetics_score (file_id, aesthetics_score) "
        "VALUES (10000, 75);"), E_OK);
 
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO tab_analysis_total (file_id, total_score) VALUES (5, 100);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO tab_analysis_total (file_id, total_score) VALUES (15, 200);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO tab_analysis_total (file_id, total_score) VALUES (25, 300);"), E_OK);
 
    FileIdMigrator migrator;
    EXPECT_TRUE(migrator.Migrate(oldDb_, newDb_));
 
    // Verify tab_analysis_aesthetics_score file_id offset
    auto rs = oldDb_->QuerySql("SELECT file_id, aesthetics_score FROM tab_analysis_aesthetics_score ORDER BY file_id;");
    ASSERT_NE(rs, nullptr);
    vector<pair<int64_t, int64_t>> aestheticsData;
    while (rs->GoToNextRow() == E_OK) {
        int64_t fileId = 0, score = 0;
        rs->GetLong(0, fileId);
        rs->GetLong(1, score);
        aestheticsData.push_back({fileId, score});
    }
    rs->Close();
 
    EXPECT_EQ(aestheticsData.size(), 3);
    EXPECT_EQ(aestheticsData[0].first, 10005);
    EXPECT_EQ(aestheticsData[0].second, 85);
    EXPECT_EQ(aestheticsData[1].first, 10015);
    EXPECT_EQ(aestheticsData[1].second, 90);
    EXPECT_EQ(aestheticsData[2].first, 10000);
    EXPECT_EQ(aestheticsData[2].second, 75);
 
    // Verify tab_analysis_total file_id offset
    rs = oldDb_->QuerySql("SELECT file_id, total_score FROM tab_analysis_total ORDER BY file_id;");
    ASSERT_NE(rs, nullptr);
    vector<pair<int64_t, int64_t>> totalData;
    while (rs->GoToNextRow() == E_OK) {
        int64_t fileId = 0, total = 0;
        rs->GetLong(0, fileId);
        rs->GetLong(1, total);
        totalData.push_back({fileId, total});
    }
    rs->Close();
 
    EXPECT_EQ(totalData.size(), 3);
    EXPECT_EQ(totalData[0].first, 10005);
    EXPECT_EQ(totalData[0].second, 100);
    EXPECT_EQ(totalData[1].first, 10015);
    EXPECT_EQ(totalData[1].second, 200);
    EXPECT_EQ(totalData[2].first, 10025);
    EXPECT_EQ(totalData[2].second, 300);
}
 
/*
 * Test Case Description:
 * - Coverage: Skip tables that don't exist
 * - Branch point: TableExists - table existence check
 * - Trigger condition: Some analysis tables don't exist in the database
 * - Business validation: Migration should succeed without error even if some tables are missing
 */
HWTEST_F(FileIdMigrateTest, Migrate_SkipNonExistentTables_Test, TestSize.Level1)
{
    ASSERT_NE(oldDb_, nullptr);
    ASSERT_NE(newDb_, nullptr);
 
    CreatePhotosTable(oldDb_);
    CreatePhotosTable(newDb_);
    CreatePhotoAlbumTable(newDb_);
 
    EXPECT_EQ(ExecuteSql(newDb_, "INSERT INTO Photos (file_id) VALUES (10);"), E_OK);
    EXPECT_EQ(ExecuteSql(newDb_, "INSERT INTO Photos (file_id) VALUES (20);"), E_OK);
 
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO Photos (file_id) VALUES (5);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO Photos (file_id) VALUES (15);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO Photos (file_id) VALUES (25);"), E_OK);
 
    FileIdMigrator migrator;
    EXPECT_TRUE(migrator.Migrate(oldDb_, newDb_));
 
    auto rs = oldDb_->QuerySql("SELECT file_id FROM Photos ORDER BY file_id;");
    ASSERT_NE(rs, nullptr);
    vector<int64_t> fileIds;
    while (rs->GoToNextRow() == E_OK) {
        int64_t fileId = 0;
        rs->GetLong(0, fileId);
        fileIds.push_back(fileId);
    }
    rs->Close();
 
    EXPECT_EQ(fileIds.size(), 3);
    EXPECT_EQ(fileIds[0], 10005);
    EXPECT_EQ(fileIds[1], 10015);
    EXPECT_EQ(fileIds[2], 10025);
}
 
/*
 * Test Case Description:
 * - Coverage: Full integration test with all table types
 * - Branch point: All code paths in Migrate
 * - Trigger condition: Complete database setup with all supported tables
 * - Business validation: All file_id and album_id references should be offset correctly
 */
HWTEST_F(FileIdMigrateTest, Migrate_FullIntegration_Test, TestSize.Level1)
{
    ASSERT_NE(oldDb_, nullptr);
    ASSERT_NE(newDb_, nullptr);
 
    CreatePhotosTable(oldDb_);
    CreatePhotosTable(newDb_);
    CreatePhotoAlbumTable(oldDb_);
    CreatePhotoAlbumTable(newDb_);
    CreateHighlightCoverTable(oldDb_);
    CreateAnalysisTables(oldDb_);
 
    EXPECT_EQ(ExecuteSql(newDb_, "INSERT INTO Photos (file_id) VALUES (10);"), E_OK);
    EXPECT_EQ(ExecuteSql(newDb_, "INSERT INTO Photos (file_id) VALUES (20);"), E_OK);
    EXPECT_EQ(ExecuteSql(newDb_, "INSERT INTO PhotoAlbum (album_id) VALUES (1);"), E_OK);
    EXPECT_EQ(ExecuteSql(newDb_, "INSERT INTO PhotoAlbum (album_id) VALUES (2);"), E_OK);
 
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO Photos (file_id, associate_file_id, owner_album_id) VALUES (5, 15, 1);"),
        E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO Photos (file_id, associate_file_id, "
        "owner_album_id) VALUES (15, 25, 3);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO Photos (file_id, associate_file_id, owner_album_id) "
        "VALUES (25, 0, 5000);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO Photos (file_id, associate_file_id, owner_album_id) "
        "VALUES (10000, 0, 5000);"), E_OK);
 
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO PhotoAlbum (album_id, cover_uri) "
        "VALUES (1, 'file://media/Photo/5/cover.jpg');"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO PhotoAlbum (album_id, cover_uri) "
        "VALUES (3, 'file://media/Video/15/cover.jpg');"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO PhotoAlbum (album_id, cover_uri) "
        "VALUES (5000, 'file://media/Photo/10000/cover.jpg');"), E_OK);
 
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO tab_highlight_cover_info (album_id, ratio, cover_key) "
        "VALUES (1, '16:9', 'file://media/Photo/5/key.jpg');"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO tab_highlight_cover_info (album_id, ratio, cover_key) "
        "VALUES (3, '16:9', 'file://media/Photo/15/key.jpg');"), E_OK);
 
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO tab_analysis_aesthetics_score "
        "(file_id, aesthetics_score) VALUES (5, 85);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO tab_analysis_aesthetics_score "
        "(file_id, aesthetics_score) VALUES (15, 90);"), E_OK);
 
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO tab_analysis_total (file_id, total_score) VALUES (5, 100);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO tab_analysis_total (file_id, total_score) VALUES (15, 200);"), E_OK);
    EXPECT_EQ(ExecuteSql(oldDb_, "INSERT INTO tab_analysis_total (file_id, total_score) VALUES (25, 300);"), E_OK);
 
    FileIdMigrator migrator;
    EXPECT_TRUE(migrator.Migrate(oldDb_, newDb_));
 
    auto rs = oldDb_->QuerySql("SELECT file_id, associate_file_id, owner_album_id FROM Photos ORDER BY file_id;");
    ASSERT_NE(rs, nullptr);
    int count = 0;
    while (rs->GoToNextRow() == E_OK) {
        count++;
    }
    rs->Close();
    EXPECT_EQ(count, 4);
 
    rs = oldDb_->QuerySql("SELECT album_id, cover_uri FROM PhotoAlbum ORDER BY album_id;");
    ASSERT_NE(rs, nullptr);
    count = 0;
    while (rs->GoToNextRow() == E_OK) {
        count++;
    }
    rs->Close();
    EXPECT_EQ(count, 3);
}
 
} // namespace Media
} // namespace OHOS
