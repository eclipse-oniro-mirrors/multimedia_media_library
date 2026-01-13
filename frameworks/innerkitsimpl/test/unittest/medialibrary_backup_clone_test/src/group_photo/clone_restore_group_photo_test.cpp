/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "CloneRestoreGroupPhotoTest"

#include "clone_restore_group_photo_test.h"
#include "vision_db_sqls.h"
#include "vision_db_sqls_more.h"
#define private public
#define protected public
#include "clone_group_photo_source.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_rdb_utils.h"
#include "userfile_manager_types.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "backup_const.h"
#include "clone_restore.h"
#include "media_log.h"
#undef protected
#undef private

using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
const string TEST_BACKUP_PATH = "/data/test/backup/db";
const string TEST_DB_PATH = "/data/storage/el2/database/rdb/media_library.db";
const string TEST_BACKUP_DB_PATH = TEST_BACKUP_PATH + TEST_DB_PATH;
const std::unordered_map<int32_t, PhotoInfo> PHOTO_INFO_MAP = {
    { 1, PhotoInfo({ 1, MediaType::MEDIA_TYPE_IMAGE, "test.jpg", "/Photo/1/coverUri.jpg" }) },
    { 2, PhotoInfo({ 2, MediaType::MEDIA_TYPE_IMAGE, "test_002.jpg", "/Photo/1/coverUri_002.jpg" }) },
};

const std::unordered_map<int32_t, PhotoInfo> PHOTO_INFO_MAP_NO_DATA = {};
static shared_ptr<MediaLibraryRdbStore> g_rdbStore = nullptr;
static unique_ptr<CloneRestore> restoreService = nullptr;
static CloneGroupPhotoSource cloneSource;
static std::vector<std::string> createTableSqlLists = {
    PhotoUpgrade::CREATE_PHOTO_TABLE,
    CREATE_ANALYSIS_ALBUM_FOR_ONCREATE,
    CREATE_ANALYSIS_ALBUM_MAP,
};

static std::vector<std::string> testTables = {
    PhotoColumn::PHOTOS_TABLE,
    ANALYSIS_ALBUM_TABLE,
    ANALYSIS_PHOTO_MAP_TABLE,
};
const int32_t EXPECTED_GROUP_ALBUM_COUNT = 1;

static void InsertPhotoInNewDb(std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    rdbPtr->ExecuteSql(INSERT_PHOTO + VALUES_BEGIN + "1, " +
        "'/storage/cloud/files/Photo/16/test.jpg', 175258, 'test', 'test.jpg', 1, " +
        "'com.ohos.camera', '相机', 1501924205218, 1501924205423, 1501924205, 0, 0, 0, 0, " +
        "1280, 960, 0, '1'" + VALUES_END); // cam, pic, shootingmode = 1
    rdbPtr->ExecuteSql(INSERT_PHOTO + VALUES_BEGIN + "2, " +
        "'/storage/cloud/files/Photo/1/IMG_1501924307_001.jpg', 175397, 'test_002', 'test_002.jpg', 1, " +
        "'com.ohos.camera', '相机', 1501924207184, 1501924207286, 1501924207, 0, 0, 0, 0, " +
        "1280, 960, 0, ''" + VALUES_END); // cam, pic, trashed
}

static void InsertAnalysisAlbumInNewDb(std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    rdbPtr->ExecuteSql("INSERT INTO AnalysisAlbum ("
        "album_type, album_subtype, album_name, tag_id, cover_uri, is_cover_satisfied) "
        "VALUES (4096, 4103, 'test_group_photo_album_001', 'a|c,', 'test_cover_uri', 1)");
}

static void InsertAnalysisAlbumWithMerging(std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    rdbPtr->ExecuteSql("INSERT INTO AnalysisAlbum ("
        "album_type, album_subtype, album_name, tag_id, group_tag, cover_uri, is_cover_satisfied) "
        "VALUES (4096, 4103, 'test_group_photo_album_001', 'a|c,d', 'a|c,d', 'test_cover_uri', 1)");
}

static void InsertAnalysisPhotoMapInNewDb(std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    rdbPtr->ExecuteSql(INSERT_ANALYSIS_PHOTO_MAP + VALUES_BEGIN + "1, 1, 1" + VALUES_END);
    rdbPtr->ExecuteSql(INSERT_ANALYSIS_PHOTO_MAP + VALUES_BEGIN + "1, 2, 2" + VALUES_END);
}

static void PreProcessInNewDb(std::shared_ptr<Media::MediaLibraryRdbStore> mediaRdbPtr)
{
    std::shared_ptr<NativeRdb::RdbStore> rdbPtr = mediaRdbPtr->GetRaw();
    InsertPhotoInNewDb(rdbPtr);
    InsertAnalysisAlbumInNewDb(rdbPtr);
    InsertAnalysisPhotoMapInNewDb(rdbPtr);
}

