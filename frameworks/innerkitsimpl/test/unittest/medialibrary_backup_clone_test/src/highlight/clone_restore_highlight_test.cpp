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

#define MLOG_TAG "BackupCloneTest"

#include "clone_restore_highlight_test.h"
#include "clone_highlight_source.h"
#include "media_file_utils.h"
#include "vision_db_sqls.h"
#include "vision_db_sqls_more.h"
#include "story_db_sqls.h"

#define private public
#define protected public
#include "backup_const.h"
#include "clone_restore_cv_analysis.h"
#include "clone_restore_highlight.h"
#include "clone_restore.h"
#include "media_column.h"
#include "media_log.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "story_album_column.h"
#include "story_cover_info_column.h"
#include "story_play_info_column.h"
#undef protected
#undef private
#include "media_upgrade.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {

static std::vector<std::string> createTableSqlLists = {
    PhotoUpgrade::CREATE_PHOTO_TABLE,
    CREATE_ANALYSIS_ALBUM_FOR_ONCREATE,
    CREATE_ANALYSIS_ALBUM_MAP,
    CREATE_HIGHLIGHT_ALBUM_TABLE,
    CREATE_HIGHLIGHT_COVER_INFO_TABLE,
    CREATE_HIGHLIGHT_PLAY_INFO_TABLE,
    CREATE_ANALYSIS_ASSET_SD_MAP_TABLE,
    CREATE_ANALYSIS_ALBUM_ASET_MAP_TABLE,
    CREATE_TAB_ANALYSIS_LABEL,
    CREATE_TAB_ANALYSIS_RECOMMENDATION,
    CREATE_TAB_ANALYSIS_SALIENCY_DETECT,
};

static std::vector<std::string> testTables = {
    PhotoColumn::PHOTOS_TABLE,
    ANALYSIS_ALBUM_TABLE,
    ANALYSIS_PHOTO_MAP_TABLE,
    HIGHLIGHT_ALBUM_TABLE,
    HIGHLIGHT_COVER_INFO_TABLE,
    HIGHLIGHT_PLAY_INFO_TABLE,
    ANALYSIS_ASSET_SD_MAP_TABLE,
    ANALYSIS_ALBUM_ASSET_MAP_TABLE,
    VISION_LABEL_TABLE,
    VISION_RECOMMENDATION_TABLE,
    VISION_SALIENCY_TABLE,
};

const string TEST_BACKUP_PATH = "/data/test/backup/db";
const string TEST_DB_PATH = "/data/storage/el2/database/rdb/media_library.db";
const string TEST_BACKUP_DB_PATH = TEST_BACKUP_PATH + TEST_DB_PATH;
const string NONE_CONDITION = "1";
const int32_t TEST_ID = 1;
const int32_t TEST_NEW_ID = 2;
const int32_t TEST_NUM = 3;
const int32_t INVALID_COUNT = -1;
const std::string HIGHLIGHT_ASSET_URI_PREFIX = "file://media/highlight/video/";
const std::unordered_map<int32_t, PhotoInfo> PHOTO_INFO_MAP = {
    { TEST_ID, PhotoInfo({ TEST_NEW_ID, MediaType::MEDIA_TYPE_IMAGE, "test.jpg", "/Photo/1/test.jpg" }) },
};

static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

shared_ptr<MediaLibraryRdbStore> newRdbStore;
unique_ptr<CloneRestoreHighlight> cloneRestoreHighlight = nullptr;
unique_ptr<CloneRestoreCVAnalysis> cloneRestoreCVAnalysis = nullptr;
unordered_map<int32_t, PhotoInfo> photoInfoMap;

void ExecuteRdbSqls(shared_ptr<NativeRdb::RdbStore> store, const vector<string> &sqls)
{
    for (const auto &sql : sqls) {
        int32_t errCode = store->ExecuteSql(sql);
        if (errCode == E_OK) {
            continue;
        }
        MEDIA_ERR_LOG("Execute %{public}s failed: %{public}d", sql.c_str(), errCode);
    }
}

void ClearHighlightData()
{
    MEDIA_INFO_LOG("Start clear data");
    MediaLibraryUnitTestUtils::CleanTestTables(newRdbStore, testTables);
    if (cloneRestoreHighlight) {
        cloneRestoreHighlight->analysisInfos_.clear();
        cloneRestoreHighlight->highlightInfos_.clear();
        cloneRestoreHighlight->coverInfos_.clear();
        cloneRestoreHighlight->playInfos_.clear();
        cloneRestoreHighlight->albumPhotoCounter_.clear();
        cloneRestoreHighlight->intersectionMap_.clear();
        cloneRestoreHighlight->photoInfoMap_.clear();
        cloneRestoreHighlight->lastIdOfMap_ = 0;
    }
    if (cloneRestoreCVAnalysis) {
        cloneRestoreCVAnalysis->assetUriMap_.clear();
    }
    MEDIA_INFO_LOG("End clear data");
}

void Init(CloneHighlightSource &cloneHighlightSource, const string &path, const vector<string> &tableList)
{
    MEDIA_INFO_LOG("Start init clone source database");
    cloneHighlightSource.Init(path, tableList);
}

void CloneRestoreHighlightTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("Start CloneRestoreHighlightTest Init");
    MediaLibraryUnitTestUtils::Init();
    newRdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(newRdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(newRdbStore, createTableSqlLists);
    cloneRestoreHighlight = make_unique<CloneRestoreHighlight>();
    cloneRestoreHighlight->mediaLibraryRdb_ = newRdbStore->GetRaw(); // destination database
    cloneRestoreCVAnalysis = make_unique<CloneRestoreCVAnalysis>();
    cloneRestoreCVAnalysis->mediaLibraryRdb_ = newRdbStore->GetRaw(); // destination database
}

void CloneRestoreHighlightTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
    MediaLibraryUnitTestUtils::CleanTestTables(newRdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    cloneRestoreHighlight->mediaLibraryRdb_ = nullptr;
    cloneRestoreCVAnalysis->mediaLibraryRdb_ = nullptr;
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void CloneRestoreHighlightTest::SetUp()
{
    ClearHighlightData();
}

void CloneRestoreHighlightTest::TearDown(void) {}

void ClearCloneSource(CloneHighlightSource &cloneHighlightSource, const string &dbPath)
{
    cloneHighlightSource.cloneStorePtr_ = nullptr;
    NativeRdb::RdbHelper::DeleteRdbStore(dbPath);
}

void QueryIntBySql(shared_ptr<NativeRdb::RdbStore> rdbStore, const string &querySql, const string &columnName,
    int32_t &result)
{
    ASSERT_NE(rdbStore, nullptr);
    auto resultSet = rdbStore->QuerySql(querySql);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    result = GetInt32Val(columnName, resultSet);
    MEDIA_INFO_LOG("Query %{public}s result: %{public}d", querySql.c_str(), result);
}

int32_t GetAlbumCountByCondition(shared_ptr<NativeRdb::RdbStore> rdbStore, const string &tableName,
    const string condition)
{
    string querySql = "SELECT " + MEDIA_COLUMN_COUNT_1 + " FROM " + tableName + " WHERE " + condition;
    int32_t result = INVALID_COUNT;
    QueryIntBySql(rdbStore, querySql, MEDIA_COLUMN_COUNT_1, result);
    return result;
}

void InsertIntoHighlightAlbumInfo(CloneRestoreHighlight::HighlightAlbumInfo &highlightAlbumInfo)
{
    highlightAlbumInfo.highlightIdOld = TEST_NUM;
    highlightAlbumInfo.highlightIdNew = TEST_NUM;
    highlightAlbumInfo.albumIdOld = TEST_NUM;
    highlightAlbumInfo.albumIdNew = TEST_NUM;
    highlightAlbumInfo.clusterType = "cluster_type_data";
    highlightAlbumInfo.clusterSubType = "cluster_sub_type_data";
    highlightAlbumInfo.clusterCondition = "cluster_condition_data";
    highlightAlbumInfo.highlightVersion = TEST_NUM;
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_highlight_restore_albums_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_highlight_restore_albums_test_001 start");
    CloneHighlightSource cloneHighlightSource;
    vector<string> tableList = { PhotoColumn::PHOTOS_TABLE, ANALYSIS_ALBUM_TABLE, ANALYSIS_PHOTO_MAP_TABLE,
        HIGHLIGHT_ALBUM_TABLE, HIGHLIGHT_COVER_INFO_TABLE, HIGHLIGHT_PLAY_INFO_TABLE };
    Init(cloneHighlightSource, TEST_BACKUP_DB_PATH, tableList);
    CloneRestoreHighlight::InitInfo initInfo = {
        CLONE_RESTORE_ID, "", newRdbStore->GetRaw(), cloneHighlightSource.cloneStorePtr_, "", PHOTO_INFO_MAP
    };
    cloneRestoreHighlight->Init(initInfo);
    cloneRestoreHighlight->Preprocess();
    cloneRestoreHighlight->RestoreAlbums();
    EXPECT_EQ(cloneRestoreHighlight->isMapOrder_, true);
    string analysisCondition = "album_name = 'test_highlight_album'";
    int32_t analysisCount = GetAlbumCountByCondition(newRdbStore->GetRaw(), ANALYSIS_ALBUM_TABLE, analysisCondition);
    EXPECT_EQ(analysisCount, 2);
    int32_t highlightCount = GetAlbumCountByCondition(newRdbStore->GetRaw(), HIGHLIGHT_ALBUM_TABLE, NONE_CONDITION);
    EXPECT_EQ(highlightCount, 1);
    int32_t coverCount = GetAlbumCountByCondition(newRdbStore->GetRaw(), HIGHLIGHT_COVER_INFO_TABLE, NONE_CONDITION);
    EXPECT_EQ(coverCount, 0);
    int32_t playCount = GetAlbumCountByCondition(newRdbStore->GetRaw(), HIGHLIGHT_PLAY_INFO_TABLE, NONE_CONDITION);
    EXPECT_EQ(playCount, 1);
    ClearCloneSource(cloneHighlightSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_highlight_restore_maps_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_highlight_restore_maps_test_001 start");
    CloneHighlightSource cloneHighlightSource;
    vector<string> tableList = { PhotoColumn::PHOTOS_TABLE, ANALYSIS_ALBUM_TABLE, ANALYSIS_PHOTO_MAP_TABLE,
        HIGHLIGHT_ALBUM_TABLE };
    Init(cloneHighlightSource, TEST_BACKUP_DB_PATH, tableList);
    CloneRestoreHighlight::InitInfo initInfo = {
        CLONE_RESTORE_ID, "", newRdbStore->GetRaw(), cloneHighlightSource.cloneStorePtr_, "", PHOTO_INFO_MAP
    };
    cloneRestoreHighlight->Init(initInfo);
    cloneRestoreHighlight->Preprocess();
    cloneRestoreHighlight->isMapOrder_ = true;

    CloneRestoreHighlight::AnalysisAlbumInfo testAnalysisInfo;
    testAnalysisInfo.albumIdOld = make_optional<int32_t>(TEST_ID);
    testAnalysisInfo.albumIdNew = make_optional<int32_t>(TEST_NEW_ID);
    testAnalysisInfo.oldCoverUri = make_optional<string>("file://media/Photo/1/test/IMG_00000000_000000.jpg");
    testAnalysisInfo.albumName = make_optional<string>("testAlbumName");
    cloneRestoreHighlight->analysisInfos_.emplace_back(testAnalysisInfo);
    cloneRestoreHighlight->RestoreMaps();
    string mapCondition = "map_album = 2 AND map_asset = 2";
    int32_t mapCount = GetAlbumCountByCondition(newRdbStore->GetRaw(), ANALYSIS_PHOTO_MAP_TABLE, mapCondition);
    EXPECT_EQ(mapCount, 1);
    ClearCloneSource(cloneHighlightSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_highlight_update_values_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_highlight_update_values_test_001 start");
    CloneHighlightSource cloneHighlightSource;
    vector<string> tableList = { PhotoColumn::PHOTOS_TABLE, ANALYSIS_ALBUM_TABLE, ANALYSIS_PHOTO_MAP_TABLE,
        HIGHLIGHT_ALBUM_TABLE };
    Init(cloneHighlightSource, TEST_BACKUP_DB_PATH, tableList);
    CloneRestoreHighlight::InitInfo initInfo = {
        CLONE_RESTORE_ID, "", newRdbStore->GetRaw(), cloneHighlightSource.cloneStorePtr_, "", PHOTO_INFO_MAP
    };
    cloneRestoreHighlight->Init(initInfo);
    cloneRestoreHighlight->Preprocess();
    cloneRestoreHighlight->isMapOrder_ = false;

    CloneRestoreHighlight::AnalysisAlbumInfo testAnalysisInfo;
    testAnalysisInfo.albumIdOld = make_optional<int32_t>(TEST_ID);
    testAnalysisInfo.albumIdNew = make_optional<int32_t>(TEST_NEW_ID);
    cloneRestoreHighlight->analysisInfos_.emplace_back(testAnalysisInfo);

    vector<NativeRdb::ValuesBucket> values;
    cloneRestoreHighlight->UpdateMapInsertValues(values);
    string mapCondition = "map_album = 2 AND map_asset = 2";
    int32_t mapCount = GetAlbumCountByCondition(newRdbStore->GetRaw(), ANALYSIS_PHOTO_MAP_TABLE, mapCondition);
    EXPECT_EQ(values.empty(), false);
    ClearCloneSource(cloneHighlightSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_highlight_update_albums_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_highlight_update_albums_test_001 start");
    CloneHighlightSource cloneHighlightSource;
    vector<string> tableList = { ANALYSIS_ALBUM_TABLE, ANALYSIS_PHOTO_MAP_TABLE, HIGHLIGHT_COVER_INFO_TABLE };
    cloneHighlightSource.Insert(tableList, newRdbStore->GetRaw());
    CloneRestoreHighlight::InitInfo initInfo = {
        CLONE_RESTORE_ID, "", newRdbStore->GetRaw(), cloneHighlightSource.cloneStorePtr_, "", PHOTO_INFO_MAP
    };
    cloneRestoreHighlight->Init(initInfo);
    CloneRestoreHighlight::AnalysisAlbumInfo testAnalysisInfo;
    testAnalysisInfo.oldCoverUri = make_optional<string>("file://testOldCoverUri");
    testAnalysisInfo.albumIdNew = make_optional<int32_t>(TEST_ID);
    testAnalysisInfo.coverUri = "file://testNewCoverUri";
    testAnalysisInfo.highlightIdNew = make_optional<int32_t>(TEST_ID);
    testAnalysisInfo.albumName = make_optional<string>("testAlbumName");
    cloneRestoreHighlight->analysisInfos_.emplace_back(testAnalysisInfo);

    CloneRestoreHighlight::HighlightCoverInfo testCoverInfo;
    testCoverInfo.ratio = make_optional<string>("1_1");
    testCoverInfo.highlightIdNew = make_optional<int32_t>(TEST_ID);
    cloneRestoreHighlight->coverInfos_.emplace_back(testCoverInfo);

    cloneRestoreHighlight->UpdateAlbums();
    string analysisCondition = "cover_uri = 'file://testNewCoverUri'";
    int32_t analysisCount = GetAlbumCountByCondition(newRdbStore->GetRaw(), ANALYSIS_ALBUM_TABLE, analysisCondition);
    EXPECT_EQ(analysisCount, 1);
    string coverCondition = "cover_key = 'testAlbumName_1_1_file://testNewCoverUri'";
    int32_t coverCount = GetAlbumCountByCondition(newRdbStore->GetRaw(), HIGHLIGHT_COVER_INFO_TABLE, coverCondition);
    EXPECT_EQ(coverCount, 0);
    ClearCloneSource(cloneHighlightSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_highlight_update_albums_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_highlight_update_albums_test_002 start");
    CloneHighlightSource cloneHighlightSource;
    vector<string> tableList = { ANALYSIS_ALBUM_TABLE, ANALYSIS_PHOTO_MAP_TABLE, HIGHLIGHT_COVER_INFO_TABLE };
    cloneHighlightSource.Insert(tableList, newRdbStore->GetRaw());
    CloneRestoreHighlight::InitInfo initInfo = {
        CLONE_RESTORE_ID, "", newRdbStore->GetRaw(), cloneHighlightSource.cloneStorePtr_, "", PHOTO_INFO_MAP
    };
    cloneRestoreHighlight->Init(initInfo);
    CloneRestoreHighlight::AnalysisAlbumInfo testAnalysisInfo;
    testAnalysisInfo.albumIdNew = make_optional<int32_t>(TEST_ID);
    testAnalysisInfo.coverUri = "file://testNewCoverUri";
    testAnalysisInfo.highlightIdNew = make_optional<int32_t>(TEST_ID);
    testAnalysisInfo.albumName = make_optional<string>("testAlbumName");
    cloneRestoreHighlight->analysisInfos_.emplace_back(testAnalysisInfo);

    CloneRestoreHighlight::HighlightCoverInfo testCoverInfo;
    testCoverInfo.highlightIdNew = make_optional<int32_t>(TEST_ID);
    cloneRestoreHighlight->coverInfos_.emplace_back(testCoverInfo);

    cloneRestoreHighlight->UpdateAlbums();
    string analysisCondition = "cover_uri = 'file://testNewCoverUri'";
    int32_t analysisCount = GetAlbumCountByCondition(newRdbStore->GetRaw(), ANALYSIS_ALBUM_TABLE, analysisCondition);
    EXPECT_EQ(analysisCount, 0);
    string coverCondition = "cover_key = 'testAlbumName_1_1_file://testNewCoverUri'";
    int32_t coverCount = GetAlbumCountByCondition(newRdbStore->GetRaw(), HIGHLIGHT_COVER_INFO_TABLE, coverCondition);
    EXPECT_EQ(coverCount, 0);
    ClearCloneSource(cloneHighlightSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_highlight_update_albums_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_highlight_update_albums_test_003 start");
    CloneHighlightSource cloneHighlightSource;
    vector<string> tableList = { ANALYSIS_ALBUM_TABLE, ANALYSIS_PHOTO_MAP_TABLE, HIGHLIGHT_COVER_INFO_TABLE };
    cloneHighlightSource.Insert(tableList, newRdbStore->GetRaw());
    CloneRestoreHighlight::InitInfo initInfo = {
        CLONE_RESTORE_ID, "", newRdbStore->GetRaw(), cloneHighlightSource.cloneStorePtr_, "", PHOTO_INFO_MAP
    };
    cloneRestoreHighlight->Init(initInfo);
    CloneRestoreHighlight::AnalysisAlbumInfo testAnalysisInfo;
    testAnalysisInfo.albumIdNew = make_optional<int32_t>(TEST_ID);
    testAnalysisInfo.coverUri = "file://testNewCoverUri";
    testAnalysisInfo.highlightIdNew = make_optional<int32_t>(TEST_ID);
    testAnalysisInfo.albumName = make_optional<string>("testAlbumName");
    cloneRestoreHighlight->analysisInfos_.emplace_back(testAnalysisInfo);

    CloneRestoreHighlight::HighlightCoverInfo testCoverInfo;
    testCoverInfo.ratio = make_optional<string>("1_1");
    cloneRestoreHighlight->coverInfos_.emplace_back(testCoverInfo);

    cloneRestoreHighlight->UpdateAlbums();
    string analysisCondition = "cover_uri = 'file://testNewCoverUri'";
    int32_t analysisCount = GetAlbumCountByCondition(newRdbStore->GetRaw(), ANALYSIS_ALBUM_TABLE, analysisCondition);
    EXPECT_EQ(analysisCount, 0);
    string coverCondition = "cover_key = 'testAlbumName_1_1_file://testNewCoverUri'";
    int32_t coverCount = GetAlbumCountByCondition(newRdbStore->GetRaw(), HIGHLIGHT_COVER_INFO_TABLE, coverCondition);
    EXPECT_EQ(coverCount, 0);
    ClearCloneSource(cloneHighlightSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_highlight_update_albums_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_highlight_update_albums_test_004 start");
    CloneHighlightSource cloneHighlightSource;
    vector<string> tableList = { ANALYSIS_ALBUM_TABLE, ANALYSIS_PHOTO_MAP_TABLE, HIGHLIGHT_COVER_INFO_TABLE };
    cloneHighlightSource.Insert(tableList, newRdbStore->GetRaw());
    CloneRestoreHighlight::InitInfo initInfo = {
        CLONE_RESTORE_ID, "", newRdbStore->GetRaw(), cloneHighlightSource.cloneStorePtr_, "", PHOTO_INFO_MAP
    };
    cloneRestoreHighlight->Init(initInfo);
    CloneRestoreHighlight::AnalysisAlbumInfo testAnalysisInfo;
    testAnalysisInfo.oldCoverUri = make_optional<string>("file://testOldCoverUri");
    testAnalysisInfo.albumIdNew = make_optional<int32_t>(TEST_ID);
    testAnalysisInfo.coverUri = "file://testNewCoverUri";
    testAnalysisInfo.highlightIdNew = make_optional<int32_t>(TEST_NEW_ID);
    testAnalysisInfo.albumName = make_optional<string>("testAlbumName");
    cloneRestoreHighlight->analysisInfos_.emplace_back(testAnalysisInfo);

    CloneRestoreHighlight::HighlightCoverInfo testCoverInfo;
    testCoverInfo.ratio = make_optional<string>("1_1");
    testCoverInfo.highlightIdNew = make_optional<int32_t>(TEST_ID);
    cloneRestoreHighlight->coverInfos_.emplace_back(testCoverInfo);

    cloneRestoreHighlight->UpdateAlbums();
    string analysisCondition = "cover_uri = 'file://testNewCoverUri'";
    int32_t analysisCount = GetAlbumCountByCondition(newRdbStore->GetRaw(), ANALYSIS_ALBUM_TABLE, analysisCondition);
    EXPECT_EQ(analysisCount, 1);
    string coverCondition = "cover_key = 'testAlbumName_1_1_file://testNewCoverUri'";
    int32_t coverCount = GetAlbumCountByCondition(newRdbStore->GetRaw(), HIGHLIGHT_COVER_INFO_TABLE, coverCondition);
    EXPECT_EQ(coverCount, 0);
    ClearCloneSource(cloneHighlightSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_highlight_update_albums_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_highlight_update_albums_test_005 start");
    CloneHighlightSource cloneHighlightSource;
    vector<string> tableList = { ANALYSIS_ALBUM_TABLE, ANALYSIS_PHOTO_MAP_TABLE, HIGHLIGHT_COVER_INFO_TABLE };
    cloneHighlightSource.Insert(tableList, newRdbStore->GetRaw());
    CloneRestoreHighlight::InitInfo initInfo = {
        CLONE_RESTORE_ID, "", newRdbStore->GetRaw(), cloneHighlightSource.cloneStorePtr_, "", PHOTO_INFO_MAP
    };
    cloneRestoreHighlight->Init(initInfo);
    CloneRestoreHighlight::AnalysisAlbumInfo testAnalysisInfo;
    testAnalysisInfo.oldCoverUri = make_optional<string>("file://testOldCoverUri");
    testAnalysisInfo.albumIdNew = make_optional<int32_t>(TEST_ID);
    testAnalysisInfo.coverUri = "file://testNewCoverUri";
    testAnalysisInfo.highlightIdNew = make_optional<int32_t>(TEST_ID);
    cloneRestoreHighlight->analysisInfos_.emplace_back(testAnalysisInfo);

    CloneRestoreHighlight::HighlightCoverInfo testCoverInfo;
    testCoverInfo.ratio = make_optional<string>("1_1");
    testCoverInfo.highlightIdNew = make_optional<int32_t>(TEST_ID);
    cloneRestoreHighlight->coverInfos_.emplace_back(testCoverInfo);

    cloneRestoreHighlight->UpdateAlbums();
    string analysisCondition = "cover_uri = 'file://testNewCoverUri'";
    int32_t analysisCount = GetAlbumCountByCondition(newRdbStore->GetRaw(), ANALYSIS_ALBUM_TABLE, analysisCondition);
    EXPECT_EQ(analysisCount, 1);
    string coverCondition = "cover_key = 'testAlbumName_1_1_file://testNewCoverUri'";
    int32_t coverCount = GetAlbumCountByCondition(newRdbStore->GetRaw(), HIGHLIGHT_COVER_INFO_TABLE, coverCondition);
    EXPECT_EQ(coverCount, 0);
    ClearCloneSource(cloneHighlightSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_highlight_get_new_highlight_album_id_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_highlight_get_new_highlight_album_id_test_001 start");
    int32_t oldId = 1;
    CloneRestoreHighlight::HighlightAlbumInfo testInfo;
    testInfo.highlightIdOld = make_optional<int32_t>(oldId);
    testInfo.highlightIdNew = make_optional<int32_t>(TEST_NEW_ID);
    cloneRestoreHighlight->highlightInfos_.emplace_back(testInfo);
    int32_t newId = cloneRestoreHighlight->GetNewHighlightAlbumId(oldId);
    EXPECT_EQ(newId, testInfo.highlightIdNew.value());
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_highlight_get_new_highlight_photo_id_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_highlight_get_new_highlight_photo_id_test_001 start");
    cloneRestoreHighlight->photoInfoMap_ = PHOTO_INFO_MAP;
    int32_t newId = cloneRestoreHighlight->GetNewHighlightPhotoId(TEST_ID);
    EXPECT_EQ(newId, TEST_NEW_ID);
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_highlight_get_new_highlight_photo_id_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_highlight_get_new_highlight_photo_id_test_002 start");
    cloneRestoreHighlight->photoInfoMap_ = PHOTO_INFO_MAP;
    int32_t newId = cloneRestoreHighlight->GetNewHighlightPhotoId(TEST_NEW_ID);
    EXPECT_EQ(newId, 0);
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_highlight_get_new_highlight_photo_uri_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_highlight_get_new_highlight_photo_uri_test_001 start");
    cloneRestoreHighlight->photoInfoMap_ = PHOTO_INFO_MAP;
    string newUri = cloneRestoreHighlight->GetNewHighlightPhotoUri(TEST_ID);
    EXPECT_FALSE(newUri.empty());
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_highlight_get_new_highlight_photo_uri_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_highlight_get_new_highlight_photo_uri_test_002 start");
    cloneRestoreHighlight->photoInfoMap_ = PHOTO_INFO_MAP;
    string newUri = cloneRestoreHighlight->GetNewHighlightPhotoUri(TEST_NEW_ID);
    EXPECT_TRUE(newUri.empty());
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_highlight_is_clone_highlight_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_highlight_is_clone_highlight_test_001 start");
    cloneRestoreHighlight->isCloneHighlight_ = false;
    bool isClone = cloneRestoreHighlight->IsCloneHighlight();
    EXPECT_EQ(isClone, false);
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_highlight_deduplicate_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_highlight_deduplicate_test_001 start");
    CloneHighlightSource cloneHighlightSource;
    vector<string> tableList = { ANALYSIS_ALBUM_TABLE, HIGHLIGHT_ALBUM_TABLE };
    cloneHighlightSource.Insert(tableList, newRdbStore->GetRaw());
    CloneRestoreHighlight::InitInfo initInfo = {
        CLONE_RESTORE_ID, "", newRdbStore->GetRaw(), cloneHighlightSource.cloneStorePtr_, "", PHOTO_INFO_MAP
    };
    cloneRestoreHighlight->Init(initInfo);
    CloneRestoreHighlight::AnalysisAlbumInfo testAnalysisInfo;
    testAnalysisInfo.albumIdOld = make_optional<int32_t>(TEST_ID);
    testAnalysisInfo.albumIdNew = make_optional<int32_t>(TEST_ID);
    testAnalysisInfo.highlightIdNew = make_optional<int32_t>(TEST_ID);
    testAnalysisInfo.albumName = make_optional<string>("test_highlight_album");
    cloneRestoreHighlight->analysisInfos_.emplace_back(testAnalysisInfo);

    CloneRestoreHighlight::HighlightAlbumInfo testHighlightInfo;
    testHighlightInfo.albumIdOld = make_optional<int32_t>(TEST_ID);
    testHighlightInfo.highlightVersion = make_optional<int32_t>(2);
    testHighlightInfo.highlightStatus = make_optional<int32_t>(1);
    testHighlightInfo.clusterType = make_optional<string>("TYPE_DBSCAN");
    testHighlightInfo.clusterSubType = make_optional<string>("Old_AOI_0");
    testHighlightInfo.clusterCondition = make_optional<string>("[]");
    cloneRestoreHighlight->highlightInfos_.emplace_back(testHighlightInfo);

    cloneRestoreHighlight->HighlightDeduplicate(testHighlightInfo);
    string highlightCondition = "highlight_status = -4";
    int32_t highlightCount = GetAlbumCountByCondition(newRdbStore->GetRaw(), HIGHLIGHT_ALBUM_TABLE, highlightCondition);
    EXPECT_EQ(highlightCount, 1);
    ClearCloneSource(cloneHighlightSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_highlight_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_highlight_test_001 start");
    CloneHighlightSource cloneHighlightSource;
    vector<string> tableList = { ANALYSIS_ALBUM_TABLE, ANALYSIS_PHOTO_MAP_TABLE, HIGHLIGHT_ALBUM_TABLE,
        HIGHLIGHT_COVER_INFO_TABLE, HIGHLIGHT_PLAY_INFO_TABLE, ANALYSIS_ASSET_SD_MAP_TABLE,
        ANALYSIS_ALBUM_ASSET_MAP_TABLE, VISION_LABEL_TABLE, VISION_SALIENCY_TABLE, VISION_RECOMMENDATION_TABLE };
    Init(cloneHighlightSource, TEST_BACKUP_DB_PATH, tableList);

    CloneRestoreHighlight::InitInfo initInfo = {
        CLONE_RESTORE_ID, "", nullptr, cloneHighlightSource.cloneStorePtr_, "", PHOTO_INFO_MAP
    };
    cloneRestoreHighlight->Init(initInfo);
    CloneRestoreHighlight::AnalysisAlbumInfo analysisAlbumInfo;
    cloneRestoreHighlight->analysisInfos_.emplace_back(analysisAlbumInfo);
    cloneRestoreHighlight->InsertIntoAnalysisAlbum();
    EXPECT_GT(cloneRestoreHighlight->albumFailedCnt_, 0);
    std::vector<NativeRdb::ValuesBucket> values;
    NativeRdb::ValuesBucket value;
    values.emplace_back(value);
    cloneRestoreHighlight->InsertAnalysisPhotoMap(values);
    EXPECT_GT(cloneRestoreHighlight->mapFailedCnt_, 0);
    CloneRestoreHighlight::HighlightAlbumInfo highlightAlbumInfo;
    cloneRestoreHighlight->GetHighlightNewAlbumId(highlightAlbumInfo);
    InsertIntoHighlightAlbumInfo(highlightAlbumInfo);
    cloneRestoreHighlight->highlightInfos_.emplace_back(highlightAlbumInfo);
    cloneRestoreHighlight->InsertIntoHighlightAlbum();
    EXPECT_GT(cloneRestoreHighlight->highlightFailedCnt_, 0);
    CloneRestoreHighlight::HighlightCoverInfo highlightCoverInfo;
    highlightCoverInfo.highlightIdNew = make_optional<int32_t>(TEST_ID);
    cloneRestoreHighlight->coverInfos_.emplace_back(highlightCoverInfo);
    cloneRestoreHighlight->InsertIntoHighlightCoverInfo();
    EXPECT_GT(cloneRestoreHighlight->coverInfoFailedCnt_, 0);

    ClearCloneSource(cloneHighlightSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_cv_analysis_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_cv_analysis_test_001 start");
    CloneHighlightSource cloneHighlightSource;
    vector<string> tableList = { ANALYSIS_ALBUM_TABLE, ANALYSIS_PHOTO_MAP_TABLE, HIGHLIGHT_ALBUM_TABLE,
        HIGHLIGHT_COVER_INFO_TABLE, HIGHLIGHT_PLAY_INFO_TABLE, ANALYSIS_ASSET_SD_MAP_TABLE,
        ANALYSIS_ALBUM_ASSET_MAP_TABLE, VISION_LABEL_TABLE, VISION_SALIENCY_TABLE, VISION_RECOMMENDATION_TABLE };
    Init(cloneHighlightSource, TEST_BACKUP_DB_PATH, tableList);

    CloneRestoreHighlight restoreHighlight;
    CloneRestoreHighlight::InitInfo initInfo = {
        CLONE_RESTORE_ID, "", newRdbStore->GetRaw(), cloneHighlightSource.cloneStorePtr_, "", PHOTO_INFO_MAP
    };
    restoreHighlight.Init(initInfo);
    restoreHighlight.Preprocess();
    restoreHighlight.RestoreAlbums();
    EXPECT_EQ(restoreHighlight.isCloneHighlight_, true);
    cloneRestoreCVAnalysis->Init(2, "", newRdbStore->GetRaw(), cloneHighlightSource.cloneStorePtr_, "");
    cloneRestoreCVAnalysis->RestoreAlbums(restoreHighlight);
    EXPECT_GT(cloneRestoreCVAnalysis->assetSdSuccessCnt_, 0);
    EXPECT_GT(cloneRestoreCVAnalysis->albumAssetSuccessCnt_, 0);

    ClearCloneSource(cloneHighlightSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_cv_analysis_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_cv_analysis_test_002 start");
    CloneHighlightSource cloneHighlightSource;
    vector<string> tableList = { ANALYSIS_ALBUM_TABLE, ANALYSIS_PHOTO_MAP_TABLE, HIGHLIGHT_ALBUM_TABLE,
        HIGHLIGHT_COVER_INFO_TABLE, HIGHLIGHT_PLAY_INFO_TABLE, ANALYSIS_ASSET_SD_MAP_TABLE,
        ANALYSIS_ALBUM_ASSET_MAP_TABLE, VISION_LABEL_TABLE, VISION_SALIENCY_TABLE, VISION_RECOMMENDATION_TABLE };
    Init(cloneHighlightSource, TEST_BACKUP_DB_PATH, tableList);

    cloneRestoreCVAnalysis->Init(2, "", newRdbStore->GetRaw(), cloneHighlightSource.cloneStorePtr_, "");

    std::vector<NativeRdb::ValuesBucket> values1;
    NativeRdb::ValuesBucket value1;
    value1.PutInt("map_asset_source", 1);
    value1.PutInt("map_asset_destination", 1);
    values1.emplace_back(value1);
    cloneRestoreCVAnalysis->InsertIntoAssetSdMap(values1);
    EXPECT_EQ(cloneRestoreCVAnalysis->assetSdFailedCnt_, 0);

    std::vector<NativeRdb::ValuesBucket> values2;
    NativeRdb::ValuesBucket value2;
    value2.PutInt("map_album", 1);
    value2.PutInt("map_asset", 1);
    values2.emplace_back(value2);
    cloneRestoreCVAnalysis->InsertIntoAlbumAssetMap(values2);
    EXPECT_EQ(cloneRestoreCVAnalysis->albumAssetFailedCnt_, 0);

    ClearCloneSource(cloneHighlightSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_cv_analysis_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_cv_analysis_test_004 start");
    CloneHighlightSource cloneHighlightSource;
    vector<string> tableList = { ANALYSIS_ALBUM_TABLE, ANALYSIS_PHOTO_MAP_TABLE, HIGHLIGHT_ALBUM_TABLE,
        HIGHLIGHT_COVER_INFO_TABLE, HIGHLIGHT_PLAY_INFO_TABLE, ANALYSIS_ASSET_SD_MAP_TABLE,
        ANALYSIS_ALBUM_ASSET_MAP_TABLE, VISION_LABEL_TABLE, VISION_SALIENCY_TABLE, VISION_RECOMMENDATION_TABLE };
    Init(cloneHighlightSource, TEST_BACKUP_DB_PATH, tableList);

    CloneRestoreHighlight restoreHighlight;
    CloneRestoreHighlight::InitInfo initInfo = {
        CLONE_RESTORE_ID, "", newRdbStore->GetRaw(), cloneHighlightSource.cloneStorePtr_, "", PHOTO_INFO_MAP
    };
    restoreHighlight.Init(initInfo);
    cloneRestoreCVAnalysis->Init(2, "", newRdbStore->GetRaw(), cloneHighlightSource.cloneStorePtr_, "");
    std::string oldPlayInfo1 = R"({})";
    nlohmann::json newPlayInfo1 = nlohmann::json::parse(oldPlayInfo1, nullptr, false);
    cloneRestoreCVAnalysis->ParsePlayInfo(oldPlayInfo1, restoreHighlight);
    EXPECT_FALSE(newPlayInfo1["effectline"].contains("effectline"));
    EXPECT_FALSE(newPlayInfo1.contains("timeline"));

    std::string oldPlayInfo2 = R"({
        "effectline": {
            "effectline": [
                {"effectVideoUri": "photo://uri1", "transitionVideoUri": "trans://uri1", "effect": "MASK2"},
                {"effectVideoUri": "highlight://uri2", "transitionVideoUri": "trans://uri2", "effect": "MASK1"}
            ]
        },
        "timeline": [
            {
                "effectVideoUri": "old_uri_1",
                "transitionVideoUri": "old_uri_2",
                "fileId": [1, 2],
                "fileUri": ["uri_1", "uri_2"]
            }
        ]
    })";
    nlohmann::json newPlayInfo2 = nlohmann::json::parse(oldPlayInfo2, nullptr, false);
    cloneRestoreCVAnalysis->ParsePlayInfo(oldPlayInfo2, restoreHighlight);
    EXPECT_TRUE(newPlayInfo2["effectline"].contains("effectline"));
    EXPECT_TRUE(newPlayInfo2.contains("timeline"));

    ClearCloneSource(cloneHighlightSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_cv_analysis_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_cv_analysis_test_006 start");
    CloneHighlightSource cloneHighlightSource;
    vector<string> tableList = { ANALYSIS_ALBUM_TABLE, ANALYSIS_PHOTO_MAP_TABLE, HIGHLIGHT_ALBUM_TABLE,
        HIGHLIGHT_COVER_INFO_TABLE, HIGHLIGHT_PLAY_INFO_TABLE, ANALYSIS_ASSET_SD_MAP_TABLE,
        ANALYSIS_ALBUM_ASSET_MAP_TABLE, VISION_LABEL_TABLE, VISION_SALIENCY_TABLE, VISION_RECOMMENDATION_TABLE };
    Init(cloneHighlightSource, TEST_BACKUP_DB_PATH, tableList);

    CloneRestoreHighlight restoreHighlight;
    CloneRestoreHighlight::InitInfo initInfo = {
        CLONE_RESTORE_ID, "", newRdbStore->GetRaw(), cloneHighlightSource.cloneStorePtr_, "", PHOTO_INFO_MAP
    };
    restoreHighlight.Init(initInfo);
    cloneRestoreCVAnalysis->Init(2, "", newRdbStore->GetRaw(), cloneHighlightSource.cloneStorePtr_, "");
    std::string oldPlayInfo1 = R"({
        "effectline": {
            "effectline": [
                {"effectVideoUri": "file://media/Photo/uri1",
                "transitionVideoUri": "trans://uri1", "effect": "TYPE_MASK2"},
                {"effectVideoUri": "file://media/highlight/video/uri2",
                "transitionVideoUri": "trans://uri2", "effect": "TYPE_MASK1"}
            ]
        }
    })";
    nlohmann::json newPlayInfo1 = nlohmann::json::parse(oldPlayInfo1, nullptr, false);
    std::string oldEffectVideoUri0 = newPlayInfo1["effectline"]["effectline"][0]["effectVideoUri"];
    std::string oldEffectVideoUri1 = newPlayInfo1["effectline"]["effectline"][1]["effectVideoUri"];
    EXPECT_TRUE(MediaFileUtils::StartsWith(oldEffectVideoUri0, PHOTO_URI_PREFIX));
    EXPECT_TRUE(MediaFileUtils::StartsWith(oldEffectVideoUri1, HIGHLIGHT_ASSET_URI_PREFIX));
    cloneRestoreCVAnalysis->ParseEffectline(newPlayInfo1, restoreHighlight);
    MEDIA_INFO_LOG("newPlayInfo1: %{public}s",
        newPlayInfo1.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace).c_str());
    std::string newEffectVideoUri1 = newPlayInfo1["effectline"]["effectline"][1]["effectVideoUri"];
    EXPECT_FALSE(MediaFileUtils::StartsWith(newEffectVideoUri1, HIGHLIGHT_ASSET_URI_PREFIX)); // invalid effectVideoUri

    std::string oldPlayInfo2 = R"({
        "effectline": {
            "effectline": [
                {"fileId": [1, 2], "prefileId": [3, 4], "fileUri": ["uri1", "uri2"], "prefileUri": ["uri3"]}
            ]
        },
        "timeline": [
            {}
        ]
    })";
    nlohmann::json newPlayInfo2 = nlohmann::json::parse(oldPlayInfo2, nullptr, false);
    cloneRestoreCVAnalysis->ParseTimeline(newPlayInfo2, restoreHighlight);
    cloneRestoreCVAnalysis->ParseEffectline(newPlayInfo2, restoreHighlight);
    EXPECT_TRUE(newPlayInfo2["effectline"]["effectline"][0].contains("fileId"));
    EXPECT_TRUE(newPlayInfo2["effectline"]["effectline"][0].contains("fileUri"));

    ClearCloneSource(cloneHighlightSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_restore_highlight_albums_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_restore_highlight_albums_test_001 start");
    CloneHighlightSource cloneHighlightSource;
    vector<string> tableList = { ANALYSIS_ALBUM_TABLE, ANALYSIS_PHOTO_MAP_TABLE, HIGHLIGHT_ALBUM_TABLE,
        HIGHLIGHT_COVER_INFO_TABLE, HIGHLIGHT_PLAY_INFO_TABLE, ANALYSIS_ASSET_SD_MAP_TABLE,
        ANALYSIS_ALBUM_ASSET_MAP_TABLE, PhotoColumn::PHOTOS_TABLE };
    Init(cloneHighlightSource, TEST_BACKUP_DB_PATH, tableList);

    CloneRestore restoreService;
    restoreService.mediaRdb_ = cloneHighlightSource.cloneStorePtr_;
    restoreService.mediaLibraryRdb_ = newRdbStore->GetRaw();
    restoreService.photoInfoMap_ = PHOTO_INFO_MAP;
    restoreService.RestoreHighlightAlbums();

    string analysisCondition = "album_name = 'test_highlight_album'";
    int32_t analysisCount = GetAlbumCountByCondition(newRdbStore->GetRaw(), ANALYSIS_ALBUM_TABLE, analysisCondition);
    EXPECT_GT(analysisCount, 0);

    ClearCloneSource(cloneHighlightSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_restore_analysis_tables_data_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_restore_analysis_tables_data_test_001 start");
    CloneHighlightSource cloneHighlightSource;
    vector<string> tableList = { PhotoColumn::PHOTOS_TABLE, VISION_TOTAL_TABLE, VISION_RECOMMENDATION_TABLE };
    Init(cloneHighlightSource, TEST_BACKUP_DB_PATH, tableList);

    CloneRestore restoreService;
    restoreService.mediaRdb_ = cloneHighlightSource.cloneStorePtr_;
    restoreService.mediaLibraryRdb_ = newRdbStore->GetRaw();
    restoreService.photoInfoMap_ = PHOTO_INFO_MAP;
    restoreService.RestoreAnalysisTablesData();

    string condition = " id > 0 ";
    int32_t count = GetAlbumCountByCondition(newRdbStore->GetRaw(), VISION_RECOMMENDATION_TABLE, condition);
    EXPECT_GT(count, 0);

    ClearCloneSource(cloneHighlightSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_restore_analysis_tables_data_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_restore_analysis_tables_data_test_002 start");
    CloneHighlightSource cloneHighlightSource;
    vector<string> tableList = { PhotoColumn::PHOTOS_TABLE, VISION_TOTAL_TABLE, VISION_SALIENCY_TABLE };
    Init(cloneHighlightSource, TEST_BACKUP_DB_PATH, tableList);

    CloneRestore restoreService;
    restoreService.mediaRdb_ = cloneHighlightSource.cloneStorePtr_;
    restoreService.mediaLibraryRdb_ = newRdbStore->GetRaw();
    restoreService.photoInfoMap_ = PHOTO_INFO_MAP;
    restoreService.RestoreAnalysisTablesData();

    string condition = " id > 0 ";
    int32_t count = GetAlbumCountByCondition(newRdbStore->GetRaw(), VISION_SALIENCY_TABLE, condition);
    EXPECT_GT(count, 0);

    ClearCloneSource(cloneHighlightSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_highlight_preprocess_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_highlight_preprocess_test_001 start");
    CloneHighlightSource cloneHighlightSource;
    vector<string> tableList;
    Init(cloneHighlightSource, TEST_BACKUP_DB_PATH, tableList);

    CloneRestoreHighlight::InitInfo initInfo = {
        CLONE_RESTORE_ID, "", newRdbStore->GetRaw(), cloneHighlightSource.cloneStorePtr_, "", PHOTO_INFO_MAP
    };
    cloneRestoreHighlight->Init(initInfo);
    cloneRestoreHighlight->Preprocess();
    EXPECT_EQ(cloneRestoreHighlight->isCloneHighlight_, false);

    ClearCloneSource(cloneHighlightSource, TEST_BACKUP_DB_PATH);
}

HWTEST_F(CloneRestoreHighlightTest, clone_restore_highlight_update_restore_time_cost_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("clone_restore_highlight_update_restore_time_cost_test_001 start");
    cloneRestoreHighlight->restoreTimeCost_ = 0;
    cloneRestoreHighlight->UpdateRestoreTimeCost(SLEEP_FIVE_SECONDS);
    EXPECT_GT(cloneRestoreHighlight->restoreTimeCost_, 0);
}
} // namespace Media
} // namespace OHOS