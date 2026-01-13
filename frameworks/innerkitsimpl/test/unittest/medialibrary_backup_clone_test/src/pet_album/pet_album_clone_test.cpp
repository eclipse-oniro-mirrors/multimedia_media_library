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
#define MLOG_TAG "PetAlbumCloneTest"

#include "pet_album_clone_test.h"
#include "pet_album_source.h"

#include "vision_column.h"
#include "vision_photo_map_column.h"
#include "vision_db_sqls_more.h"
#include "vision_db_sqls.h"
#define private public
#define protected public
#include "clone_restore_pet_album.h"
#include "clone_restore_pet_base.h"
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
static unique_ptr<CloneRestorePet> cloneRestorePet = nullptr;

static std::vector<std::string> createTableSqlLists = {
    PhotoUpgrade::CREATE_PHOTO_TABLE,
    CREATE_ANALYSIS_ALBUM_FOR_ONCREATE,
    CREATE_ANALYSIS_ALBUM_MAP,
    CREATE_TAB_ANALYSIS_PET_TAG,
    CREATE_TAB_ANALYSIS_PET_FACE,
};

static std::vector<std::string> testTables = {
    PhotoColumn::PHOTOS_TABLE,
    ANALYSIS_ALBUM_TABLE,
    ANALYSIS_PHOTO_MAP_TABLE,
    VISION_PET_TAG_TABLE,
    VISION_PET_FACE_TABLE,
};
const int32_t EXPECTED_MAP_ALBUM = 201;
const int32_t EXPECTED_MAP_ASSET = 101;

void PetAlbumCloneTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("Start PetAlbumCloneTest::Init");
    MediaLibraryUnitTestUtils::Init();
    newRdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(newRdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(newRdbStore, createTableSqlLists);
    cloneRestorePet = make_unique<CloneRestorePet>();
}

void PetAlbumCloneTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("PetAlbumCloneTest::TearDownTestCase");
    MediaLibraryUnitTestUtils::CleanTestTables(newRdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    cloneRestorePet = nullptr;
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void PetAlbumCloneTest::SetUp()
{
    MEDIA_INFO_LOG("enter PetAlbumCloneTest::SetUp");
    MediaLibraryUnitTestUtils::CleanTestTables(newRdbStore, testTables);
}

void PetAlbumCloneTest::TearDown() {}

void PetAlbumCloneTest::Init(PetAlbumSource &petAlbumSource, const string &path,
    const vector<string>& tableList)
{
    MEDIA_INFO_LOG("Start init clone source database");
    petAlbumSource.Init(path, tableList);
}

void PetAlbumCloneTest::SetupMockPetAlbumInfoMap(std::vector<AnalysisAlbumTbl> &petAlbumInfoMap)
{
    AnalysisAlbumTbl petAlbumTbl;
    petAlbumTbl.albumIdOld = 1;
    petAlbumTbl.albumIdNew = EXPECTED_MAP_ALBUM;
    petAlbumInfoMap.push_back(petAlbumTbl);
}

void PetAlbumCloneTest::SetupMockPhotoInfoMap(std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    PhotoInfo photoInfo;
    photoInfo.fileIdNew = FILE_INFO_NEW_ID;
    photoInfo.displayName = "test.jpg";
    photoInfo.cloudPath = "/storage/cloud/files/Photo/14/test.jpg";
    photoInfoMap.insert(std::make_pair(1, photoInfo));
}

void ClearCloneSource(PetAlbumSource &cloneSource, const string &dbPath)
{
    cloneSource.cloneStorePtr_ = nullptr;
    NativeRdb::RdbHelper::DeleteRdbStore(dbPath);
}

HWTEST_F(PetAlbumCloneTest, medialibrary_backup_clone_restore_pet_album_test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("Start medialibrary_backup_clone_restore_pet_album_test_001");
    PetAlbumSource petAlbumSource;
    vector<string> tableList = { ANALYSIS_ALBUM_TABLE, PhotoColumn::PHOTOS_TABLE, ANALYSIS_PHOTO_MAP_TABLE };
    Init(petAlbumSource, TEST_BACKUP_DB_PATH, tableList);
    cloneRestorePet = make_unique<CloneRestorePet>();
    cloneRestorePet->Init(CLONE_RESTORE_ID, "", newRdbStore->GetRaw(), petAlbumSource.cloneStorePtr_, {}, false);
    cloneRestorePet->totalPetAlbumNumber_ = 1;
    cloneRestorePet->RestoreFromGalleryPetAlbum();
    auto db = newRdbStore->GetRaw();
    EXPECT_NE(db, nullptr);
    std::string querySql = "SELECT * FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE album_type = " + std::to_string(SMART) +
        " AND album_subtype = " + std::to_string(PET);

    std::shared_ptr<NativeRdb::ResultSet> resultSet = db->QuerySql(querySql);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_TRUE(resultSet->GoToFirstRow() == NativeRdb::E_OK);

    int index;
    std::string columnValue;
    (void)resultSet->GetColumnIndex("album_name", index);
    resultSet->GetString(index, columnValue);
    EXPECT_EQ(columnValue, "test_pet_album_001");

    (void)resultSet->GetColumnIndex("tag_id", index);
    resultSet->GetString(index, columnValue);
    EXPECT_EQ(columnValue, "test_tag_id");

    EXPECT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    cloneRestorePet = nullptr;
    ClearCloneSource(petAlbumSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(PetAlbumCloneTest, medialibrary_backup_clone_restore_pet_clustering_test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("Start medialibrary_backup_clone_restore_pet_clustering_test_001");
    PetAlbumSource petAlbumSource;
    vector<string> tableList = { VISION_PET_TAG_TABLE, PhotoColumn::PHOTOS_TABLE,
        ANALYSIS_ALBUM_TABLE, ANALYSIS_PHOTO_MAP_TABLE };
    Init(petAlbumSource, TEST_BACKUP_DB_PATH, tableList);
    cloneRestorePet = make_unique<CloneRestorePet>();
    cloneRestorePet->Init(CLONE_RESTORE_ID, "", newRdbStore->GetRaw(), petAlbumSource.cloneStorePtr_, {}, false);
    cloneRestorePet->RestorePetClusteringInfo();
    auto db = newRdbStore->GetRaw();
    EXPECT_NE(db, nullptr);
    std::string querySql = "SELECT * FROM " + VISION_PET_TAG_TABLE;
    std::shared_ptr<NativeRdb::ResultSet> resultSet = db->QuerySql(querySql);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_TRUE(resultSet->GoToFirstRow() == NativeRdb::E_OK);

    int index;
    std::string stringValue;
    int intValue;
    (void)resultSet->GetColumnIndex(PET_TAG_COL_TAG_ID, index);
    resultSet->GetString(index, stringValue);
    EXPECT_EQ(stringValue, "test_tag_id");

    (void)resultSet->GetColumnIndex(PET_TAG_COL_CENTER_FEATURES, index);
    resultSet->GetString(index, stringValue);
    EXPECT_EQ(stringValue, "test_center_features");

    (void)resultSet->GetColumnIndex(PET_TAG_COL_TAG_VERSION, index);
    resultSet->GetString(index, stringValue);
    EXPECT_EQ(stringValue, "tag_version");

    (void)resultSet->GetColumnIndex(PET_TAG_COL_ANALYSIS_VERSION, index);
    resultSet->GetString(index, stringValue);
    EXPECT_EQ(stringValue, "1.0");

    (void)resultSet->GetColumnIndex(PET_TAG_COL_PET_LABEL, index);
    resultSet->GetInt(index, intValue);
    EXPECT_EQ(intValue, 9); // 9: cat

    EXPECT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    cloneRestorePet = nullptr;
    ClearCloneSource(petAlbumSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(PetAlbumCloneTest, medialibrary_backup_clone_restore_pet_face_test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("Start medialibrary_backup_clone_restore_pet_face_test_001");
    PetAlbumSource petAlbumSource;
    vector<string> tableList = { VISION_PET_FACE_TABLE };
    Init(petAlbumSource, TEST_BACKUP_DB_PATH, tableList);
    std::unordered_map<int32_t, PhotoInfo> photoInfoMap;
    SetupMockPhotoInfoMap(photoInfoMap);
    cloneRestorePet = make_unique<CloneRestorePet>();
    cloneRestorePet->Init(CLONE_RESTORE_ID, "", newRdbStore->GetRaw(),
        petAlbumSource.cloneStorePtr_, photoInfoMap, false);
    cloneRestorePet->RestorePetFaceInfo();
    auto db = newRdbStore->GetRaw();
    EXPECT_NE(db, nullptr);
    std::string querySql = "SELECT * FROM " + VISION_PET_FACE_TABLE +
        " WHERE " + PET_FACE_COL_FILE_ID + " = " + std::to_string(cloneRestorePet->photoInfoMap_[1].fileIdNew);

    std::shared_ptr<NativeRdb::ResultSet> resultSet = db->QuerySql(querySql);
    ASSERT_NE(resultSet, nullptr);

    EXPECT_TRUE(resultSet->GoToFirstRow() == NativeRdb::E_OK);

    int index;
    int intValue;
    std::string stringValue;
    double doubleValue;

    (void)resultSet->GetColumnIndex(PET_FACE_COL_FILE_ID, index);
    resultSet->GetInt(index, intValue);
    EXPECT_EQ(intValue, cloneRestorePet->photoInfoMap_[1].fileIdNew);

    (void)resultSet->GetColumnIndex(PET_FACE_COL_PET_ID, index);
    resultSet->GetInt(index, intValue);
    EXPECT_EQ(intValue, 1);

    (void)resultSet->GetColumnIndex(PET_FACE_COL_PET_TAG_ID, index);
    resultSet->GetString(index, stringValue);
    EXPECT_EQ(stringValue, "test_tag_id");

    (void)resultSet->GetColumnIndex(PET_FACE_COL_SCALE_X, index);
    resultSet->GetDouble(index, doubleValue);
    EXPECT_DOUBLE_EQ(doubleValue, 1.0);

    (void)resultSet->GetColumnIndex(PET_FACE_COL_SCALE_Y, index);
    resultSet->GetDouble(index, doubleValue);
    EXPECT_DOUBLE_EQ(doubleValue, 1.0);

    EXPECT_FALSE(resultSet->GoToNextRow() == NativeRdb::E_OK);
    cloneRestorePet = nullptr;
    ClearCloneSource(petAlbumSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(PetAlbumCloneTest, medialibrary_backup_clone_restore_photo_map_test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("Start medialibrary_backup_clone_restore_photo_map_test_001");
    PetAlbumSource petAlbumSource;
    vector<string> tableList = { ANALYSIS_ALBUM_TABLE, ANALYSIS_PHOTO_MAP_TABLE };
    Init(petAlbumSource, TEST_BACKUP_DB_PATH, tableList);
    std::unordered_map<int32_t, PhotoInfo> photoInfoMap;
    SetupMockPhotoInfoMap(photoInfoMap);
    std::vector<AnalysisAlbumTbl> petAlbumInfoMap;
    SetupMockPetAlbumInfoMap(petAlbumInfoMap);
    cloneRestorePet = make_unique<CloneRestorePet>();
    cloneRestorePet->Init(CLONE_RESTORE_ID, "",
        newRdbStore->GetRaw(), petAlbumSource.cloneStorePtr_, photoInfoMap, false);
    cloneRestorePet->PetAlbumInfoMap_ = petAlbumInfoMap;
    cloneRestorePet->RestoreMaps();
    auto db = newRdbStore->GetRaw();
    EXPECT_NE(db, nullptr);
    std::string querySql = "SELECT * FROM " + ANALYSIS_PHOTO_MAP_TABLE +
        " WHERE " + MAP_ASSET + " = " + std::to_string(cloneRestorePet->photoInfoMap_[1].fileIdNew);

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
    cloneRestorePet = nullptr;
    ClearCloneSource(petAlbumSource, TEST_BACKUP_DB_PATH);
}
}
}