static void CreateDataWithMerging(std::shared_ptr<Media::MediaLibraryRdbStore> mediaRdbPtr)
{
    std::shared_ptr<NativeRdb::RdbStore> rdbPtr = mediaRdbPtr->GetRaw();
    InsertPhotoInNewDb(rdbPtr);
    InsertAnalysisAlbumWithMerging(rdbPtr);
    InsertAnalysisPhotoMapInNewDb(rdbPtr);
}

static void ClearGroupPhotoData(std::shared_ptr<NativeRdb::RdbStore>& mediaRdbPtr)
{
    MEDIA_INFO_LOG("Start clear data");
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
    MEDIA_INFO_LOG("End clear data");
}

void CloneRestoreGroupPhotoTest::Init(CloneGroupPhotoSource &cloneGroupPhotoSource,
    const string &path, const vector<string> &tableList)
{
    MEDIA_INFO_LOG("Start init clone source database");
    cloneGroupPhotoSource.Init(path, tableList);
}

void CloneRestoreGroupPhotoTest::VerifyGroupAlbumRestore(const std::shared_ptr<NativeRdb::RdbStore>& db)
{
    std::string querySql = "SELECT * FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE album_type = " + std::to_string(SMART) +
        " AND album_subtype = " + std::to_string(GROUP_PHOTO) +
        " ORDER BY album_id ASC";

    std::shared_ptr<NativeRdb::ResultSet> resultSet = db->QuerySql(querySql);
    ASSERT_NE(resultSet, nullptr);

    int32_t count = 0;
    int32_t errCode = resultSet->GetRowCount(count);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(count, EXPECTED_GROUP_ALBUM_COUNT);
    EXPECT_TRUE(resultSet->GoToFirstRow() == NativeRdb::E_OK);

    int index;
    std::string columnValue;

    (void)resultSet->GetColumnIndex("album_name", index);
    resultSet->GetString(index, columnValue);
    EXPECT_EQ(columnValue, "test_group_photo_album_001");

    (void)resultSet->GetColumnIndex("tag_id", index);
    resultSet->GetString(index, columnValue);
    EXPECT_EQ(columnValue, "a|c,");

    (void)resultSet->GetColumnIndex("cover_uri", index);
    resultSet->GetString(index, columnValue);
    EXPECT_EQ(columnValue, "test_cover_uri");

    (void)resultSet->GetColumnIndex("is_cover_satisfied", index);
    int isCoverSatisfied;
    resultSet->GetInt(index, isCoverSatisfied);
    EXPECT_EQ(isCoverSatisfied, 1);

    EXPECT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
}

void CloneRestoreGroupPhotoTest::VerifyGroupPhotoAlbumWithoutData(const std::shared_ptr<NativeRdb::RdbStore>& db)
{
    std::string querySql = "SELECT * FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE album_type = " + std::to_string(SMART) +
        " AND album_subtype = " + std::to_string(GROUP_PHOTO);

    std::shared_ptr<NativeRdb::ResultSet> resultSet = db->QuerySql(querySql);
    ASSERT_NE(resultSet, nullptr);

    int32_t count = 0;
    int32_t errCode = resultSet->GetRowCount(count);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(count, 1);
    EXPECT_TRUE(resultSet->GoToFirstRow() == NativeRdb::E_OK);

    int index;
    std::string columnValue;

    (void)resultSet->GetColumnIndex("album_name", index);
    resultSet->GetString(index, columnValue);
    EXPECT_EQ(columnValue, "test_group_photo_album_001");

    (void)resultSet->GetColumnIndex("tag_id", index);
    resultSet->GetString(index, columnValue);
    EXPECT_EQ(columnValue, "a|c,");

    (void)resultSet->GetColumnIndex("cover_uri", index);
    resultSet->GetString(index, columnValue);
    EXPECT_EQ(columnValue, "test_cover_uri");

    (void)resultSet->GetColumnIndex("is_cover_satisfied", index);
    int isCoverSatisfied;
    resultSet->GetInt(index, isCoverSatisfied);
    EXPECT_EQ(isCoverSatisfied, 1);
}

