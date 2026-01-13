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
#define MLOG_TAG "PortraitAlbumCloneTest"

#include "portrait_album_clone_test.h"
#include "portrait_album_source.h"

#include "vision_column.h"
#include "vision_photo_map_column.h"
#include "vision_db_sqls_more.h"
#include "vision_db_sqls.h"
#define private public
#define protected public
#include "clone_restore_portrait_album.h"
#include "clone_restore_portrait_base.h"
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
#include "media_upgrade.h"

using namespace OHOS::NativeRdb;
using namespace testing::ext;

namespace OHOS {
namespace Media {
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
static constexpr int32_t FILE_INFO_NEW_ID = 101;
static const string TEST_BACKUP_PATH = "/data/test/backup/db";
static const string TEST_DB_PATH = "/data/storage/el2/database/rdb/media_library.db";
static const string TEST_BACKUP_DB_PATH = TEST_BACKUP_PATH + TEST_DB_PATH;
static shared_ptr<MediaLibraryRdbStore> newRdbStore = nullptr;
static unique_ptr<CloneRestorePortrait> cloneRestorePortrait = nullptr;

static std::vector<std::string> createTableSqlLists = {
    PhotoUpgrade::CREATE_PHOTO_TABLE,
    CREATE_ANALYSIS_ALBUM_FOR_ONCREATE,
    CREATE_ANALYSIS_ALBUM_MAP,
    CREATE_TAB_FACE_TAG,
    CREATE_TAB_IMAGE_FACE,
};

static std::vector<std::string> testTables = {
    PhotoColumn::PHOTOS_TABLE,
    ANALYSIS_ALBUM_TABLE,
    ANALYSIS_PHOTO_MAP_TABLE,
    VISION_FACE_TAG_TABLE,
    VISION_IMAGE_FACE_TABLE,
};
const int32_t EXPECTED_MAP_ALBUM = 201;
const int32_t EXPECTED_MAP_ASSET = 101;

void PortraitAlbumCloneTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("Start PortraitAlbumCloneTest::Init");
    MediaLibraryUnitTestUtils::Init();
    newRdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(newRdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(newRdbStore, createTableSqlLists);
    cloneRestorePortrait = make_unique<CloneRestorePortrait>();
}

void PortraitAlbumCloneTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("PortraitAlbumCloneTest::TearDownTestCase");
    MediaLibraryUnitTestUtils::CleanTestTables(newRdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    cloneRestorePortrait = nullptr;
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void PortraitAlbumCloneTest::SetUp()
{
    MEDIA_INFO_LOG("enter PortraitAlbumCloneTest::SetUp");
    MediaLibraryUnitTestUtils::CleanTestTables(newRdbStore, testTables);
}

void PortraitAlbumCloneTest::TearDown() {}

void PortraitAlbumCloneTest::Init(PortraitAlbumSource &portraitAlbumSource, const string &path,
    const vector<string>& tableList)
{
    MEDIA_INFO_LOG("Start init clone source database");
    portraitAlbumSource.Init(path, tableList);
}

void PortraitAlbumCloneTest::SetupMockPortraitAlbumInfoMap(std::vector<AnalysisAlbumTbl> &portraitAlbumInfoMap)
{
    AnalysisAlbumTbl portraitAlbumTbl;
    portraitAlbumTbl.albumIdOld = 1;
    portraitAlbumTbl.albumIdNew = EXPECTED_MAP_ALBUM;
    portraitAlbumInfoMap.push_back(portraitAlbumTbl);
}

void PortraitAlbumCloneTest::VerifyMaps(const std::shared_ptr<NativeRdb::RdbStore>& db)
{
    std::string querySql = "SELECT * FROM " + ANALYSIS_PHOTO_MAP_TABLE +
        " WHERE " + MAP_ASSET + " = " + std::to_string(cloneRestorePortrait->photoInfoMap_[1].fileIdNew);

    std::shared_ptr<NativeRdb::ResultSet> resultSet = db->QuerySql(querySql);
    ASSERT_NE(resultSet, nullptr);

    EXPECT_TRUE(resultSet->GoToFirstRow() == NativeRdb::E_OK);

    int index;
    int map_album;
    int map_asset;
    int fileIdNew;

    (void)resultSet->GetColumnIndex("map_album", index);
    resultSet->GetInt(index, map_album);
    EXPECT_EQ(map_album, EXPECTED_MAP_ALBUM);

    (void)resultSet->GetColumnIndex("map_asset", index);
    resultSet->GetInt(index, map_asset);
    EXPECT_EQ(map_asset, EXPECTED_MAP_ASSET);

    EXPECT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
}

void PortraitAlbumCloneTest::SetupMockPhotoInfoMap(std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    PhotoInfo photoInfo;
    photoInfo.fileIdNew = FILE_INFO_NEW_ID;
    photoInfo.displayName = "test.jpg";
    photoInfo.cloudPath = "/storage/cloud/files/Photo/14/test.jpg";
    photoInfoMap.insert(std::make_pair(1, photoInfo));
}

void PortraitAlbumCloneTest::VerifyImageFaceRestore(const std::shared_ptr<NativeRdb::RdbStore>& db)
{
    std::string querySql = "SELECT * FROM " + VISION_IMAGE_FACE_TABLE +
        " WHERE " + IMAGE_FACE_COL_FILE_ID + " = " + std::to_string(cloneRestorePortrait->photoInfoMap_[1].fileIdNew);

    std::shared_ptr<NativeRdb::ResultSet> resultSet = db->QuerySql(querySql);
    ASSERT_NE(resultSet, nullptr);

    EXPECT_TRUE(resultSet->GoToFirstRow() == NativeRdb::E_OK);

    int index;
    int intValue;
    std::string stringValue;
    double doubleValue;

    (void)resultSet->GetColumnIndex(IMAGE_FACE_COL_FILE_ID, index);
    resultSet->GetInt(index, intValue);
    EXPECT_EQ(intValue, cloneRestorePortrait->photoInfoMap_[1].fileIdNew);

    (void)resultSet->GetColumnIndex(IMAGE_FACE_COL_FACE_ID, index);
    resultSet->GetString(index, stringValue);
    EXPECT_EQ(stringValue, "test_face_id");

    (void)resultSet->GetColumnIndex(IMAGE_FACE_COL_TAG_ID, index);
    resultSet->GetString(index, stringValue);
    EXPECT_EQ(stringValue, "test_tag_id");

    (void)resultSet->GetColumnIndex(IMAGE_FACE_COL_SCALE_X, index);
    resultSet->GetDouble(index, doubleValue);
    EXPECT_DOUBLE_EQ(doubleValue, 1.0);

    (void)resultSet->GetColumnIndex(IMAGE_FACE_COL_SCALE_Y, index);
    resultSet->GetDouble(index, doubleValue);
    EXPECT_DOUBLE_EQ(doubleValue, 1.0);

    EXPECT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
}

void ClearCloneSource(PortraitAlbumSource &cloneSource, const string &dbPath)
{
    cloneSource.cloneStorePtr_ = nullptr;
    NativeRdb::RdbHelper::DeleteRdbStore(dbPath);
}

void PortraitAlbumCloneTest::VerifyPortraitAlbumRestore(const std::shared_ptr<NativeRdb::RdbStore>& db)
{
    std::string querySql = "SELECT * FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE album_type = " + std::to_string(SMART) +
        " AND album_subtype = " + std::to_string(PORTRAIT);

    std::shared_ptr<NativeRdb::ResultSet> resultSet = db->QuerySql(querySql);
    ASSERT_NE(resultSet, nullptr);

    EXPECT_TRUE(resultSet->GoToFirstRow() == NativeRdb::E_OK);

    int index;
    std::string columnValue;

    (void)resultSet->GetColumnIndex("album_name", index);
    resultSet->GetString(index, columnValue);
    EXPECT_EQ(columnValue, "test_portrait_album_001");

    (void)resultSet->GetColumnIndex("tag_id", index);
    resultSet->GetString(index, columnValue);
    EXPECT_EQ(columnValue, "test_tag_id");

    EXPECT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
}

void PortraitAlbumCloneTest::VerifyPortraitClusteringRestore(const std::shared_ptr<NativeRdb::RdbStore>& db)
{
    std::string querySql = "SELECT * FROM " + VISION_FACE_TAG_TABLE;

    std::shared_ptr<NativeRdb::ResultSet> resultSet = db->QuerySql(querySql);
    ASSERT_NE(resultSet, nullptr);

    EXPECT_TRUE(resultSet->GoToFirstRow() == NativeRdb::E_OK);

    int index;
    std::string stringValue;
    int intValue;

    (void)resultSet->GetColumnIndex(FACE_TAG_COL_TAG_ID, index);
    resultSet->GetString(index, stringValue);
    EXPECT_EQ(stringValue, "test_tag_id");

    (void)resultSet->GetColumnIndex(FACE_TAG_COL_TAG_NAME, index);
    resultSet->GetString(index, stringValue);
    EXPECT_EQ(stringValue, "test_face_tag_name");

    (void)resultSet->GetColumnIndex(FACE_TAG_COL_CENTER_FEATURES, index);
    resultSet->GetString(index, stringValue);
    EXPECT_EQ(stringValue, "test_center_features");

    (void)resultSet->GetColumnIndex(FACE_TAG_COL_TAG_VERSION, index);
    resultSet->GetInt(index, intValue);
    EXPECT_EQ(intValue, 1);

    (void)resultSet->GetColumnIndex(FACE_TAG_COL_ANALYSIS_VERSION, index);
    resultSet->GetInt(index, intValue);
    EXPECT_EQ(intValue, 1);

    EXPECT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
}

HWTEST_F(PortraitAlbumCloneTest, medialibrary_backup_clone_restore_portrait_album_test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("Start medialibrary_backup_clone_restore_portrait_album_test_001");
    PortraitAlbumSource portraitAlbumSource;
    vector<string> tableList = { ANALYSIS_ALBUM_TABLE, PhotoColumn::PHOTOS_TABLE, ANALYSIS_PHOTO_MAP_TABLE };
    Init(portraitAlbumSource, TEST_BACKUP_DB_PATH, tableList);
    cloneRestorePortrait = make_unique<CloneRestorePortrait>();
    cloneRestorePortrait->Init(CLONE_RESTORE_ID, "", newRdbStore->GetRaw(), portraitAlbumSource.cloneStorePtr_, {}, false);
    cloneRestorePortrait->RestoreFromGalleryPortraitAlbum();
    VerifyPortraitAlbumRestore(newRdbStore->GetRaw());
    cloneRestorePortrait = nullptr;
    ClearCloneSource(portraitAlbumSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(PortraitAlbumCloneTest, medialibrary_backup_clone_restore_portrait_clustering_test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("Start medialibrary_backup_clone_restore_portrait_clustering_test_001");
    PortraitAlbumSource portraitAlbumSource;
    vector<string> tableList = { VISION_FACE_TAG_TABLE, PhotoColumn::PHOTOS_TABLE,
        ANALYSIS_ALBUM_TABLE, ANALYSIS_PHOTO_MAP_TABLE };
    Init(portraitAlbumSource, TEST_BACKUP_DB_PATH, tableList);
    cloneRestorePortrait = make_unique<CloneRestorePortrait>();
    cloneRestorePortrait->Init(CLONE_RESTORE_ID, "", newRdbStore->GetRaw(), portraitAlbumSource.cloneStorePtr_, {}, false);
    cloneRestorePortrait->RestorePortraitClusteringInfo();
    VerifyPortraitClusteringRestore(newRdbStore->GetRaw());
    cloneRestorePortrait = nullptr;
    ClearCloneSource(portraitAlbumSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(PortraitAlbumCloneTest, medialibrary_backup_clone_restore_image_face_test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("Start medialibrary_backup_clone_restore_image_face_test_001");
    PortraitAlbumSource portraitAlbumSource;
    vector<string> tableList = { VISION_IMAGE_FACE_TABLE };
    Init(portraitAlbumSource, TEST_BACKUP_DB_PATH, tableList);
    std::unordered_map<int32_t, PhotoInfo> photoInfoMap;
    SetupMockPhotoInfoMap(photoInfoMap);
    cloneRestorePortrait = make_unique<CloneRestorePortrait>();
    cloneRestorePortrait->Init(CLONE_RESTORE_ID, "", newRdbStore->GetRaw(),
        portraitAlbumSource.cloneStorePtr_, photoInfoMap, false);
    cloneRestorePortrait->RestoreImageFaceInfo();
    VerifyImageFaceRestore(newRdbStore->GetRaw());
    cloneRestorePortrait = nullptr;
    ClearCloneSource(portraitAlbumSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(PortraitAlbumCloneTest, medialibrary_backup_clone_restore_photo_map_test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("Start medialibrary_backup_clone_restore_photo_map_test_001");
    PortraitAlbumSource portraitAlbumSource;
    vector<string> tableList = { ANALYSIS_ALBUM_TABLE, ANALYSIS_PHOTO_MAP_TABLE };
    Init(portraitAlbumSource, TEST_BACKUP_DB_PATH, tableList);
    std::unordered_map<int32_t, PhotoInfo> photoInfoMap;
    SetupMockPhotoInfoMap(photoInfoMap);
    std::vector<AnalysisAlbumTbl> portraitAlbumInfoMap;
    SetupMockPortraitAlbumInfoMap(portraitAlbumInfoMap);
    cloneRestorePortrait = make_unique<CloneRestorePortrait>();
    cloneRestorePortrait->Init(CLONE_RESTORE_ID, "",
        newRdbStore->GetRaw(), portraitAlbumSource.cloneStorePtr_, photoInfoMap, false);
    cloneRestorePortrait->portraitAlbumInfoMap_ = portraitAlbumInfoMap;
    cloneRestorePortrait->RestoreMaps();
    VerifyMaps(newRdbStore->GetRaw());
    cloneRestorePortrait = nullptr;
    ClearCloneSource(portraitAlbumSource, TEST_BACKUP_DB_PATH);
}
}
}