void CloneRestoreGroupPhotoTest::VerifyGroupPhotoAlbumWithMerging(const std::shared_ptr<NativeRdb::RdbStore>& db)
{
    std::string querySql = "SELECT * FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE album_type = " + std::to_string(SMART) +
        " AND album_subtype = " + std::to_string(GROUP_PHOTO);

    std::shared_ptr<NativeRdb::ResultSet> resultSet = db->QuerySql(querySql);
    ASSERT_NE(resultSet, nullptr);

    int32_t count = 0;
    int32_t errCode = resultSet->GetRowCount(count);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(count, 1);
    EXPECT_TRUE(resultSet->GoToFirstRow() == NativeRdb::E_OK);

    int index;
    std::string columnValue;

    (void)resultSet->GetColumnIndex("album_name", index);
    resultSet->GetString(index, columnValue);
    EXPECT_EQ(columnValue, "test_group_photo_album_001");

    (void)resultSet->GetColumnIndex("tag_id", index);
    resultSet->GetString(index, columnValue);
    EXPECT_EQ(columnValue, "a|c,d");

    (void)resultSet->GetColumnIndex("group_tag", index);
    resultSet->GetString(index, columnValue);
    EXPECT_EQ(columnValue, "a|c,d");

    (void)resultSet->GetColumnIndex("cover_uri", index);
    resultSet->GetString(index, columnValue);
    EXPECT_EQ(columnValue, "test_cover_uri");

    (void)resultSet->GetColumnIndex("is_cover_satisfied", index);
    int isCoverSatisfied;
    resultSet->GetInt(index, isCoverSatisfied);
    EXPECT_EQ(isCoverSatisfied, 1);
}

void CloneRestoreGroupPhotoTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("Start CloneRestoreGroupPhotoTest::Init");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    MEDIA_INFO_LOG("Start init restoreService");
    restoreService = make_unique<CloneRestore>();
    restoreService->mediaLibraryRdb_ = g_rdbStore->GetRaw(); // destination database
}

void CloneRestoreGroupPhotoTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("CloneRestoreGroupPhotoTest::TearDownTestCase");
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    restoreService->mediaLibraryRdb_ = nullptr;
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void CloneRestoreGroupPhotoTest::SetUp()
{
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
}

void CloneRestoreGroupPhotoTest::TearDown(void) {}

HWTEST_F(CloneRestoreGroupPhotoTest, medialibrary_backup_clone_restore_group_photo_album_test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("Start medialibrary_backup_clone_restore_group_photo_album_test_001");
    vector<string> tableList = {PhotoColumn::PHOTOS_TABLE, ANALYSIS_ALBUM_TABLE, ANALYSIS_PHOTO_MAP_TABLE};
    Init(cloneSource, TEST_BACKUP_DB_PATH, tableList);
    PreProcessInNewDb(g_rdbStore);
    restoreService->mediaRdb_ = cloneSource.cloneStorePtr_;
    restoreService->mediaLibraryRdb_ = g_rdbStore->GetRaw();
    restoreService->photoInfoMap_ = PHOTO_INFO_MAP;
    restoreService->RestoreGroupPhoto();

    VerifyGroupAlbumRestore(restoreService->mediaLibraryRdb_);
}

HWTEST_F(CloneRestoreGroupPhotoTest, medialibrary_backup_clone_restore_group_photo_album_no_data_test_001,
    TestSize.Level2)
{
    MEDIA_INFO_LOG("Start medialibrary_backup_clone_restore_group_photo_album_no_data_test_001");
    EXPECT_NE(g_rdbStore, nullptr);
    EXPECT_NE(cloneSource.cloneStorePtr_, nullptr);
    ClearGroupPhotoData(cloneSource.cloneStorePtr_);
    std::shared_ptr<NativeRdb::RdbStore> rdbPtr = g_rdbStore->GetRaw();
    ClearGroupPhotoData(rdbPtr);
    PreProcessInNewDb(g_rdbStore);
    restoreService->mediaRdb_ = cloneSource.cloneStorePtr_;
    restoreService->mediaLibraryRdb_ = g_rdbStore->GetRaw();
    restoreService->photoInfoMap_ = PHOTO_INFO_MAP_NO_DATA;
    restoreService->RestoreGroupPhoto();
    VerifyGroupPhotoAlbumWithoutData(restoreService->mediaLibraryRdb_);
}

HWTEST_F(CloneRestoreGroupPhotoTest, medialibrary_backup_clone_restore_group_photo_album_merge_test_001,
    TestSize.Level2)
{
    MEDIA_INFO_LOG("Start medialibrary_backup_clone_restore_group_photo_album_merge_test_001");
    EXPECT_NE(g_rdbStore, nullptr);
    EXPECT_NE(cloneSource.cloneStorePtr_, nullptr);
    ClearGroupPhotoData(cloneSource.cloneStorePtr_);
    std::shared_ptr<NativeRdb::RdbStore> rdbPtr = g_rdbStore->GetRaw();
    ClearGroupPhotoData(rdbPtr);
    CreateDataWithMerging(g_rdbStore);
    restoreService->mediaRdb_ = cloneSource.cloneStorePtr_;
    restoreService->mediaLibraryRdb_ = g_rdbStore->GetRaw();
    restoreService->photoInfoMap_ = PHOTO_INFO_MAP_NO_DATA;
    restoreService->RestoreGroupPhoto();
    VerifyGroupPhotoAlbumWithMerging(restoreService->mediaLibraryRdb_);
}
}
}